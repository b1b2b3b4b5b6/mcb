#include "mesh_ota_protocol.h"

#define STD_LOCAL_LOG_LEVEL STD_LOG_DEBUG
#define BIN_TASK_PRI ESP_TASK_MAIN_PRIO + 1
#define BIN_TASK_SIZE 2048

#define MAX_CHECK_BITS 1440
#define OTA_BIN_CUT_SIZE 1440

#define SEND_WAIT_TIME_MS 3000
#define SEND_RETRY 3

#define WIAT_CHECK_TIME_MS 5000
#define OTA_WRITE_WAIT_MS  3000

#define OTA_WAIT_STABLE_TIME_S 3
#define OTA_REBOOT_TIME_S 5


typedef enum ota_cmd_type_t{
    OTA_START = 1,
    OTA_START_ACK,
    OTA_CHECK,
    OTA_CHECK_ACK,
    OTA_END,
    OTA_END_ACK,
    OTA_REBOOT,
}ota_cmd_type_t;

typedef enum ota_flag_type_t{
    OTA_FREE = 0,
    OTA_RUNNING,
    OTA_ENDING,
}ota_flag_type_t;

typedef struct ota_progress_t{
    uint8_t bin_status[MAX_CHECK_BITS];
    char version[10];
    int status;
    int bin_len;
    int cut_count;
}__attribute__((packed)) ota_progress_t;

typedef struct ota_bin_cut_t{
    int seq;
    uint8_t packet[OTA_BIN_CUT_SIZE];
}__attribute__((packed)) ota_bin_cut_t;

static ota_progress_t g_ota_progress;
static esp_ota_handle_t update_handle = 0;
static int spread_peroid_ms;

static void ota_set_type(char *json, int type)
{
    json_pack_object(json, "type", &type);
}

static void ota_start_handle(uint8_t *mac, char *data)
{
    bool reset_flag = false;
    int bin_len;
    char *version;
    int res;
    json_parse_object(data, "version", &version);
    res = std_ota_check_version(version);
    if(res != 0)
    {
        STD_LOGI("same version , ignore");
        goto FAIL;
    }
    STD_ERROR_GOTO_FAIL(res);
    STD_LOGI("ota version: %s", version);
    strcpy(g_ota_progress.version, version);
    g_ota_progress.version[strlen(version)] = 0;
        
    json_parse_object(data, "bin_len", &bin_len);
    STD_LOGI("ota bin len: %d", bin_len);

    switch(g_ota_progress.status)
    {
        case OTA_FREE:
            reset_flag = true;
            break;

        case OTA_RUNNING:
            if(memcmp(version, g_ota_progress.version, strlen(version)) != 0)
                reset_flag  = true;
            break;

        case OTA_ENDING:
            if(memcmp(version, g_ota_progress.version, strlen(version)) != 0)
                reset_flag  = true;
            break;

        default:
            STD_END("never be here");
            break;
    }

    if(reset_flag)
    {
        memset(&g_ota_progress, 0, sizeof(g_ota_progress));
        g_ota_progress.status = OTA_RUNNING;
        g_ota_progress.bin_len = bin_len;
        g_ota_progress.cut_count = bin_len / OTA_BIN_CUT_SIZE + 1;
        strcpy(g_ota_progress.version, version);
        esp_ota_begin(std_ota_update_partition(), OTA_SIZE_UNKNOWN, &update_handle);
        STD_LOGI("ota progress reset");
        set_self_ota(true);
    }
    else
        STD_LOGI("ota progress continue");
FAIL:
    json_free(version, 0);
}

static void ota_start_ack_handle(uint8_t *mac, char *data)
{
}

static void ota_check_handle(uint8_t *mac, char *data)
{
    base_shell_t *packet_shell;
    packet_shell = build_base_shell(SHELL_CHECK_ACK, 1, MAX_CHECK_BITS);
    memcpy(packet_shell->mix_mac, mac, 6);
    if(g_ota_progress.status == OTA_RUNNING)
        memcpy(packet_shell->data, g_ota_progress.bin_status, g_ota_progress.cut_count);
    else
        memset(packet_shell->data, true, MAX_CHECK_BITS);
    STD_RETRY_TRUE_BREAK(mesh_link_send(packet_shell, SEND_WAIT_TIME_MS) == 0, SEND_RETRY);
    free_base_shell(packet_shell);
}

static void ota_check_ack_handle(uint8_t *mac, char *data)
{

}

