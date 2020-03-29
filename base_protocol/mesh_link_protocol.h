#ifndef MESH_LINK_PROTOCOL_H
#define MESH_LINK_PROTOCOL_H

#include "mesh_link_trans.h"
#include "mesh_link_data.h"
#include "mesh_interface.h"



void mesh_link_protocol_init(mesh_event_cb_t mesh_cb, int node_type);
int mesh_await_connect(int time_s);
void await_spread_free();
void await_spread_empty();
int mesh_link_shell_add(int type, int size);
shell_receive_t *mesh_link_wait_shell(int type, uint32_t time_ms);
void mesh_link_empty_shell(int type);

int mesh_link_send(base_shell_t *base_shell, int time_ms);
int mesh_link_send_out(uint8_t type, uint8_t *data, int len, int time_ms);

void set_mesh_is_connect(bool value);
void set_router_is_connect(bool value);

bool mesh_is_connect();
bool router_is_connect();


void mesh_link_test();

#endif