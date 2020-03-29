#include "mesh_report.h"

#define STD_LOCAL_LOG_LEVEL STD_LOG_DEBUG

#define TASK_SIZE 2048
#define TASK_PRI ESP_TASK_MAIN_PRIO

#define BRUST_SIZE 5
#define UPGATHER_SIZE 5

#define SEND_RETRY 3
#define SEND_TIME_OUT_MS 3000

#define BRUST_BUF 1024
#define GATHER_BUF 1024
#define NUM_BUF 100


struct{
    xSemaphoreHandle start_sem;
    int time_out_ms;
}g_upgather_status;

static xTimerHandle brust_repeat;

bool mesh_is_primary()
{
    static int res = 0; 
    if(res == 0)
    {
        network_info_t info;
        std_nvs_load(KEY_NETWORK, &info, sizeof(info));
        if(strcmp(info.mesh_id, INIT_MESH_ID) == 0)
            res = 11;
        else
            res = 10;
    }
    
    switch(res)
    {
        case 11:
            return true;
            break;
        case 10:
            return false;
            break;
        default:
            STD_ASSERT(-1);
            return false;
            break;
    }
    
}

void mesh_report_log(char *str)
{
    if(is_self_ota())
    {
        STD_LOGW("can not send log in ota progress");
        return;
    }

    if(!mesh_is_connect())
    {
        STD_LOGE("mesh not connect, no report log");
        return;      
    }
        
    char *json = get_log_data(str);
    if(json == NULL)
        return;
    int res = STD_RETRY_TRUE_BREAK(mesh_link_send_out(OUT_STR, (uint8_t *)json, strlen(json) + 1, SEND_TIME_OUT_MS) == 0, SEND_RETRY);
    free_log_data(json);
    if(res != 0)
        STD_LOGE("report log fail");
    else
        STD_LOGD("report log success");
    STD_FREE(json);
}

void mesh_report_online()
{
    if(!esp_mesh_is_root())
    {
        STD_LOGE("not root , no report online");
        return;
    }

    if(!mesh_is_connect())
    {
        STD_LOGE("mesh not connect, no report online");
        return;      
    }
        
    char *json = get_online_data();
    int res = STD_RETRY_TRUE_BREAK(mesh_link_send_out(OUT_STR, (uint8_t *)json, strlen(json) + 1, SEND_TIME_OUT_MS) == 0, SEND_RETRY);
    free_online_data(json);
    if(res != 0)
        STD_LOGE("report online fail");
    else
        STD_LOGD("report online success");   
}

void mesh_report_brust()
{   
    //lock prevernt unuseless repeat brust
    static uint8_t lock = false;
    if(lock == true)
        return;
    lock = true;
    if(!mesh_is_connect())
    {
        STD_LOGE("mesh not connect, no report brust");
        return;      
    }

    if(is_self_ota())
    {
        STD_LOGW("can not send data in ota progress");
        return;
    }

    char *json = get_brust_data();
    if(json == NULL)
        return;
    int res = STD_RETRY_TRUE_BREAK(mesh_link_send_out(OUT_STR, (uint8_t *)json, strlen(json) + 1, SEND_TIME_OUT_MS) == 0, SEND_RETRY);
    free_brust_data(json);
    if(res != 0)
        STD_LOGE("report brust fail");
    else
        STD_LOGD("report brust success");
    STD_ASSERT(xTimerReset(brust_repeat, 0) == pdTRUE);
    lock = false;
}

void mesh_report_gather()
{
    mesh_cmd_shell_t *cmd = build_mesh_cmd_shell("gather", 0, 200);
    if(mesh_cmd_spread(cmd, SPREAD_3000, SEND_RETRY) != 0)
        STD_LOGE("launch gather fail");
    else
        STD_LOGD("launch gather success");
    free_mesh_cmd_shell(cmd);
}

