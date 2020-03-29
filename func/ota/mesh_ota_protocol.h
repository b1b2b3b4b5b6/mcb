#ifndef MESH_OTA_PROTOCOL_H
#define MESH_OTA_PROTOCOL_H

#include "mesh_link_protocol.h"
#include "mesh_cmd_handle.h"
#include "std_ota.h"
#include "mesh_out_cmd.h"
#include "mesh_status.h"
#include "packet_ota.h"
#include "http_ota.h"

void mesh_ota_protocol_init();
void mesh_ota_temp();
#endif