static void ota_end_handle(uint8_t *mac, char *data)
{
    esp_ota_end(update_handle);
    if(g_ota_progress.status != OTA_RUNNING)
        return;
        
    if(std_ota_check_update() != 0)
    {
        STD_LOGE("std ota end fail");
    }
    else
    {
        g_ota_progress.status = OTA_ENDING;
        STD_LOGI("ota end success");
        std_ota_reboot(-1);
    }
}

static void ota_end_ack_handle(uint8_t *mac, char *data)
{
}

static void ota_rebooot_handle(uint8_t *mac, char *data)
{
    if(g_ota_progress.status == OTA_ENDING)
    {
        int time_s;
        json_parse_object(data, "time_s", &time_s);
        std_ota_reboot(time_s * 1000);
    }
}
static void ota_cmd_prase(uint8_t *mac, char *data)
{
    int type;
    json_parse_object(data, "type", &type);
    if(esp_mesh_is_root())
    {
        switch(type)
        {
            case OTA_START_ACK:
                ota_start_ack_handle(mac, data);
                break;

            case OTA_CHECK_ACK:
                ota_check_ack_handle(mac, data);
                break;

            case OTA_END_ACK:
                ota_end_ack_handle(mac, data);
                break;
            
            default:
                break;
        }
    }
    else
    {
        switch(type)
        {
            case OTA_START:
                ota_start_handle(mac, data);
                break;

            case OTA_CHECK:
                ota_check_handle(mac, data);
                break;

            case OTA_END:
                ota_end_handle(mac, data);
                break;

            case OTA_REBOOT:
                ota_rebooot_handle(mac, data);
                break;
            
            default:
                break;
        }       
    }

}

static void ota_bin_cut_handle(ota_bin_cut_t *ota_cut)
{
    int res;
    if(g_ota_progress.bin_status[ota_cut->seq] == false)
    {
        STD_LOGD("ota wirte[%d]", ota_cut->seq);
        res = esp_partition_write(std_ota_update_partition(), ota_cut->seq * OTA_BIN_CUT_SIZE, ota_cut->packet, OTA_BIN_CUT_SIZE);
        g_ota_progress.bin_status[ota_cut->seq] = true;
        STD_ASSERT(res == 0);
    }   
    else
        STD_LOGD("ota ingore[%d]", ota_cut->seq);
}

static void ota_bin_recv_task(void *argc)
{
    shell_receive_t *shell_receive;
    for(;;)
    {
        shell_receive = mesh_link_wait_shell(SHELL_OTA, 10000);

        if(shell_receive == NULL)
            continue;
        
        if(!esp_mesh_is_root() && g_ota_progress.status == OTA_RUNNING)
            ota_bin_cut_handle((ota_bin_cut_t *)shell_receive->data);    
        free_shell_receive(shell_receive);
    }
}

static void ota_set_start(char *data)
{
    ota_set_type(data, OTA_START);
    char *version = g_ota_progress.version;
    json_pack_object(data, "version", &version);
    json_pack_object(data, "bin_len", &g_ota_progress.bin_len);
}

static void ota_set_check(char *data)
{
    ota_set_type(data, OTA_CHECK);
}

static void ota_set_end(char *data)
{
    ota_set_type(data, OTA_END);
}

static void ota_set_reboot(char *data, int time_s)
{
    ota_set_type(data, OTA_REBOOT);
    json_pack_object(data, "time_s", &time_s);
}

static int ota_start(uint8_t *mac)
{
    mesh_cmd_shell_t *start_shell = NULL;
    int res;

    STD_LOGD("start mac: "MACSTR, MAC2STR(mac));
    start_shell = build_mesh_cmd_shell("ota", 1, 200);
    memcpy(start_shell->base_shell->mix_mac, mac, 6);
    ota_set_start((char *)start_shell->json);
    res = STD_RETRY_TRUE_BREAK(mesh_cmd_send(start_shell, SEND_WAIT_TIME_MS) == 0, SEND_RETRY);
    STD_ERROR_GOTO_FAIL(res);

FAIL:
    free_mesh_cmd_shell(start_shell);
    if(res == 0)
        STD_LOGD("ota start success");
    else
        STD_LOGE("ota start fail");
    return res;
}

