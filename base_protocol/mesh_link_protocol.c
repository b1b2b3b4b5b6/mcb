#include "mesh_link_protocol.h"

#define STD_LOCAL_LOG_LEVEL STD_LOG_INFO


#define SENDOUT_TASK_PRI  ESP_TASK_MAIN_PRIO + 3
#define SHELL_SPREAD_TASK_PRI  ESP_TASK_MAIN_PRIO + 4
#define SHELL_NODE_RECEIVE_TASK_PRI ESP_TASK_MAIN_PRIO + 5

#define SENDOUT_TASK_SIZE 3096
#define SHELL_SPREAD_TASK_SIZE 2048
#define SHELL_NODE_RECEIVE_TASK_SIZE 3072


#define SPREAD_SIZE 5

#define SEND_OUT_SISE 10

#define SEM_SIZE 20

static bool g_mesh_connect = false;
static bool g_router_connect = false;

typedef struct shell_queue_t{
    xQueueHandle queue;
}shell_queue_t;

static struct{
    xSemaphoreHandle sem;
    bool free;
}g_sem_list[SEM_SIZE];


shell_queue_t g_shell_queue[MAX_SHELL_TYPE] = {{0}};
xQueueHandle g_spread_queue;

static void sem_list_init()
{
    for(int n = 0; n < SEM_SIZE; n++)
    {
        g_sem_list[n].sem = xSemaphoreCreateBinary();
        g_sem_list[n].free = true;
    }
}

static int take_sem_index()
{
    static int offset = 0;
    int n;
    n = offset;
    for(;;)
    { 
        if(n >= SEM_SIZE)
            n = 0;
        if(g_sem_list[n].free)
        {
            g_sem_list[n].free = false;
            xSemaphoreTake(g_sem_list[n].sem, 0);
            offset ++;
            if(offset >= SEM_SIZE)
                offset = 0;
            return n;
        }
        n++;
    }
    return -1;
}

static void give_sem_index(int index)
{
    g_sem_list[index].free = true;
}

int mesh_link_shell_add(int type, int size)
{
    STD_ASSERT(g_shell_queue[type].queue == NULL);
    g_shell_queue[type].queue = xQueueCreate(size, sizeof(void *));
    STD_LOGD("mesh link add shell[%d]", type);
    return 0;
}

shell_receive_t *mesh_link_wait_shell(int type, uint32_t time_ms)
{
    shell_receive_t *shell_receive = NULL;

    if(xQueueReceive(g_shell_queue[type].queue, &shell_receive, time_ms/portTICK_PERIOD_MS) == pdFALSE)
        return NULL;
    else
        return shell_receive;                   

    return NULL;
}

void mesh_link_empty_shell(int type)
{
    shell_receive_t *recevie;
    for(;;)
    {
        if(xQueueReceive(g_shell_queue[type].queue, &recevie, 0) == pdTRUE)
            free_shell_receive(recevie);
        else
            return;
    }                   
}

static void send_self(base_shell_t *base_shell)
{
    uint8_t type = base_shell->prefix->type;
    shell_receive_t *recv = build_shell_receive(base_shell);
    if(g_shell_queue[type].queue == 0)
    {
        STD_LOGE("recv unregister shell type[%d]", type);
        return;
    }
    if(xQueueSend(g_shell_queue[type].queue, &recv, portMAX_DELAY) == pdFALSE)
        STD_ASSERT(false);   

}

static int mesh_link_singal_send(base_shell_t *base_shell, mesh_addr_t *dst_mac, int time_ms)
{
    int res;
    mesh_link_data_t mesh_link_data;
    mesh_link_data.data = base_shell->raw_data;
    mesh_link_data.len = base_shell->raw_data_len;

    if(time_ms != 0)
    {
        base_shell->prefix->response = true;
        base_shell->prefix->index = take_sem_index();
        res = mesh_link_send2node(dst_mac, &mesh_link_data);
        if(res != 0)
        {
            give_sem_index(base_shell->prefix->index);
            return -1;
        }

        if(xSemaphoreTake(g_sem_list[base_shell->prefix->index].sem, time_ms / portTICK_PERIOD_MS) == pdTRUE)
            res = 0;
        else
        {
            STD_LOGW("shell wait sem fail[%d]", base_shell->prefix->index);
            res = -1;
        }   
        give_sem_index(base_shell->prefix->index);
        return res;
    }
    else
    {
        return mesh_link_send2node(dst_mac, &mesh_link_data);
    }

}


