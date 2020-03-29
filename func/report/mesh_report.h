#ifndef MESH_REPORT_H
#define MESH_REPORT_H

#include "mesh_link_protocol.h"
#include "mesh_topo.h"
#include "mesh_interface.h"
#include "mesh_cmd_handle.h"
#include "mesh_out_cmd.h"
#include "mesh_status.h"

#include "mesh_gather_data.h"
#include "mesh_brust_data.h"
#include "mesh_log_data.h"
#include "mesh_online_data.h"

void mesh_report_log(char *str);
void mesh_report_brust();
void mesh_report_gather();
void mesh_report_init();
void mesh_report_online();
bool mesh_is_primary();


#endif