static int mesh_ota_sup(uint8_t *mac, bool *bin_status)
{
    int res = 0;
    ota_bin_cut_t *ota_cut;
    base_shell_t *cut_shell;

    if(mac == NULL)
    {
        cut_shell = build_base_shell(SHELL_OTA, 0, sizeof(ota_bin_cut_t)); 
        cut_shell->prefix->spread_quality = SPREAD_0;
        for(int n = 0; n < g_ota_progress.cut_count; n++)
        {
            
            ota_cut = (ota_bin_cut_t *)cut_shell->data;
            ota_cut->seq = n;
            ESP_ERROR_CHECK(esp_partition_read(std_ota_update_partition(), n * OTA_BIN_CUT_SIZE, ota_cut->packet, OTA_BIN_CUT_SIZE));
            mesh_link_send(cut_shell, portMAX_DELAY);
            vTaskDelay(spread_peroid_ms / portTICK_PERIOD_MS);
            STD_LOGD("spread[%d], total[%d]", n, g_ota_progress.cut_count);
            if(n < 2)
                vTaskDelay(3000 / portTICK_PERIOD_MS);
        }
        
    }
         
    else
    {
        cut_shell = build_base_shell(SHELL_OTA, 1, sizeof(ota_bin_cut_t));
        memcpy(cut_shell->mix_mac, mac, 6);
        for(int n = 0; n < g_ota_progress.cut_count; n++)
        {
            if(bin_status[n])
                continue;
            ota_cut = (ota_bin_cut_t *)cut_shell->data;
            ota_cut->seq = n;
            ESP_ERROR_CHECK(esp_partition_read(std_ota_update_partition(), n * OTA_BIN_CUT_SIZE, ota_cut->packet, OTA_BIN_CUT_SIZE));
            STD_RETRY_TRUE_BREAK(mesh_link_send(cut_shell, OTA_WRITE_WAIT_MS) == 0, SEND_RETRY);
        }
    }

    free_base_shell(cut_shell);
    if(res != 0)
        STD_LOGE("ota sub fail");
    else
        STD_LOGD("ota sub success");
    return res;
}

static int ota_end(uint8_t *mac)
{
    int res;

    mesh_cmd_shell_t *end_shell = build_mesh_cmd_shell("ota", 1, 200);
    memcpy(end_shell->base_shell->mix_mac, mac, 6);
    ota_set_end((char *)end_shell->json);
    res = STD_RETRY_TRUE_BREAK(mesh_cmd_send(end_shell, SEND_WAIT_TIME_MS) == 0, SEND_RETRY);
    STD_ERROR_GOTO_FAIL(res);

FAIL:
    free_mesh_cmd_shell(end_shell);
    if(res != 0)
        STD_LOGE("ota send end fail");
    else
        STD_LOGD("ota send end success");

    return res;
}


static int ota_singal(uint8_t *mac)
{
    int res;
    static bool bin_status[MAX_CHECK_BITS] = {0};
    mesh_cmd_shell_t *check_shell = NULL;

    STD_LOGD("singal ota mac "MACSTR, MAC2STR(mac));

    mesh_link_empty_shell(SHELL_CHECK_ACK);
    check_shell = build_mesh_cmd_shell("ota", 1, 200);
    memcpy(check_shell->base_shell->mix_mac, mac, 6);
    ota_set_check(check_shell->json);
    res = STD_RETRY_TRUE_BREAK(mesh_cmd_send(check_shell, SEND_WAIT_TIME_MS) == 0, SEND_RETRY);
    STD_ERROR_GOTO_FAIL(res);
    shell_receive_t *shell_receive = mesh_link_wait_shell(SHELL_CHECK_ACK, WIAT_CHECK_TIME_MS);
    if(shell_receive == NULL)
        res = -1;
    else
        res = 0;
    STD_CHECK_GOTO_FAIL(shell_receive != NULL);
    memcpy(bin_status, shell_receive->data, MAX_CHECK_BITS);
    free_shell_receive(shell_receive);

    res = mesh_ota_sup(mac, bin_status);
    STD_ERROR_GOTO_FAIL(res);

    res = STD_RETRY_TRUE_BREAK(ota_end(mac) == 0, SEND_RETRY);
    STD_ERROR_GOTO_FAIL(res);

FAIL:
    free_mesh_cmd_shell(check_shell);

    if(res != 0)
        STD_LOGE("ota singal fail");
    else
        STD_LOGD("ota singal succcess");

    return res;
}

static int ota_reboot(uint8_t *mac, int time_s)
{
    int res;
    mesh_cmd_shell_t *reboot_shell;
    STD_LOGD("reboot mac: "MACSTR, MAC2STR(mac));
    reboot_shell = build_mesh_cmd_shell("ota", 1, 200);
    memcpy(reboot_shell->base_shell->mix_mac, mac, 6);
    ota_set_reboot((char *)reboot_shell->json, time_s);

    res = STD_RETRY_TRUE_BREAK(mesh_cmd_send(reboot_shell, SEND_WAIT_TIME_MS) == 0, SEND_RETRY);
    STD_ERROR_GOTO_FAIL(res);
FAIL:
    free_mesh_cmd_shell(reboot_shell);
    if(res == 0)
        STD_LOGD("ota reboot send success");
    else
        STD_LOGE("ota reboot send fail");
    return res;
}

