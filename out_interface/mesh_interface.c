#include "mesh_interface.h"

#define STD_LOCAL_LOG_LEVEL STD_LOG_DEBUG 
#define TASK_PRI ESP_TASK_MAIN_PRIO
#define TASK_SIZE 2048



static xSemaphoreHandle g_send_lock = NULL;
static xQueueHandle g_queue[MAX_RECV_TYPE] = {NULL};

void mesh_interface_add_type(uint8_t type, uint8_t size)
{
    STD_ASSERT(g_queue[type] == NULL);
    g_queue[type] = xQueueCreate(sizeof(out_data_t *), size);
}

static void send_lock()
{
    xSemaphoreTake(g_send_lock, portMAX_DELAY);
}

static void send_unlock()
{
    xSemaphoreGive(g_send_lock);
}

int mesh_send_out(out_data_t *out)
{
    send_lock();
    static uint8_t *mesh_id = NULL;
    if(mesh_id == NULL)
    {
        network_info_t info;
        std_nvs_load(KEY_NETWORK, &info, sizeof(network_info_t));
        mesh_id = STD_MALLOC(6);
        str2mac(mesh_id, info.mesh_id);
    }
    memcpy(out->prefix->send.mesh_id, mesh_id, 6);
    out->prefix->send.len = out->raw_data_len - sizeof(out->prefix->send.len);
    flush_big_uint16(&out->prefix->send.len, out->prefix->send.len);
    int res = interface_send(out->raw_data, out->raw_data_len);
    send_unlock();

    return res;
}

void fill_out_data(out_data_t *out, uint8_t *raw_data, int raw_data_len)
{
    out->raw_data = raw_data;
    out->raw_data_len = raw_data_len;
    out->prefix = (out_data_prefix_t *)raw_data;
    out->data_len = raw_data_len - sizeof(out_data_prefix_t);
    out->data = &out->raw_data[sizeof(out_data_prefix_t)];
}

out_data_t *build_send_out_data(uint8_t type, uint8_t *data, int len)
{
    out_data_t *out = STD_CALLOC(1, sizeof(out_data_t));
    out->raw_data_len = len + sizeof(out_data_prefix_t);
    out->raw_data = STD_MALLOC(out->raw_data_len);
    out->prefix = (out_data_prefix_t *)out->raw_data;
    out->prefix->send.type = type;
    out->data = &out->raw_data[sizeof(out_data_prefix_t)];
    out->data_len = len;
    memcpy(out->data, data, len);
    return out;
}

out_data_t *build_recv_out_data(uint8_t *data, int len)
{
    out_data_t *out = STD_CALLOC(1, sizeof(out_data_t));
    out->raw_data_len = len;
    out->raw_data = STD_MALLOC(out->raw_data_len);
    memcpy(out->raw_data, data, out->raw_data_len);
    out->prefix = (out_data_prefix_t *)out->raw_data;
    out->data_len = len - sizeof(out_data_prefix_t);
    out->data = &out->raw_data[sizeof(out_data_prefix_t)];
    
    return out;
}

void print_out_data(out_data_t *out)
{
    STD_LOGD("----------out_data-----------");
    STD_LOGD("len[%d] type[%d] mesh_id["MACSTR"]",out->prefix->send.len, out->prefix->send.type, MAC2STR(out->prefix->send.mesh_id));
    PRINT_HEX(out->raw_data, out->raw_data_len);
    STD_LOGD("-----------------------------");
}

void free_out_data(out_data_t *out)
{
    if(out == NULL)
        return;
    if(out->raw_data == NULL)
    {
        STD_FREE(out);
        return;
    }
    STD_FREE(out->raw_data);
    STD_FREE(out);
}

void mesh_empty_out(uint8_t type)
{
    for(;;)
    {
        out_data_t *out;
        out = mesh_wait_out(type, 0);
        if(out == NULL)
            break;
        free_out_data(out);
    }
}

out_data_t *mesh_wait_out(uint8_t type, int time_ms)
{
    out_data_t *out;
    if(xQueueReceive(g_queue[type], &out, time_ms / portTICK_PERIOD_MS) == pdTRUE)
        return out;
    else
        return NULL;
}

static void loop_func(void *argc)
{
    for(;;)
    {
        interface_receive_t *recv = interface_wait_data(10000);
        if(recv == NULL)
            continue;

        out_data_t *out = build_recv_out_data(recv->data, recv->len);
        free_interface_receive(recv); 
        if(g_queue[out->prefix->recv.type] == NULL)
            STD_LOGE("recv unregister out data[%d]", out->prefix->recv.type);
        else
            STD_ASSERT(xQueueSend(g_queue[out->prefix->recv.type], &out, portMAX_DELAY) == pdTRUE);  
    }
}

void mesh_interface_init()
{
    interface_config_t cfg;
    STD_ASSERT(std_nvs_load(INTERFACE_KEY, &cfg, sizeof(interface_config_t)) == 0);
    switch(cfg.type)
    {
        case INTERFACE_UDP:
            interface_udp_init();
            break;
        case INTERFACE_TCP:
            interface_tcp_init();
            break;
        default:
            STD_LOGE("interface typ[%d] illegal", cfg.type);
            STD_ASSERT(-1);
    }

    interface_protocol_init();
    g_send_lock = xSemaphoreCreateBinary();
    xSemaphoreGive(g_send_lock);
    xTaskCreate(loop_func, "mesh interface", TASK_SIZE, NULL, TASK_PRI, NULL);
    mesh_interface_add_type(OUT_BIN, 5);
}