static void gather_up()
{
    if(esp_mesh_is_root())
    {
        int res = STD_RETRY_TRUE_BREAK(mesh_link_send_out(OUT_BIN, get_gather_data(), get_gather_data_len(), SEND_TIME_OUT_MS) == 0, SEND_RETRY);
        if(res != 0)
            STD_LOGE("root gather up fail");
        else
            STD_LOGD("root gather up success");
    }
    else
    {
        mesh_addr_t *parent = topo_get_parent();
        if(parent == NULL)
        {
            STD_LOGE("node gather up fail, no parnet");
            return;
        }
        base_shell_t *shell = build_base_shell(SHELL_UPGATHER, 1, get_gather_data_len());
        memcpy(shell->mix_mac, parent, 6);
        memcpy(shell->data, get_gather_data(), get_gather_data_len());
        if(STD_RETRY_TRUE_BREAK(mesh_link_send(shell, SEND_TIME_OUT_MS) == 0, SEND_RETRY) != 0)
            STD_LOGE("node gather up fail, send fail");
        else
            STD_LOGD("node gather up success");
        free_base_shell(shell);
    }

}

static void gather_handel_task(void *argc)
{
    for(;;)
    {
        reset_gather_data();
        if(xSemaphoreTake(g_upgather_status.start_sem, 5000 / portTICK_PERIOD_MS) != pdTRUE)   
            continue;

        int num = topo_get_child_num();
        STD_LOGD("start collect report, time_ms[%d], num[%d]", g_upgather_status.time_out_ms, num);

        for(int n = 0; n < g_upgather_status.time_out_ms / 200; n++)
        {
            shell_receive_t *recv = mesh_link_wait_shell(SHELL_UPGATHER, 200);//FIXME: could result in wdt fail
            if(recv == NULL)
                continue;
            STD_LOGD("recv gather["MACSTR"], len[%d]", MAC2STR(recv->mac), recv->len);
            if(add_gather_data(recv->data, recv->len) != 0)
            {
                gather_up();
                empty_gather_data();
                STD_ASSERT(add_gather_data(recv->data, recv->len) != 0);
            }
            num = num - 1;
            if(num == 0)
                break;
        }
        gather_up();
    }

}

static void handle_start(uint8_t *mac, char *data)
{
    if(is_self_ota())
    {
        set_self_ota(false);
        STD_LOGD("cancle ota status");
    }

    mesh_link_empty_shell(SHELL_UPGATHER);
    switch(topo_get_layer())
    {
        case 1:
            g_upgather_status.time_out_ms = 20000;
            break;

        case 2:
            g_upgather_status.time_out_ms = 15000;
            break;

        case 3:
            g_upgather_status.time_out_ms = 10000;
            break;

        case 4:
            g_upgather_status.time_out_ms = 7000;
            break;

        case 5:
            g_upgather_status.time_out_ms = 3000;
            break;

        case 6:
            g_upgather_status.time_out_ms = 0;
            break;
        default:
            STD_ASSERT(-1);
    }
    if(topo_get_child_num() == 0)
        g_upgather_status.time_out_ms = 0;
    xSemaphoreGive(g_upgather_status.start_sem);
    STD_LOGD("start upgather success");
}
static void brust_repeat_cb(void *arg)
{
    mesh_report_brust();
}

static mesh_cmd_handle_t gather_cmd_handle = {
    .cmd = "gather",
    .func = handle_start,
};


void mesh_report_init()
{
    device_data_init();
    g_upgather_status.start_sem = xSemaphoreCreateBinary();
    xSemaphoreTake(g_upgather_status.start_sem, 0);
    mesh_cmd_handle_add(&gather_cmd_handle);
    mesh_link_shell_add(SHELL_UPGATHER, UPGATHER_SIZE);
    xTaskCreate(gather_handel_task, "gather task", TASK_SIZE, NULL, TASK_PRI, NULL);
    if(!mesh_is_primary())
    {
        brust_repeat = xTimerCreate("brust repeat",  (BRUST_TIME_PEROID_S + esp_random() % BRUST_TIME_PEROID_S)* 1000 /portTICK_PERIOD_MS, pdTRUE, NULL, brust_repeat_cb);
        STD_ASSERT(xTimerStart(brust_repeat, 0) == pdTRUE);
    }
}

