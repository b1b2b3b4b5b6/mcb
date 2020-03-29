#include "mesh_cmd_handle.h"

#define STD_LOCAL_LOG_LEVEL STD_LOG_DEBUG

#define SHELL_CMD_MAX 20

#define TASK_PRI ESP_TASK_MAIN_PRIO
#define TASK_SIZE 3096


static mesh_cmd_handle_t *g_cmd_handle[SHELL_CMD_MAX] = {NULL};

void mesh_cmd_handle_add(mesh_cmd_handle_t *cmd_handle)
{
    for(int n = 0; n < SHELL_CMD_MAX; n++)
    {
        if(g_cmd_handle[n] == NULL)
        {   
            g_cmd_handle[n] = cmd_handle;
            STD_LOGD("mesh cmd add: %s", cmd_handle->cmd);
            return;
        }
    }
    STD_END("mesh cmd handle overflow");
}

mesh_cmd_shell_t *build_mesh_cmd_shell(char *cmd, int count, int len)
{
    mesh_cmd_shell_t *cmd_shell = STD_CALLOC(1, sizeof(base_shell_t));
    base_shell_t * base_shell = build_base_shell(SHELL_CMD, count, len + sizeof(mesh_cmd_shell_prefix_t));

    cmd_shell->base_shell = base_shell;
    cmd_shell->json = (char *)&base_shell->data[sizeof(mesh_cmd_shell_prefix_t)];
    json_pack_object(cmd_shell->json, "Cmd", &cmd);
    return cmd_shell;
}

void free_mesh_cmd_shell(mesh_cmd_shell_t *mesh_cmd_shell)
{
    if(mesh_cmd_shell == NULL)
        return;
    if(mesh_cmd_shell->base_shell == NULL)
    {
        STD_FREE(mesh_cmd_shell);
        return;
    }
    free_base_shell(mesh_cmd_shell->base_shell);
    STD_FREE(mesh_cmd_shell);
}

int mesh_cmd_send(mesh_cmd_shell_t *cmd_shell, int time_ms)
{
    return mesh_link_send(cmd_shell->base_shell, time_ms);
}

int mesh_cmd_spread(mesh_cmd_shell_t *cmd_shell, int spread_quality, int retry)
{
    cmd_shell->base_shell->prefix->spread_quality = spread_quality + (retry - 1) * SPREAD_MUL;
    return mesh_link_send(cmd_shell->base_shell, 0);
}

static void mesh_cmd_handle_task(void *arg)
{
    shell_receive_t *shell_receive;
    char *json;
    char *cmd;
    int n;
    STD_LOGI("cmd handle task start");
    for(;;)
    {
        shell_receive = mesh_link_wait_shell(SHELL_CMD, portMAX_DELAY);
        //FIXME:shall not use portMAX_DELAY, instead with 10000
        if(shell_receive == NULL)
            continue;

        json = (char *)&shell_receive->data[sizeof(mesh_cmd_shell_prefix_t)];

        STD_LOGD("cmd handle receive: %s", json);
        STD_ASSERT(json_parse_object(json, "Cmd", &cmd) == 0) ;
        n = 0;
        for(;;)
        {
            if(g_cmd_handle[n] == NULL)
            {
                STD_LOGE("receive undefined cmd");
                break;
            }

            if(strcmp(cmd, g_cmd_handle[n]->cmd) == 0)
            {
                g_cmd_handle[n]->func(shell_receive->mac, json);
                break;
            }
            n++;
        }
        STD_FREE(cmd);
        free_shell_receive(shell_receive);
    }
    vTaskDelete(NULL);
}


void mesh_cmd_handle_init()
{
    mesh_link_shell_add(SHELL_CMD, SHELL_CMD_MAX);
    xTaskCreate(mesh_cmd_handle_task, "cmd handle task", TASK_SIZE, NULL, TASK_PRI, NULL);
}

