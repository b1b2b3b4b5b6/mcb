#include "network.h"

#define STD_LOCAL_LOG_LEVEL STD_LOG_DEBUG

static bool check_network()
{
    return std_nvs_is_exist(KEY_NETWORK);
}

void network_erase()
{
    std_nvs_erase(KEY_NETWORK);
    STD_LOGW("erase all network info");
}

void network_init()
{
    if(check_network())
    {
        STD_LOGI("network config exited");
        return;
    }
    config_network_init();
    std_blufi_init(config_blufi_cb);
    chain_start(config_chain_cb);
    config_network_wait();
    std_blufi_deinit();
    chain_stop();
    auto_send();
    config_network_deinit();
    STD_LOGI("network init success");
}
