#include "mesh_link_trans.h"

#define STD_LOCAL_LOG_LEVEL STD_LOG_INFO
#define MESH_AP_AUTHMODE WIFI_AUTH_WPA2_PSK
#define MESH_HEART_TIME_S 10



static xSemaphoreHandle g_send_lock;

static void send_unlock()
{
    xSemaphoreGive(g_send_lock);
}

static void send_lock()
{
    xSemaphoreTake(g_send_lock, portMAX_DELAY);
}

static void send_lock_init()
{
    g_send_lock = xSemaphoreCreateBinary();
    xSemaphoreGive(g_send_lock);
}

int mesh_link_root_receive(mesh_addr_t *src_addr, mesh_link_data_t *link_data, int timeout_ms)
{
    static mesh_addr_t to;
    static int flag;
    static mesh_data_t mesh_data;
    int res;

    mesh_data.data = link_data->data;
    mesh_data.size = link_data->len;
    res = esp_mesh_recv_toDS(src_addr, &to, &mesh_data, timeout_ms, &flag, NULL, 0);
    
    switch(res)
    {
        case ESP_OK:
            STD_LOGD("root receive[%d]: success", mesh_data.size);
            link_data->len = mesh_data.size;
            break;

        case ESP_ERR_MESH_TIMEOUT:
            STD_LOGV("root receive: timeout");
            goto FAIL;
            break;

        default:
            STD_LOGE("root receive error: %s", esp_err_to_name(res));
            break;
    }
    return 0;

FAIL:
    return -1;
}

int mesh_link_node_receive(mesh_addr_t *src_addr, mesh_link_data_t *link_data, int timeout_ms)
{
    static mesh_opt_t opt;
    static int flag;
    static mesh_data_t mesh_data;
    int res;

    mesh_data.data = link_data->data;
    mesh_data.size = link_data->len;
    res = esp_mesh_recv(src_addr, &mesh_data, timeout_ms, &flag, &opt, 1);

    switch(res)
    {
        case ESP_OK:
            STD_LOGD("node receive[%d]: success", mesh_data.size);
            link_data->len = mesh_data.size;
            break;

        case ESP_ERR_MESH_TIMEOUT:
            STD_LOGV("node receive: timeout");
            break;

        default:
            STD_LOGE("node receive error: %s", esp_err_to_name(res));
            break;
    }
    return res;
}

int mesh_link_send2root(mesh_link_data_t *link_data)
{
    static int flag = MESH_DATA_TODS;
    static mesh_data_t mesh_data;
    int res;  

    mesh_data.data = link_data->data;
    mesh_data.size = link_data->len;

    send_lock();
    res = esp_mesh_send((mesh_addr_t *)WIFI_MESH_ROOT_TODS_ADDR, &mesh_data, flag, NULL,  0);
    send_unlock();

    switch(res)
    {
        case ESP_OK:
            STD_LOGD("send2root[%d]: success", mesh_data.size);
            return 0;
            break;

        default:
            STD_LOGE("send2root error: %s", esp_err_to_name(res));
            return -1;
            break;
    }

}

int mesh_link_send2node(mesh_addr_t *dst_addr, mesh_link_data_t *link_data)
{
    static int flag = MESH_DATA_P2P;
    static mesh_data_t mesh_data;
    int res;  

    mesh_data.data = link_data->data;
    mesh_data.size = link_data->len;

    STD_LOGD("send to node["MACSTR"]", MAC2STR((uint8_t *)dst_addr));
    send_lock();
    res = esp_mesh_send(dst_addr, &mesh_data, flag, NULL,  0);
    send_unlock();

    switch(res)
    {
        case ESP_OK:
            STD_LOGD("send2node[%d]: success", mesh_data.size);
            return 0;
            break;

        default:
            STD_LOGE("send2node error: %s", esp_err_to_name(res));
            return -1;
            break;
    }
    

}

int mesh_link_broadcast(mesh_link_data_t *link_data)
{
    static int flag = MESH_DATA_P2P;
    static mesh_data_t mesh_data;
    int res;  

    mesh_data.data = link_data->data;
    mesh_data.size = link_data->len;

    send_lock();
    res = esp_mesh_send((mesh_addr_t *)WIFI_MESH_BROADCAST_ADDR, &mesh_data, flag, NULL,  0);
    send_unlock();

    switch(res)
    {
        case ESP_OK:
            STD_LOGD("broadcase[%d]: success", mesh_data.size);
            return 0;
            break;

        default:
            STD_LOGE("broadcase error: %s", esp_err_to_name(res));
            return -1;
            break;
    }
}

