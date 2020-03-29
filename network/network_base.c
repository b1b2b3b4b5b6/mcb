#include "network_base.h"

#define STD_LOCAL_LOG_LEVEL STD_LOG_DEBUG
#define WAIT_WIFI_TIME_MS 8000
#define RESULT_BUF_SIZE 200
#define WIFI_OK 0
#define WIFI_ERROR -1

xSemaphoreHandle g_network_config_lock = NULL;
network_config_t *g_network_config = NULL;
network_info_t g_network_info;
static bool g_end = false;

static void set_end()
{
    g_end = true;
}

static bool check_end()
{
    return g_end;
}

int config_network_lock()
{
    if(xSemaphoreTake(g_network_config_lock, 0) == pdTRUE)
        return 0;
    else
        return -1;
}

void config_network_unlock()
{
    xSemaphoreGive(g_network_config_lock);
}

int config_network_send(network_config_t *network_config)
{
    if(config_network_lock() == 0)
    {
        g_network_config = network_config;
        STD_LOGD("network config set and lock");
        return 0;
    }
    else
        return -1;
}

void config_network_init()
{
    if(g_network_config_lock == NULL)
        g_network_config_lock =  xSemaphoreCreateBinary();
    xSemaphoreGive(g_network_config_lock);
    STD_LOGD("config network init");
}

void config_network_deinit()
{
    STD_ASSERT(g_network_config_lock != NULL);
    vSemaphoreDelete(g_network_config_lock);
    g_end = false;
    free_network_config(g_network_config);
    g_network_config = NULL;
    STD_LOGD("config network deinit");
}

void config_network_wait()
{
    for(;;)
    {
        vTaskDelay(300/portTICK_PERIOD_MS);
        if(check_end())
        {
            network_info_t info;
            memset(&info, 0, sizeof(network_info_t));
            strcpy(info.ssid, g_network_config->ssid);
            strcpy(info.passwd, g_network_config->passwd);
            strcpy(info.mesh_id, g_network_config->mesh_id);
            std_nvs_save(KEY_NETWORK, &info, sizeof(network_info_t));
            break;
        }
    }
}

network_config_t *config_network_get()
{
    return g_network_config;
}

void free_network_config(network_config_t *network_config)
{
    if(network_config == NULL)
        return;
    
    STD_FREE(network_config->ssid);
    STD_FREE(network_config->passwd);
    STD_FREE(network_config->mesh_id);

    if(network_config->ble_macs != NULL)
    {
        char **ble_macs = network_config->ble_macs;
        for(int n = 0; n < network_config->count; n++)
        {
            STD_FREE(ble_macs[n]);
        }
        STD_FREE(ble_macs);
    }

    STD_FREE(network_config);
}

static void send_config_result(int status)
{
    char *result = STD_CALLOC(RESULT_BUF_SIZE, 1);
    json_pack_object(result, "status", &status);
    esp_blufi_send_custom_data((uint8_t *)result, strlen(result) + 1);
    STD_FREE(result);
}

static void handle_config(char *data)
{
    network_config_t *network_config = STD_CALLOC(sizeof(network_config_t), 1);
    STD_ERROR_GOTO_FAIL(json_parse_object(data, "ssid", &network_config->ssid));
    STD_ERROR_GOTO_FAIL(json_parse_object(data, "password", &network_config->passwd));
    STD_ERROR_GOTO_FAIL(json_parse_object(data, "mesh_id", &network_config->mesh_id));
    STD_ERROR_GOTO_FAIL(json_parse_object(data, "broadcast", &network_config->broadcast));
    if(network_config->broadcast == 0)
        network_config->count = json_parse_object(data, "ble_macs", &network_config->ble_macs);

    STD_LOGD("ssid: %s", network_config->ssid);
    STD_LOGD("password: %s", network_config->passwd);
    STD_LOGD("mesh_id: %s", network_config->mesh_id);
    STD_LOGD("broadcast: %d", network_config->broadcast);


    /*if recv ssid is NOROUTER, jump wifi check, use no router mesh*/
    if(strcmp(network_config->ssid, NOROUTER) == 0)
    {
        send_config_result(WIFI_OK);
        STD_LOGD("no router, info ok");
        if(config_network_send(network_config) == -1)
            free_network_config(network_config);
        set_end();
        return;    
    }

    std_wifi_lock();
    std_wifi_disconnect();
    std_wifi_connect(network_config->ssid, network_config->passwd);
    if(std_wifi_await_connect(8) == 0)
    {
        send_config_result(WIFI_OK);
        STD_LOGD("wifi info ok");
        if(config_network_send(network_config) == -1)
            free_network_config(network_config);
        set_end();
    }
    else
    {
        std_wifi_disconnect();
        send_config_result(WIFI_ERROR);
        STD_LOGD("wifi info error");
        free_network_config(network_config);
    }
    std_wifi_unlock();

    return;
FAIL:
    free_network_config(network_config);
}

static void handle_wifi_status(char *data)
{
    send_config_result(WIFI_ERROR);
}

void config_blufi_cb(uint8_t *data, int len)
{
    char *json = STD_CALLOC(len + 1, 1);
    memcpy(json, data, len);

    char *type = NULL;
    STD_CHECK_GOTO_FAIL(json_parse_object(json, "type", &type) == 0); 

    if(strcmp(type, "config") == 0)
        handle_config(json);

    if(strcmp(type, "wifi_status") == 0)
        handle_wifi_status(json);

FAIL:
    STD_FREE(type);
    STD_FREE(json);
}

void config_chain_cb(uint8_t *data, int len)
{
    std_espnow_select_type(ESPNOW_NONE);
    STD_LOGM("recv: %s", data);
    network_config_t *network_config = STD_CALLOC(sizeof(network_config_t), 1);;
    STD_ERROR_GOTO_FAIL(json_parse_object(data, "ssid", &network_config->ssid));
    STD_ERROR_GOTO_FAIL(json_parse_object(data, "passwd", &network_config->passwd));
    STD_ERROR_GOTO_FAIL(json_parse_object(data, "mesh_id", &network_config->mesh_id));
    STD_ERROR_GOTO_FAIL(json_parse_object(data, "broadcast", &network_config->broadcast));
    if(config_network_send(network_config) == -1)
        free_network_config(network_config);
    set_end();
    vTaskDelete(NULL);

FAIL:
    free_network_config(network_config);
}