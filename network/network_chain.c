#include "network_chain.h"

#define STD_LOCAL_LOG_LEVEL STD_LOG_DEBUG
#define TASK_SIZE 2048
#define TASK_PRI (ESP_TASK_MAIN_PRIO + 3)
#define WAIT_TIME_MS 100
static data_handle_cb_t g_data_handle_cb = NULL;
static bool g_stop_flag = false;

static void chain_slave_task(void *arg)
{
    int channel = 1;
    espnow_recv_t *recv;

    for(;;)
    {
        recv = std_espnow_read(ESPNOW_CONFIG, WAIT_TIME_MS);
        if(recv != NULL)
        {
            g_data_handle_cb(recv->data.data, 0);
            free_espnow_recv(recv);
        }
            
        if(g_stop_flag)
            vTaskDelete(NULL);
    
        std_wifi_lock();
        channel = channel % 13 + 1;
        esp_wifi_set_channel(channel,0);//FIXME: may falut here, do not why
        std_wifi_unlock();
        vTaskDelay(200/portTICK_PERIOD_MS);
    }
}

void chain_start(data_handle_cb_t cb)
{
    std_espnow_init();
    g_data_handle_cb = cb;
    g_stop_flag = false;
    std_espnow_select_type(ESPNOW_CONFIG);
    xTaskCreate(chain_slave_task, "chain slave task", TASK_SIZE, NULL, TASK_PRI, NULL);
    STD_LOGD("chain start success");
}

void chain_stop()
{
    std_espnow_select_type(ESPNOW_NONE);
    std_espnow_empty();
    g_stop_flag = true;
    vTaskDelay(WAIT_TIME_MS / portTICK_PERIOD_MS);
    STD_LOGD("chain stop success");
}