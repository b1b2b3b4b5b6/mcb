#ifndef HTTP_OTA_H
#define HTTP_OTA_H

#include "mesh_link_protocol.h"
#include "mesh_cmd_handle.h"
#include "std_ota.h"
#include "mesh_out_cmd.h"
#include "mesh_status.h"

int http_ota_prepare(char *url);

#endif