int mesh_link_group(uint8_t *mix_mac, int count, mesh_link_data_t *link_data)
{
    static int flag = MESH_DATA_P2P;
    static mesh_data_t mesh_data;
    int res;  
    mesh_opt_t opt;
    opt.type = MESH_OPT_SEND_GROUP;
    opt.len = count * 6;
    opt.val = mix_mac;
    mesh_data.data = link_data->data;
    mesh_data.size = link_data->len;

    send_lock();
    res = esp_mesh_send((mesh_addr_t *)WIFI_MESH_MULTICAST_ADDR, &mesh_data, flag, &opt,  1);
    send_unlock();

    switch(res)
    {
        case ESP_OK:
            STD_LOGD("group[%d]: success", link_data->len);
            break;

        default:
            STD_LOGE("group error: %s", esp_err_to_name(res));
            break;
    }
    return 0;
}



static int scan_ap_channel(char *ssid)
{
    int channel;
    wifi_scan_config_t config = {0};
    config.ssid = (uint8_t *)ssid;
    ESP_ERROR_CHECK(esp_wifi_scan_start(&config, true));

    uint16_t ap_count;
    ESP_ERROR_CHECK(esp_wifi_scan_get_ap_num(&ap_count));
    if (ap_count == 0) {
        STD_LOGW("AP: %s no found", ssid);
        channel = -1;
    } 
    else
    {
        wifi_ap_record_t *ap_list = (wifi_ap_record_t *)STD_CALLOC(sizeof(wifi_ap_record_t) * ap_count, ap_count);
        ESP_ERROR_CHECK(esp_wifi_scan_get_ap_records(&ap_count, ap_list));
        channel = ap_list[0].primary;
        STD_LOGI("[ap]%s, [rssi]%d", ap_list[0].ssid, ap_list[0].rssi);
        STD_FREE(ap_list);
    }

    esp_wifi_scan_stop();

    STD_LOGD("got channel: %d", channel);
    return channel;
}



void mesh_link_deinit()
{
    esp_mesh_stop();
    esp_mesh_deinit();
}


void mesh_link_init(mesh_event_cb_t mesh_cb, int node_type)
{
    send_lock_init();
    std_wifi_init();
    std_wifi_restore();
    tcpip_adapter_dhcps_stop(TCPIP_ADAPTER_IF_AP);
    tcpip_adapter_dhcpc_stop(TCPIP_ADAPTER_IF_STA);

    ESP_ERROR_CHECK(esp_mesh_init());
    ESP_ERROR_CHECK(esp_mesh_set_max_layer(MESH_MAX_LAYER));
    ESP_ERROR_CHECK(esp_mesh_set_vote_percentage(1));
    ESP_ERROR_CHECK(esp_mesh_set_ap_assoc_expire(MESH_HEART_TIME_S));

    network_info_t info;
    STD_ASSERT(std_nvs_load(KEY_NETWORK, &info, sizeof(info)) == 0);

    mesh_cfg_t cfg = MESH_INIT_CONFIG_DEFAULT();
    str2mac(cfg.mesh_id.addr, info.mesh_id);

    if(strcmp(info.ssid, "NOROUTER") != 0)
    {
        strcpy((char *)cfg.router.ssid, info.ssid);
        cfg.router.ssid_len = strlen(info.ssid);
        strcpy((char *)cfg.router.password, info.passwd);
    }

    cfg.event_cb = mesh_cb;
    int channel = DEFAULT_CHANNEL;
    switch(node_type)
    {
        case MESH_MASTER:
            esp_mesh_fix_root(true);
            esp_mesh_set_type(MESH_ROOT);
            STD_LOGI("fix device to master");
            channel = DEFAULT_CHANNEL;
            break;

        case MESH_SLAVE:
            esp_mesh_fix_root(true);
             esp_mesh_set_type(MESH_NODE);
            STD_LOGI("fix device to slave");
            channel = DEFAULT_CHANNEL;
            break;           
        
        case MESH_AUTO:
            for(;;)
            {
                channel = scan_ap_channel(info.ssid);
                if(channel > 0)
                    break; 
            }
            break;

        default:
            STD_ASSERT(-1);
            break;
    }
    cfg.channel = channel;

    ESP_ERROR_CHECK(esp_mesh_set_ap_authmode(MESH_AP_AUTHMODE));
    cfg.mesh_ap.max_connection = MESH_MAX_CHILD_NODE;

    strcpy((char *)cfg.mesh_ap.password, "mcb_passwd");

    ESP_ERROR_CHECK(esp_mesh_set_config(&cfg));


    
    ESP_ERROR_CHECK(esp_mesh_start());

    STD_LOGI("mesh link init success");

}




