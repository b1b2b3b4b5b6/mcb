#include "mesh_debug.h"

void mcb_debug_init()
{
    ADD_CMD("Brust", mesh_report_brust);
    ADD_OUT_CMD("ReportGather", mesh_report_gather);
}