static void send_ack(mesh_addr_t *mac, uint8_t index){
    static base_shell_t *ack = NULL;
    if(ack == NULL)
        ack = build_base_shell(SHELL_ACK, 1, 0);
    ack->prefix->index = index;
    memcpy(ack->mix_mac, mac, 6);
    mesh_link_singal_send(ack, mac, 0);
}

static void shell_node_receive_task(void *args)
{
    static mesh_addr_t src;
    static uint8_t data[MESH_MPS];
    static mesh_link_data_t mesh_link_data;
    mesh_link_data.data = data;
    
    //mesh_await_connect(0);
    while(1)
    {
        mesh_link_data.len = MESH_MPS;
        int res = mesh_link_node_receive(&src, &mesh_link_data, portMAX_DELAY);
        if(res == 0)
        {
            base_shell_t local_base;
            fill_base_shell(&local_base, mesh_link_data.data, mesh_link_data.len);

            if(local_base.prefix->type == SHELL_ACK)
            {   
                STD_LOGD("recv ack[%d]", local_base.prefix->index);
                if(g_sem_list[local_base.prefix->index].free == false)
                        xSemaphoreGive(g_sem_list[local_base.prefix->index].sem);
                continue;
            }
            
            STD_LOGD("receive type[%d] response[%d] mac_len[%d]", local_base.prefix->type, local_base.prefix->response, local_base.prefix->mac_len);
            if(local_base.prefix->response)
                send_ack(&src, local_base.prefix->index);
            
            if(local_base.prefix->mac_len == 1)
                send_self(&local_base);

            if(local_base.prefix->mac_len > 1 || local_base.prefix->mac_len == 0)
            {
                static int spread_index = -1;
                if(spread_index == local_base.prefix->index)
                {
                    STD_LOGW("same spread index[%d], ignore", spread_index);
                    continue;
                }
                mesh_link_send(&local_base, 0);
                continue;
            }

        }
    }
}


void mesh_link_spread(base_shell_t *base_shell)
{
    
    uint8_t spread_quality = base_shell->prefix->spread_quality;
    uint8_t retry = spread_quality/SPREAD_MUL + 1;
    int time_ms = spread_quality % SPREAD_MUL;
    switch(time_ms)
    {
        case SPREAD_0:
            time_ms = 0;
            break;
        case SPREAD_100:
            time_ms = 100;
            break;
        case SPREAD_200:
            time_ms = 200;
            break;
        case SPREAD_500:
            time_ms = 500;
            break;
        case SPREAD_1000:
            time_ms = 1000;
            break;
        case SPREAD_1500:
            time_ms = 1500;
            break;
        case SPREAD_3000:
            time_ms = 3000;
            break;
        case SPREAD_5000:
            time_ms = 5000;
            break;
        default:
            STD_ASSERT(-1);
    }
    STD_LOGM("spread time_ms[%d] retry[%d]", time_ms, retry);

    mac_list_t *mac_list = topo_get_child_list();
    for(int n = 0; n < mac_list->num; n++)
    {
        int res = STD_RETRY_TRUE_BREAK(mesh_link_singal_send(base_shell, (mesh_addr_t *)&mac_list->macs[n], time_ms) == 0, retry);
        if(res != 0)
            STD_LOGE("spread fail, mac: "MACSTR, MAC2STR((uint8_t *)&mac_list->macs[n]));
    }

    free_mac_list(mac_list);
}


static void shell_spread_task(void *argc)
{
    g_spread_queue = xQueueCreate(SPREAD_SIZE, sizeof(base_shell_t *));
    base_shell_t *base_shell;
    for(;;)
    {
        //FIXME:shall not use portTICK_PERIOD_MS
        if(xQueueReceive(g_spread_queue, &base_shell, portMAX_DELAY) == pdTRUE)
        {
            mesh_link_spread(base_shell);
            free_base_shell(base_shell);
        }  
        else
            continue;
    }
}

static int mesh_link_spread_send(base_shell_t *base_shell, int time_ms)
{   
    if(topo_get_child_num() <= 0)
        return 0;

    base_shell_t *copy = copy_base_shell(base_shell);
    if(xQueueSend(g_spread_queue, &copy, time_ms / portTICK_PERIOD_MS) == pdTRUE)
        return 0;
    else
    {
        STD_LOGE("spread queue full, send fail");
        free_base_shell(copy);
        return -1;
    }
}

