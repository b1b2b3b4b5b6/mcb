#include "mesh_status.h"
static bool g_self_ota = false;
bool is_self_ota()
{
    return g_self_ota;
}
void set_self_ota(bool status)
{   
    g_self_ota = status;
}