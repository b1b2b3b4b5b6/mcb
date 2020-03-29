#include "device_config.h"

#define STD_LOCAL_LOG_LEVEL STD_LOG_DEBUG
#define REBOOT_MS 5000

static void *add_device_func(uint8_t *mac, char *data)
{
    char *sta_mac = NULL;
    json_parse_object(data, "Mac", &sta_mac);
    
    if(sta_mac == NULL)
        send_broadcast();
    else
    {
        uint8_t temp[6];
        str2mac(temp, sta_mac);
        send_mac(temp);
        json_free(sta_mac, 0);
    }
    return NULL;
}

static void *delete_device_func(uint8_t *mac, char *data)
{
    network_erase();
    std_reset(REBOOT_MS);
    return NULL;
}

static void *change_netconfig_func(uint8_t *mac, char *data)
{
    network_info_t info;
    std_nvs_load(KEY_NETWORK, &info, sizeof(network_info_t));
    char *temp_str = NULL;
    json_parse_object(data, "SSID", &temp_str);
    if(temp_str != NULL)
    {
        strcpy(info.ssid, temp_str);
        info.ssid[strlen(temp_str)] = 0;
        STD_LOGW("new SSID[%s]", info.ssid);
    }
    json_free(temp_str, 0);

    json_parse_object(data, "PASSWD", &temp_str);
    if(temp_str != NULL)
    {
        strcpy(info.passwd, temp_str);
        info.passwd[strlen(temp_str)] = 0;
        STD_LOGW("new PASSWD[%s]", info.passwd);
    }
    json_free(temp_str, 0);

    json_parse_object(data, "MESHID", &temp_str);
    if(temp_str != NULL)
    {
        strcpy(info.mesh_id, temp_str);
        info.mesh_id[strlen(temp_str)] = 0;
        STD_LOGW("new MESHID[%s]", info.mesh_id);
    }
    json_free(temp_str, 0);
    std_nvs_save(KEY_NETWORK, &info, sizeof(network_info_t));
    std_reboot(REBOOT_MS);
    return NULL;
}

void device_config_init()
{
    ADD_CMD("Add", add_device_func);
    ADD_CMD("Delete", delete_device_func);
    ADD_CMD("ChangeConfig", change_netconfig_func);
}