#ifndef MESH_TOPO_H
#define MESH_TOPO_H

#include "std_port_common.h"
#include "mcb_config.h"
#include "std_wifi.h"

typedef struct mac_list_t{
    mesh_addr_t *macs;
    uint8_t *ptr;
    int num;
}mac_list_t;

int topo_add_child(uint8_t aid, mesh_addr_t *child_mac);
int topo_remove_child(int aid);

mac_list_t *topo_get_all_list();
mac_list_t *topo_get_child_list();
mac_list_t *topo_get_other_list();
void free_mac_list(mac_list_t *mac_list);

void topo_set_root(mesh_addr_t *mac);
mesh_addr_t *topo_get_root();

void topo_set_parent(mesh_addr_t *mac);
mesh_addr_t *topo_get_parent();

int topo_get_child_num();
int topo_get_all_num();

void print_mac_list(mac_list_t *mac_list);
int topo_calcu_layer(int total);

void topo_set_layer(uint8_t layer);
int topo_get_layer();

#endif