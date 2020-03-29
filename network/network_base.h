#ifndef NETWORK_BASE_H
#define NETWORK_BASE_H

#include "std_common.h"

#include "std_wifi.h"
#include "std_queue.h"
#include "std_espnow.h"
#include "std_nvs.h"

#include "esp_bt.h"
#include "esp_blufi_api.h"
#include "esp_bt_defs.h"
#include "esp_gap_ble_api.h"
#include "esp_bt_main.h"
#include "esp_bt_device.h"

#define KEY_NETWORK "network_info"

typedef struct network_config_t {
    char *ssid;
    char *passwd;
    bool broadcast;
    char **ble_macs;
    int count;
    char *mesh_id;
}network_config_t;

typedef struct network_info_t{
    char ssid[32];
    char passwd[64];
    char mesh_id[20];
}network_info_t;

int config_network_send(network_config_t *network_config);
network_config_t *config_network_get();
void free_network_config(network_config_t *network_config);
void config_network_wait();
void config_network_init();
void config_network_deinit();
void config_chain_cb(uint8_t *data, int len);
void config_blufi_cb(uint8_t *data, int len);

#endif