int mesh_ota_prepare(char *app_url, int is_http)
{
    int res;
    if(is_http)
        res = http_ota_prepare(app_url);
    else
        res = packet_ota_prepare();
    
    if(res == 0)
    {
        memset(&g_ota_progress, 0, sizeof(g_ota_progress));
        g_ota_progress.bin_len = std_ota_update_len();
        g_ota_progress.cut_count = g_ota_progress.bin_len / OTA_BIN_CUT_SIZE + 1;
        strcpy(g_ota_progress.version, std_ota_update_version());
        STD_LOGI("set bin_len[%d] cut_count[%d] version[%s]", g_ota_progress.bin_len, g_ota_progress.cut_count, g_ota_progress.version);
        STD_LOGD("mesh ota prepare success");
    }
    else
        STD_LOGE("mesh ota prepare fail");
    return res;
}  

static void mesh_ota_self()
{
    STD_LOGI("ota self start");
    if(std_ota_check_version(g_ota_progress.version) == 0)
    {
        STD_LOGI("version ok, reboot");
        std_ota_reboot(0);
    }
}

void mesh_ota_temp()
{
    int res;
    bool ota_self = false;
    char **target_mac = NULL;
    int mac_len = 0;;

    spread_peroid_ms = 500;
    STD_LOGI("ota spread peroid_ms[%d]", spread_peroid_ms);

    memset(&g_ota_progress, 0, sizeof(g_ota_progress));
    g_ota_progress.bin_len = std_ota_update_len();
    g_ota_progress.cut_count = g_ota_progress.bin_len / OTA_BIN_CUT_SIZE + 1;
    strcpy(g_ota_progress.version, std_ota_update_version());
    STD_LOGI("set bin_len[%d] cut_count[%d] version[%s]", g_ota_progress.bin_len, g_ota_progress.cut_count, g_ota_progress.version);

    STD_LOGI("ota mac num: %d", mac_len);

    if(mac_len >= 1)
    {
        for(int n = 0; n < mac_len; n++)
        {
            uint8_t mac[6];
            STD_ASSERT(str2mac(mac, target_mac[n]) != NULL);
            if(std_wifi_is_self((mesh_addr_t *)mac))
                ota_self = true;
            else
            {
                res = ota_start(mac);
                STD_ERROR_CONTIUNE(res);
                res = ota_singal(mac);
                STD_ERROR_CONTIUNE(res);
            }
        }
        for(int n = 0; n < mac_len; n++)
        {
            uint8_t mac[6];
            STD_ASSERT(str2mac(mac, target_mac[n]) != NULL);
            if(std_wifi_is_self((mesh_addr_t *)mac))
                ota_self = true;
            else
            {
                res = ota_reboot(mac, topo_calcu_layer(mac_len) * OTA_REBOOT_TIME_S);
                STD_ERROR_CONTIUNE(res);
            }
        }
    }
    else
    {
        res = 0;
        ota_self = true;
        mac_list_t *mac_list = topo_get_other_list();
        STD_CHECK_GOTO_FAIL(mac_list->num > 0);

        for(int n = 0; n < mac_list->num; n++)
        {
            STD_LOGI("ota start count[%d] total[%d]", n, mac_list->num);
            ota_start((uint8_t *)&mac_list->macs[n]);
        }

        res = mesh_ota_sup(NULL, NULL);
        await_spread_empty();
        STD_LOGI("wait mesh stable ,time_s[%d]",topo_calcu_layer(mac_list->num) * OTA_WAIT_STABLE_TIME_S);
        vTaskDelay(topo_calcu_layer(mac_list->num) * OTA_WAIT_STABLE_TIME_S * 1000/ portTICK_PERIOD_MS);

        for(int n = 0; n < mac_list->num; n++)
        {
            STD_LOGI("ota singal count[%d] total[%d]", n, mac_list->num);
            ota_singal((uint8_t *)&mac_list->macs[n]);
        }
            
        for(int n = 0; n < mac_list->num; n++)
        {

            STD_LOGI("ota reboot count[%d] total[%d]", n, mac_list->num);
            ota_reboot((uint8_t *)&mac_list->macs[n], topo_calcu_layer(mac_list->num) * OTA_REBOOT_TIME_S);
        }

        free_mac_list(mac_list);
    }

FAIL:
    json_free(target_mac, mac_len);
    if(ota_self)
        mesh_ota_self();

    if(res == 0)
        STD_LOGI("ota exec success");
    else
        STD_LOGE("ota exec fail");

}

