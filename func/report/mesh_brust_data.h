#ifndef MESH_BRUST_DATA_H
#define MESH_BRUST_DATA_H

#include "mesh_device_data.h"
#include "mesh_topo.h"
#include "std_ota.h"

char *get_brust_data();
void free_brust_data(char *json);

#endif