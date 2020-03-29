#ifndef MESH_OUT_CMD_H
#define MESH_OUT_CMD_H

#include "mesh_interface.h"

#define ADD_OUT_CMD(__cmd, __func) do{\
    static mesh_out_cmd_handle_t __handle = {\
        .cmd = __cmd,\
        .func = (void (*)())__func,\
    };\
    mesh_out_cmd_handle_add(&__handle);\
}while(0)

typedef struct mesh_out_cmd_handle_t{
    char *cmd;
    void (*func)(char *data);
}mesh_out_cmd_handle_t;

void mesh_out_cmd_init();
void mesh_out_cmd_handle_add(mesh_out_cmd_handle_t *cmd_handle);

#endif