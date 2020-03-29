#include "mesh_log_data.h"

#define STD_LOCAL_LOG_LEVEL STD_LOG_DEBUG

#define BUF_SIZE 1000

char *get_log_data(char *log)
{
    mesh_addr_t *parent =topo_get_parent();
    if(parent == NULL)
    {
        STD_LOGW("no parent, no log");
        return NULL;
    }

    char *json = STD_CALLOC(BUF_SIZE, 1);

    char *typ = "log";
    json_pack_object(json, "Typ", &typ);
        
    char *mac_str = std_wifi_get_stamac_str();
    json_pack_object(json, "Mac", &mac_str);

    char str[50] = {0};
    char *s = str;
    mac2str((uint8_t *)parent, str);
    json_pack_object(json, "ParentMac", &s);

    uint8_t layer = topo_get_layer();
    json_pack_object(json, "Layer", &layer);

    char *ver = std_ota_version();
    json_pack_object(json, "Version", &ver);

    uint32_t stamp = esp_log_timestamp();
    json_pack_object(json, "TimeStamp", &stamp);

    uint32_t free_heap = esp_get_free_heap_size();
    json_pack_object(json, "FreeHeap", &free_heap);

    char *device_typ = DEVICE_TYP;
    json_pack_object(json, "DeviceTyp", &device_typ);

    wifi_ap_record_t record;
    esp_wifi_sta_get_ap_info(&record);
    json_pack_object(json, "Rssi", &record.rssi);
    
    json_pack_object(json, "Log", &log);
    return json;
}

void free_log_data(char *json)
{
    STD_FREE(json);
}