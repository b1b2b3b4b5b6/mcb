#include "mesh_func.h"

#define STD_LOCAL_LOG_LEVEL STD_LOG_DEBUG

#define SEND_RETRY 3
#define SEND_TIME_OUT_MS 3000

static void *custom_func(char *str)
{
    char **macs = NULL;

    int num = json_parse_object(str, "Devcies", &macs);
    STD_ASSERT(num != -1);
    char *cus_data = NULL;
    json_parse_object(str, "CusData", &cus_data);
    STD_ASSERT(cus_data != NULL);
    STD_LOGD("num[%d] CusData[%s]", num, cus_data);

    mesh_cmd_shell_t *cmd_shell = build_mesh_cmd_shell("temp", num, strlen(cus_data) + 1);
    cmd_shell->base_shell->prefix->spread_quality = SPREAD_RETRY(SPREAD_3000, 3);
    memcpy(cmd_shell->json, cus_data, strlen(cus_data) + 1);
    if(num >= 1)
    {
        for(int n = 0; n < num; n++)
        {
            uint8_t temp[6];
            str2mac(temp, macs[n]);
            memcpy(&cmd_shell->base_shell->mix_mac[n], temp, 6);
        }
            
    }
    int res = STD_RETRY_TRUE_BREAK(mesh_cmd_send(cmd_shell, SEND_TIME_OUT_MS) == 0, SEND_RETRY);
    free_mesh_cmd_shell(cmd_shell);
    if(res != 0)
        STD_LOGE("send cusdata fail");

    json_free(macs, num);
    json_free(cus_data, 0);
    return NULL;
}

static void penetrate_init()
{
    ADD_OUT_CMD("SendCustom", custom_func);
}

void mesh_func_init()
{
    penetrate_init();
    mesh_ota_protocol_init();
    mesh_report_init();
    device_config_init();
}