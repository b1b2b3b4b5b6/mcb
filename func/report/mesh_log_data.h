#ifndef MESH_LOG_DATA_H
#define MESH_LOG_DATA_H

#include "mesh_device_data.h"
#include "mesh_topo.h"
#include "std_ota.h"

char *get_log_data(char *log);
void free_log_data(char *json);

#endif