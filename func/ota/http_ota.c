#include "http_ota.h"

#define STD_LOCAL_LOG_LEVEL STD_LOG_DEBUG

#define HTTP_RETRY 3

int http_ota_prepare(char *app_url)
{
    std_ota_set_url(app_url);

    char *upstream_version = std_ota_upstream_version();
    if(upstream_version == NULL)
    {
        STD_LOGE("upstream version fail");
        return -1;
    }
    
    if(strcmp(std_ota_update_version(), upstream_version) == 0)
    {
        STD_LOGD("update version same");
        return 0;
    }
    else
        return STD_RETRY_TRUE_BREAK(std_ota_http_image() == 0, HTTP_RETRY);
}