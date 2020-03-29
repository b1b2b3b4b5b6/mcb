#ifndef MESH_LINK_TRANS_H
#define MESH_LINK_TRANS_H


#include "network.h"
#include "std_common.h"
#include "std_rdebug.h"
#include "mcb_config.h"

typedef enum mesh_node_type_t{
    MESH_MASTER,
    MESH_SLAVE,
    MESH_AUTO,
}mesh_node_type_t;

typedef struct mesh_link_data_t{
    uint8_t *data; 
    int len;
}mesh_link_data_t;

int mesh_link_root_receive(mesh_addr_t *src_addr, mesh_link_data_t *link_data, int timeout_ms);

int mesh_link_node_receive(mesh_addr_t *src_addr, mesh_link_data_t *link_data, int timeout_ms);

int mesh_link_send2root(mesh_link_data_t *link_data);

int mesh_link_send2node(mesh_addr_t *dst_addr, mesh_link_data_t *link_data);

int mesh_link_group(uint8_t *mix_mac, int count, mesh_link_data_t *link_data);

int mesh_link_broadcast(mesh_link_data_t *link_data);

void mesh_link_init(mesh_event_cb_t mesh_cb, int node_type);

void mesh_link_deinit();

#ifdef PRODUCT_TEST
void mesh_link_root_receive_task(void *args);
void mesh_link_node_receive_task(void *args);
void mesh_link_test_init();
#endif

#endif