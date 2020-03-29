#ifndef MESH_ONLINE_DATA_H
#define MESH_ONLINE_DATA_H

#include "mesh_device_data.h"
#include "mesh_topo.h"

char *get_online_data();
void free_online_data(char *json);

#endif