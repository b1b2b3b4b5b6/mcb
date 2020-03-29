#include "packet_ota.h"

#define STD_LOCAL_LOG_LEVEL STD_LOG_DEBUG
#define HANDLE_TASK_SIZE 2048
#define HANDLE_TASK_PRI ESP_TASK_MAIN_PRIO + 1

#define SEND_RETRY 3
#define SEND_TIME_MS 5000

static esp_ota_handle_t update_handle = 0;

static out_data_t *request_cut(int seq)
{
    mesh_empty_out(OUT_BIN);
    char json[100] = {0};
    char *cmd = "cut_ota";
    json_pack_object(json, "Typ", &cmd);
    json_pack_object(json, "Seq", &seq);
    out_data_t *out = build_send_out_data(OUT_STR, (uint8_t *)json, strlen(json) + 1);
    mesh_send_out(out);
    free_out_data(out);
    out = mesh_wait_out(OUT_BIN, SEND_TIME_MS);
    if(out == NULL)
        STD_LOGE("requset cut[%d] timeout", seq);
    else
        STD_LOGD("request cut[%d] len[%d] success", seq, out->data_len);
    return out;
}

static char *get_version()
{
    out_data_t *out = request_cut(0);
    if(out == NULL)
        return 0;
    static esp_app_desc_t new_app_info;
    memcpy(&new_app_info, &out->data[sizeof(esp_image_header_t) + sizeof(esp_image_segment_header_t)], sizeof(esp_app_desc_t));
    free_out_data(out);
    if(strcmp("app", new_app_info.project_name) != 0)
    {
        STD_LOGE("ota bin name illegal");
        return NULL;            
    }

    STD_LOGD("ota bin new version[%s]", new_app_info.version);
    return new_app_info.version;
}

int packet_ota_prepare()
{
    int res;
    char *new_verion = get_version();
    if(new_verion == NULL)
        return -1;

    if(strcmp(std_ota_update_version(), new_verion) == 0)
    {
        res = 0;
        STD_LOGD("update version same");
    }
    else
    {
        esp_ota_begin(std_ota_update_partition(), OTA_SIZE_UNKNOWN, &update_handle);

        int seq = 0;
        int bin_len = 0;
        out_data_t *out;
        for(;;)
        {
            res = STD_RETRY_TRUE_BREAK((out = request_cut(seq)) != NULL, SEND_RETRY);
            if(res == -1)
            {
                STD_LOGE("root ota fail, read bin error");
                break;
            }

            if(out->data_len == 0)
            {
                STD_LOGI("root bin read finished[%d]", bin_len);
                free_out_data(out);
                break;
            }

            ESP_ERROR_CHECK(esp_ota_write(update_handle, (const void *)out->data, out->data_len));
            bin_len += out->data_len;
            seq++;
            STD_LOGD("write seq[%d]", seq);
            free_out_data(out);
        }
        int res = esp_ota_end(update_handle);
        STD_ERROR_GOTO_FAIL(res);
        std_ota_end(bin_len, new_verion);
    }

FAIL:
    if(res == 0)
        STD_LOGI("packet ota success");
    else
        STD_LOGE("packet ota fail");
    return res;
}