void await_spread_empty()
{
    for(;;)
    {
        if(uxQueueMessagesWaiting(g_spread_queue) == 0)
            break;
        vTaskDelay(100/portTICK_PERIOD_MS);
    } 
}

void await_spread_free()
{
    for(;;)
    {
        if(uxQueueMessagesWaiting(g_spread_queue) < SPREAD_SIZE)
            break;
        vTaskDelay(100/portTICK_PERIOD_MS);
    } 
}

int mesh_link_send(base_shell_t *base_shell, int time_ms)
{
    if(!mesh_is_connect())
        return -1;

    STD_LOGD("singal send mac_len[%d], type[%d], response[%d]", base_shell->prefix->mac_len, base_shell->prefix->type, base_shell->prefix->response);

    if(base_shell->prefix->mac_len == 1)
    {
        if(std_wifi_is_self(base_shell->mix_mac))
        {
            send_self(base_shell);
            return 0;
        }
        else
            return mesh_link_singal_send(base_shell, base_shell->mix_mac, time_ms);
    }

    for(int n = 0; n < base_shell->prefix->mac_len; n++)
    {
        if(std_wifi_is_self(&base_shell->mix_mac[n]))
            send_self(base_shell);
    }

    if(base_shell->prefix->mac_len == 0)
    {
        send_self(base_shell);
        return 0;
    }

    return  mesh_link_spread_send(base_shell, time_ms);
}


bool mesh_is_connect()
{
    return g_mesh_connect;
}

bool router_is_connect()
{
    return g_router_connect;
}

void set_router_is_connect(bool value)
{
    g_router_connect = value;
}

void set_mesh_is_connect(bool value)
{
    g_mesh_connect = value;
}

int mesh_await_connect(int time_s)
{
    if(time_s == 0)
    {
        for(;;)
        {
            if(mesh_is_connect())
                return 0;
            vTaskDelay(1000/portTICK_PERIOD_MS);       
        }
    }
    for(int n = 0; n < time_s; n++)
    {
        if(mesh_is_connect())
            return 0;
        vTaskDelay(1000/portTICK_PERIOD_MS);
    }
    return -1;
}

int mesh_link_send_out(uint8_t type, uint8_t *data, int len, int time_ms)
{
    mesh_addr_t *root = topo_get_root();
    if(root == NULL)
    {
        STD_LOGE("send out fail, no root");
        return -1;
    }
    out_data_t *out = build_send_out_data(type, data, len);
    base_shell_t *shell = build_base_shell(SHELL_SEND_OUT, 1, out->raw_data_len);
    
    memcpy(shell->mix_mac, root, 6);
    memcpy(shell->data, out->raw_data, out->raw_data_len);
    free_out_data(out);
    int res = mesh_link_send(shell, time_ms);
    free_base_shell(shell);
    return res;
}

static void send_out_task(void *argc)
{
    for(;;)
    {
        shell_receive_t *recv = mesh_link_wait_shell(SHELL_SEND_OUT, portMAX_DELAY);
        //FIXME:shall not use portMAX_DELAY here;
        if(recv == NULL)
            return;    
        if(esp_mesh_is_root())
        {
            out_data_t out;
            fill_out_data(&out, recv->data, recv->len);
            if(mesh_send_out(&out) != 0)
                STD_LOGE("send out task fail");
        }   
        else
            STD_LOGE("node shall not handle out data");
        free_shell_receive(recv);
    }
}


void mesh_link_protocol_init(mesh_event_cb_t mesh_cb, int node_type)
{
    sem_list_init();
    mesh_link_init(mesh_cb, node_type);
    xTaskCreate(shell_node_receive_task, "shell_node_receive_task", SHELL_NODE_RECEIVE_TASK_SIZE, NULL, SHELL_NODE_RECEIVE_TASK_PRI, NULL);
    xTaskCreate(shell_spread_task, "shell_spread_task", SHELL_SPREAD_TASK_SIZE, NULL, SHELL_SPREAD_TASK_PRI, NULL);
    mesh_link_shell_add(SHELL_SEND_OUT, SEND_OUT_SISE);
    xTaskCreate(send_out_task, "send_out_task", SENDOUT_TASK_SIZE, NULL, SENDOUT_TASK_PRI, NULL);
}

#ifdef PRODUCT_TEST
//memery leak test: pass
//receive task continue run: pass
//send function: pass

#endif