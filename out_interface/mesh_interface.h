#ifndef MESH_INTERFACE_H
#define MESH_INTERFACE_H

#include "std_port_common.h"
#include "interface_protocol.h"
#include "interface_udp.h"
#include "interface_mqtt.h"
#include "interface_tcp.h"
#include "std_type.h"

typedef enum out_data_type_t{
    OUT_BIN = 0,
    OUT_STR = 1,
    MAX_RECV_TYPE,
}out_data_type_t;

typedef struct out_send_prefix_t {
    uint16_t len;
    uint8_t mesh_id[6];
    uint8_t type;
}__attribute__((__packed__)) out_send_prefix_t;

typedef struct out_recv_prefix_t{
    uint16_t len;
    uint8_t reserve[6];
    uint8_t type;
}__attribute__((__packed__)) out_recv_prefix_t;

typedef union out_data_prefix_t{
    out_send_prefix_t send;
    out_recv_prefix_t recv;
}__attribute__((__packed__)) out_data_prefix_t;

typedef struct out_data_t{
    out_data_prefix_t *prefix;
    uint8_t *raw_data;
    int raw_data_len;
    uint8_t *data;
    int data_len;
}__attribute__((__packed__)) out_data_t;

void mesh_interface_init();
void mesh_interface_add_type(uint8_t type, uint8_t size);

int mesh_send_out(out_data_t *out);
out_data_t *mesh_wait_out(uint8_t type, int time_ms);
void mesh_empty_out(uint8_t type);

void fill_out_data(out_data_t *out, uint8_t *raw_data, int raw_data_len);
out_data_t *build_recv_out_data(uint8_t *data, int len);
out_data_t *build_send_out_data(uint8_t type, uint8_t *data, int len);
void print_out_data(out_data_t *out);
void free_out_data(out_data_t *recv);

#endif