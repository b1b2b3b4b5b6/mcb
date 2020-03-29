#ifndef MESH_CMD_HANDLE_H
#define MESH_CMD_HANDLE_H

#include "std_port_common.h"
#include "mesh_link_protocol.h"


typedef struct mesh_cmd_handle_t{
    char *cmd;
    void (*func)(uint8_t *mac, char *data);
}mesh_cmd_handle_t;

typedef struct mesh_cmd_shell_prefix_t{
    uint8_t reserver;
}__attribute__((__packed__)) mesh_cmd_shell_prefix_t;

typedef struct mesh_cmd_shell_t{
    base_shell_t *base_shell;
    mesh_cmd_shell_prefix_t prefix;
    char *json;
}mesh_cmd_shell_t;

#define ADD_CMD(__cmd, __func) do{\
    static mesh_cmd_handle_t __handle = {\
        .cmd = __cmd,\
        .func = (void (*)(uint8_t *, char *))__func,\
    };\
    mesh_cmd_handle_add(&__handle);\
}while(0)

void mesh_cmd_handle_add(mesh_cmd_handle_t *cmd_handle);
void mesh_cmd_handle_init();
mesh_cmd_shell_t *build_mesh_cmd_shell(char *cmd, int count, int len);
void free_mesh_cmd_shell(mesh_cmd_shell_t *mesh_cmd_shell);
int mesh_cmd_send(mesh_cmd_shell_t *cmd_shell, int time_ms);
int mesh_cmd_spread(mesh_cmd_shell_t *cmd_shell, int spread_quality, int retry);

#endif