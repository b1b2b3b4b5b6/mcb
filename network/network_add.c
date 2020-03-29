#include "network_add.h"

#define SEND_TIMES 300
#define SEND_INTERVAL_MS 20

#define STD_LOCAL_LOG_LEVEL STD_LOG_DEBUG

int send_mac(uint8_t *mac_sta)
{
    int res = -1;

    uint8_t mac_local[6];
    esp_wifi_get_mac(ESP_IF_WIFI_STA, mac_local);
    if(memcmp(mac_sta, mac_local, 6) == 0)
        return 0;

    network_info_t *info = STD_CALLOC(sizeof(network_info_t), 1);
    std_nvs_load(KEY_NETWORK, info, sizeof(network_info_t));

    char *data = STD_CALLOC(200, 1);
    int broadcast = 0;
    char *temp;
    json_pack_object(data, "broadcast", &broadcast);
    temp = info->ssid;
    json_pack_object(data, "ssid", &temp);
    temp = info->passwd;
    json_pack_object(data, "passwd", &temp);
    temp = info->mesh_id;
    json_pack_object(data, "mesh_id", &temp);
    STD_FREE(info);

    espnow_peer_t *peer = std_espnow_add_peer(mac_sta);
    espnow_send_t *send = build_espnow_send(ESPNOW_CONFIG, peer, (uint8_t *)data, strlen(data) + 1);
    for(int n = 0; n < SEND_TIMES; n++)
    {
        STD_LOGD("send mac["MACSTR"]", MAC2STR(mac_sta));
        if(std_espnow_write(send, true) == 0)
        {
            res = 0;
            break;
        }
        vTaskDelay(SEND_INTERVAL_MS / portTICK_PERIOD_MS);

    }
    free_espnow_send(send);
    std_espnow_del_peer(peer);

    STD_FREE(data);
    if(res != 0 )
        STD_LOGW("add mac fail : "MACSTR, MAC2STR(mac_sta));
    else
        STD_LOGD("add mac success : "MACSTR, MAC2STR(mac_sta));

    return res;
}

void send_broadcast()
{
    network_info_t *info = STD_CALLOC(sizeof(network_info_t), 1);
    std_nvs_load(KEY_NETWORK, info, sizeof(network_info_t));

    char *data = STD_CALLOC(200, 1);
    int broadcast = 1;
    char *temp;
    json_pack_object(data, "broadcast", &broadcast);
    temp = info->ssid;
    json_pack_object(data, "ssid", &temp);
    temp = info->passwd;
    json_pack_object(data, "passwd", &temp);
    temp = info->mesh_id;
    json_pack_object(data, "mesh_id", &temp);
    STD_FREE(info);

    espnow_send_t *send = build_espnow_send(ESPNOW_CONFIG, NULL, (uint8_t *)data, strlen(data) + 1);
    for(int n = 0; n < SEND_TIMES; n++)
    {
        STD_LOGD("send broadcast");
        std_espnow_write(send, true);
        vTaskDelay(SEND_INTERVAL_MS / portTICK_PERIOD_MS);
    }
    free_espnow_send(send);
    STD_FREE(data);
}

void auto_send()
{
    std_wifi_restore();
    network_config_t *config = config_network_get();
    if(config->broadcast == 1)
        send_broadcast();
    else
    {
        if(config->count != 0)
        {
            uint8_t ble_mac[6];
            for(int n = 0; n < config->count; n++)
            {
                str2mac(ble_mac, config->ble_macs[n]);
                ADDR_BT2STA(ble_mac);
                send_mac(ble_mac);
            }
        }
    }
    STD_LOGD("auto send complete");
}