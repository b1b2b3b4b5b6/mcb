#ifndef MESH_GATHER_DATA_H
#define MESH_GATHER_DATA_H

#include "std_wifi.h"
#include "mesh_device_data.h"

void reset_gather_data();
void empty_gather_data();
int add_gather_data(uint8_t *gather_data, int len);
int copy_gather_data(uint8_t *buf);
uint8_t *get_gather_data();
int get_gather_data_len();

#endif