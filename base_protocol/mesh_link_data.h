#ifndef MESH_LINK_DATA
#define MESH_LINK_DATA

#include "std_common.h"
#include "std_port_common.h"
#include "std_wifi.h"
#include "test_device.h"
#include "mesh_topo.h"

typedef enum spread_quality_t{
    SPREAD_0 = 0,
    SPREAD_100,
    SPREAD_200,
    SPREAD_500,
    SPREAD_1000,
    SPREAD_1500,
    SPREAD_3000,
    SPREAD_5000,
    SPREAD_MUL,
}spread_quality_t;

#define SPREAD_RETRY(spread_time, retrys) (spread_time + (retrys-1)*SPREAD_MUL )

typedef enum shell_type_t{
    SHELL_ACK = 0,
    SHELL_UPGATHER,
    SHELL_OTA,
    SHELL_CMD,
    SHELL_CHECK_ACK,
    SHELL_SEND_OUT,
    MAX_SHELL_TYPE
}shell_type_t;

typedef struct mesh_base_shell_prefix_t{
    uint8_t self_mac[6];
    bool response;
    uint8_t index;
    uint8_t type;
    uint8_t mac_len;
    uint8_t spread_quality;
}__attribute__((__packed__)) mesh_base_shell_prefix_t;

typedef struct  base_shell_t {
    mesh_base_shell_prefix_t *prefix;
    mesh_addr_t *mix_mac;
    uint8_t *raw_data;
    int raw_data_len;
    uint8_t *data;
    int data_len;
}__attribute__((__packed__)) base_shell_t;

typedef struct shell_receive_t{
    uint8_t *data;
    uint8_t mac[6];
    int len;
}shell_receive_t;

void fill_base_shell(base_shell_t *base_shell, uint8_t *raw_data, int raw_data_len);
base_shell_t *build_base_shell(uint8_t type, int mac_count, int data_len);
base_shell_t *copy_base_shell(base_shell_t *base_shell);
void print_base_shell(base_shell_t *shell);
void free_base_shell(base_shell_t *shell);

int get_max_mac_count(base_shell_t *shell);

shell_receive_t *build_shell_receive(base_shell_t *shell);
void free_shell_receive(shell_receive_t *shell_receive);

#endif