#ifdef PRODUCT_TEST

// bool task_root = false; //FIXME:
// bool task_node = false;

// void target_send2root(uint8_t *mac, char *arg)
// {
//     static mesh_link_data_t mesh_link_data = {
//         .data = (uint8_t *)"send2root data",
//         .len = strlen("send2root data") + 1,
//     };
//     mesh_link_send2root(&mesh_link_data);
// }
// static target_func_t target_func_send2root = {
//     .cmd = "send2root",
//     .func = target_send2root,
// };

// void target_send2node(uint8_t *mac, char *arg)
// {
//     char *mac_str;
//     static mesh_addr_t addr;
//     json_parse_object(arg, "mac", &mac_str);
//     str2mac((uint8_t *)&addr, mac_str);

//     static mesh_link_data_t mesh_link_data = {
//         .data = (uint8_t *)"send2node data",
//         .len = strlen("send2node data") + 1,
//     };
//     mesh_link_send2node(&addr, &mesh_link_data);

//     STD_FREE(mac_str);
// }
// static target_func_t target_func_send2node= {
//     .cmd = "send2node",
//     .func = target_send2node,
// };

// void target_broadcast(uint8_t *mac, char *arg)
// {
//     static mesh_link_data_t mesh_link_data = {
//         .data = (uint8_t *)"broadcast data",
//         .len = strlen("broadcast data") + 1,
//     };
//     mesh_link_broadcast(&mesh_link_data);

// }
// static target_func_t target_func_broadcast = {
//     .cmd = "broadcast",
//     .func = target_broadcast,
// };


// void target_group(uint8_t *mac, char *arg)
// {
    
//     static mesh_link_data_t mesh_link_data = {
//         .data = (uint8_t *)"group data",
//         .len = strlen("group data") + 1,
//     };
//     static uint8_t mix_mac[] = {
//         0x30, 0xae, 0xa4, 0x80,  0x4c, 0x7c,
//         0x30, 0xae, 0xa4, 0x80,  0x04, 0x7c,
//         0x30, 0xae, 0xa4, 0x80,  0x0d, 0xf8,
//         0x30, 0xae, 0xa4, 0x80,  0x52, 0xb0,
//     };

//     mesh_link_group(mix_mac, 4, &mesh_link_data);

// }
// static target_func_t target_func_group = {
//     .cmd = "group",
//     .func = target_group,
// };


// void mesh_link_test_init()
// {
//     register_target_func(&target_func_send2root);
//     register_target_func(&target_func_send2node);
//     register_target_func(&target_func_broadcast);
//     register_target_func(&target_func_group);
// }

// void mesh_link_root_receive_task(void *args)
// {
//     mesh_addr_t src;
//     static uint8_t data[MESH_MPS];
//     mesh_link_data_t mesh_link_data;
//     mesh_link_data.data = data;
//     while(1)
//     {
//         mesh_link_data.len = MESH_MPS;
//         if(mesh_link_root_receive(&src, &mesh_link_data, 4000/portTICK_PERIOD_MS) == 0)
//         {
//             STD_LOGM("root receive[%d]: %s",mesh_link_data.len, mesh_link_data.data);
//         }
        
//         if(!task_root)
//         {
//             vTaskDelete(NULL);
//         }
//     }
// }

// void mesh_link_node_receive_task(void *args)
// {
//     mesh_addr_t src;
//     static uint8_t data[MESH_MPS];
//     mesh_link_data_t mesh_link_data;
//     mesh_link_data.data = data;
//     while(1)
//     {
//         mesh_link_data.len = MESH_MPS;
//         if(mesh_link_node_receive(&src, &mesh_link_data, 4000/portTICK_PERIOD_MS) == 0)
//         {
//             STD_LOGM("node receive[%d]: %s", mesh_link_data.len, mesh_link_data.data);
//         }
//     }
// }

#endif