void mesh_ota_exec(char *data)
{
    int res;
    bool ota_self = false;
    char **target_mac = NULL;
    int mac_len = json_parse_object(data, "Devcies", &target_mac);

    char *cus_data = NULL;
    json_parse_object(data, "CusData", &cus_data);
    STD_ASSERT(cus_data != NULL);

    char *app_url = NULL;
    json_parse_object(cus_data, "AppUrl", &app_url);
    STD_ASSERT(app_url != NULL);

    STD_ASSERT(json_parse_object(cus_data, "PeroidMs", &spread_peroid_ms) == 0);
    STD_LOGI("ota spread peroid_ms[%d]", spread_peroid_ms);

    int is_http = 1;
    json_parse_object(cus_data, "IsHttp", &is_http);

    res = mesh_ota_prepare(app_url, is_http);
    json_free(cus_data, 0);
    json_free(app_url, 0);
    STD_ERROR_GOTO_FAIL(res);
    STD_LOGI("ota mac num: %d", mac_len);
    
    if(mac_len >= 1)
    {
        for(int n = 0; n < mac_len; n++)
        {
            uint8_t mac[6];
            STD_ASSERT(str2mac(mac, target_mac[n]) != NULL);
            if(std_wifi_is_self((mesh_addr_t *)mac))
                ota_self = true;
            else
            {
                res = ota_start(mac);
                STD_ERROR_CONTIUNE(res);
                res = ota_singal(mac);
                STD_ERROR_CONTIUNE(res);
            }
        }
        for(int n = 0; n < mac_len; n++)
        {
            uint8_t mac[6];
            STD_ASSERT(str2mac(mac, target_mac[n]) != NULL);
            if(std_wifi_is_self((mesh_addr_t *)mac))
                ota_self = true;
            else
            {
                res = ota_reboot(mac, topo_calcu_layer(mac_len) * OTA_REBOOT_TIME_S);
                STD_ERROR_CONTIUNE(res);
            }
        }
    }
    else
    {
        res = 0;
        ota_self = true;
        mac_list_t *mac_list = topo_get_other_list();
        STD_CHECK_GOTO_FAIL(mac_list->num > 0);

        for(int n = 0; n < mac_list->num; n++)
        {
            STD_LOGI("ota start count[%d] total[%d]", n, mac_list->num);
            ota_start((uint8_t *)&mac_list->macs[n]);
        }

        res = mesh_ota_sup(NULL, NULL);
        await_spread_empty();
        STD_LOGI("wait mesh stable ,time_s[%d]",topo_calcu_layer(mac_list->num) * OTA_WAIT_STABLE_TIME_S);
        vTaskDelay(topo_calcu_layer(mac_list->num) * OTA_WAIT_STABLE_TIME_S * 1000/ portTICK_PERIOD_MS);

        for(int n = 0; n < mac_list->num; n++)
        {
            STD_LOGI("ota singal count[%d] total[%d]", n, mac_list->num);
            ota_singal((uint8_t *)&mac_list->macs[n]);
        }
            
        for(int n = 0; n < mac_list->num; n++)
        {

            STD_LOGI("ota reboot count[%d] total[%d]", n, mac_list->num);
            ota_reboot((uint8_t *)&mac_list->macs[n], topo_calcu_layer(mac_list->num) * OTA_REBOOT_TIME_S);
        }

        free_mac_list(mac_list);
    }

FAIL:
    json_free(target_mac, mac_len);
    if(ota_self)
        mesh_ota_self();

    if(res == 0)
        STD_LOGI("ota exec success");
    else
        STD_LOGE("ota exec fail");

}

static mesh_out_cmd_handle_t ota_exec_handle = {
    .cmd = "Ota",
    .func = mesh_ota_exec,
};

static mesh_cmd_handle_t ota_cmd_handle = {
    .cmd = "ota",
    .func = ota_cmd_prase,
};

void mesh_ota_protocol_init()
{
    mesh_cmd_handle_add(&ota_cmd_handle);
    mesh_out_cmd_handle_add(&ota_exec_handle);
    mesh_link_shell_add(SHELL_OTA, 5);
    mesh_link_shell_add(SHELL_CHECK_ACK, 5);
    xTaskCreate(ota_bin_recv_task, "bin recv task", BIN_TASK_SIZE, NULL, BIN_TASK_PRI, NULL);
}


