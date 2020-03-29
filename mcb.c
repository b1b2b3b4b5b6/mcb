#include "mcb.h"

#define STD_LOCAL_LOG_LEVEL STD_LOG_DEBUG

#define TASK_SIZE 2048
#define TASK_PRI ESP_TASK_MAIN_PRIO


static void turn2root()
{
    STD_LOGI("turn to root");
    tcpip_adapter_dhcpc_start(TCPIP_ADAPTER_IF_STA);
}

static void got_ip()
{
    if(esp_mesh_is_root())
    {
        set_router_is_connect(true);
    }
}

static void lost_ip()
{
    if(esp_mesh_is_root())
    {
        set_router_is_connect(false);
        esp_wifi_disconnect();
        esp_wifi_connect();
    }
}

static void turn2node()
{
    STD_LOGI("turn to node");
    tcpip_adapter_dhcpc_stop(TCPIP_ADAPTER_IF_STA);
}

void fix_root_handle()
{
    topo_set_root((mesh_addr_t *)std_wifi_get_stamac());
    topo_set_parent((mesh_addr_t *)std_wifi_get_stamac());
    turn2root();
    set_mesh_is_connect(true);
}

static void mesh_event_handler(mesh_event_t event)
{
    switch (event.id) {
    case MESH_EVENT_STARTED:
        if(esp_mesh_is_root())
            fix_root_handle();
        STD_LOGI("<MESH_EVENT_START>");
        break;
    case MESH_EVENT_STOPPED:
        STD_LOGI("<MESH_EVENT_STOPPED>");
        break;

    case MESH_EVENT_ROUTING_TABLE_ADD:
        STD_LOGI("<MESH_EVENT_ROUTING_TABLE_ADD>add %d, new:%d",
                 event.info.routing_table.rt_size_change,
                 event.info.routing_table.rt_size_new);
        break;

    case MESH_EVENT_ROUTING_TABLE_REMOVE:
        STD_LOGI("<MESH_EVENT_ROUTING_TABLE_REMOVE>remove %d, new:%d",
                 event.info.routing_table.rt_size_change,
                 event.info.routing_table.rt_size_new);
        break;

    case MESH_EVENT_CHILD_CONNECTED:
        topo_add_child(event.info.child_connected.aid, (mesh_addr_t *)event.info.child_connected.mac);
        STD_LOGI("<MESH_EVENT_CHILD_CONNECTED>aid:%d, "MACSTR"",
                    event.info.child_connected.aid,
                    MAC2STR(event.info.child_connected.mac));
        break;
    case MESH_EVENT_CHILD_DISCONNECTED:
        topo_remove_child(event.info.child_disconnected.aid);
        STD_LOGI("<MESH_EVENT_CHILD_DISCONNECTED>aid:%d, "MACSTR"",
                 event.info.child_disconnected.aid,
                 MAC2STR(event.info.child_disconnected.mac));
        break;

    case MESH_EVENT_NO_PARENT_FOUND:
        topo_set_parent(NULL);
        STD_LOGI("<MESH_EVENT_NO_PARENT_FOUND>scan times:%d",
                 event.info.no_parent.scan_times);
        /* TODO handler for the failure */
        break;

    case MESH_EVENT_PARENT_CONNECTED:
        STD_LOGI("<MESH_EVENT_PARENT_CONNECTED>layer: %d, parent mac: "MACSTR,
            event.info.connected.self_layer,
            MAC2STR(event.info.connected.connected.bssid));
        
        topo_set_layer(event.info.connected.self_layer);
        if (esp_mesh_is_root()) {
            turn2root();
            topo_set_parent((mesh_addr_t *)std_wifi_get_stamac());
            set_mesh_is_connect(true);
        } else {
            turn2node();
            mesh_addr_t temp = *(mesh_addr_t *)event.info.connected.connected.bssid;
            ADDR_AP2STA((uint8_t *)&temp);
            topo_set_parent(&temp);
            set_mesh_is_connect(true);
        }
        
        break;

    case MESH_EVENT_PARENT_DISCONNECTED:
        
        STD_LOGW("<MESH_EVENT_PARENT_DISCONNECTED>reason:%d", event.info.disconnected.reason);
        if(esp_mesh_is_root())
            set_router_is_connect(false);
        else
        {
            set_mesh_is_connect(false);
            topo_set_parent(NULL);
            topo_set_root(NULL);
        }
        break;

    case MESH_EVENT_LAYER_CHANGE:
        break;

    case MESH_EVENT_ROOT_ADDRESS:
        if(esp_mesh_is_root())
        {
            topo_set_root((mesh_addr_t *)std_wifi_get_stamac());
            STD_LOGI("<MESH_EVENT_ROOT_ADDRESS>root address:"MACSTR"", MAC2STR((uint8_t *)topo_get_root()));
        }
        else
        {
            
            mesh_addr_t temp = *(mesh_addr_t *)event.info.root_addr.addr;
            ADDR_AP2STA((uint8_t *)&temp);
            topo_set_root(&temp);
            STD_LOGI("<MESH_EVENT_ROOT_ADDRESS>root address:"MACSTR"", MAC2STR((uint8_t *)&temp));
        }
        break;

    case MESH_EVENT_ROOT_GOT_IP:
        /* root starts to connect to server */
        STD_LOGI("<MESH_EVENT_ROOT_GOT_IP>sta ip: " IPSTR ", mask: " IPSTR ", gw: " IPSTR,
                 IP2STR(&event.info.got_ip.ip_info.ip),
                 IP2STR(&event.info.got_ip.ip_info.netmask),
                 IP2STR(&event.info.got_ip.ip_info.gw));
        char ip_str[20] = {0};
        sprintf(ip_str, IPSTR, IP2STR(&event.info.got_ip.ip_info.ip));
        std_wifi_set_ip(ip_str);
        got_ip();
        break;
    case MESH_EVENT_ROOT_LOST_IP:
        lost_ip();
        STD_LOGW("<MESH_EVENT_ROOT_LOST_IP>");

        break;
    case MESH_EVENT_VOTE_STARTED:
        STD_LOGI("<MESH_EVENT_VOTE_STARTED>attempts:%d, reason:%d, rc_addr:"MACSTR"",
                 event.info.vote_started.attempts,
                 event.info.vote_started.reason,
                 MAC2STR(event.info.vote_started.rc_addr.addr));
        break;
    case MESH_EVENT_VOTE_STOPPED:
       STD_LOGI("<MESH_EVENT_VOTE_STOPPED>");
        break;
    case MESH_EVENT_ROOT_SWITCH_REQ:
        STD_LOGI("<MESH_EVENT_ROOT_SWITCH_REQ>reason:%d, rc_addr:"MACSTR"",
                 event.info.switch_req.reason,
                 MAC2STR( event.info.switch_req.rc_addr.addr));
        break;
    case MESH_EVENT_ROOT_SWITCH_ACK:
        break;

    case MESH_EVENT_TODS_STATE:
       STD_LOGI("<MESH_EVENT_TODS_REACHABLE>state:%d",
                 event.info.toDS_state);
        break;
    case MESH_EVENT_ROOT_FIXED:
       STD_LOGI("<MESH_EVENT_ROOT_FIXED>%s",
                 event.info.root_fixed.is_fixed ? "fixed" : "not fixed");
        break;
    case MESH_EVENT_ROOT_ASKED_YIELD:
        STD_LOGI("<MESH_EVENT_ROOT_ASKED_YIELD>"MACSTR", rssi:%d, capacity:%d",
                 MAC2STR(event.info.root_conflict.addr),
                 event.info.root_conflict.rssi,
                 event.info.root_conflict.capacity);
        break;
    case MESH_EVENT_CHANNEL_SWITCH:
       STD_LOGI("<MESH_EVENT_CHANNEL_SWITCH>new channel:%d", event.info.channel_switch.channel);
        break;
    case MESH_EVENT_SCAN_DONE:
       STD_LOGI("<MESH_EVENT_SCAN_DONE>number:%d",
                 event.info.scan_done.number);
        break;
    case MESH_EVENT_NETWORK_STATE:
       STD_LOGI("<MESH_EVENT_NETWORK_STATE>is_rootless:%d",
                 event.info.network_state.is_rootless);
        break;
    case MESH_EVENT_STOP_RECONNECTION:
       STD_LOGI("<MESH_EVENT_STOP_RECONNECTION>");
        break;
    case MESH_EVENT_FIND_NETWORK:
       STD_LOGI("<MESH_EVENT_FIND_NETWORK>new channel:%d, router BSSID:"MACSTR"",
                 event.info.find_network.channel, MAC2STR(event.info.find_network.router_bssid));
        break;
    case MESH_EVENT_ROUTER_SWITCH:
       STD_LOGI("<MESH_EVENT_ROUTER_SWITCH>new router:%s, channel:%d, "MACSTR"",
                 event.info.router_switch.ssid, event.info.router_switch.channel, MAC2STR(event.info.router_switch.bssid));
        break;
    default:
        break;
    }
}

static void mcb_task(void *argc)
{
    for(;;)
    {
        vTaskDelay(DEVICE_ONLINE_TIME_PEROID_S * 1000 / portTICK_PERIOD_MS);
        if(esp_mesh_is_root())
            mesh_report_online();
    }  
    
    vTaskDelete(NULL);
}



int mcb_init(int node_type)
{
    network_init();
    mesh_link_protocol_init(mesh_event_handler, node_type);
    mesh_interface_init();
    mesh_out_cmd_init();
    mesh_cmd_handle_init();
    mesh_func_init();
    mcb_debug_init();
    xTaskCreate(mcb_task, "mcb task", TASK_SIZE, NULL, TASK_PRI, NULL);
    vTaskDelay(3000 / portTICK_PERIOD_MS);//prevent mesh wifi trigger watch dog
    STD_LOGI("mcb init success");
    return 0;
}  