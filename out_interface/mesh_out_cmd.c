#include "mesh_out_cmd.h"

#define STD_LOCAL_LOG_LEVEL STD_LOG_DEBUG

#define HANDLE_TASK_PRI  ESP_TASK_MAIN_PRIO
#define HANDLE_TASK_SIZE 4096

#define MAX_HANDLE 20

static mesh_out_cmd_handle_t *g_cmd_handle[MAX_HANDLE] = {NULL};

void mesh_out_cmd_handle_add(mesh_out_cmd_handle_t *cmd_handle)
{
    for(int n = 0; n < MAX_HANDLE; n++)
    {
        if(g_cmd_handle[n] == NULL)
        {   
            g_cmd_handle[n] = cmd_handle;
            STD_LOGD("interface cmd handle add : %s", cmd_handle->cmd);
            return;
        }
    }
    STD_END("interface cmd handle overflow");
}

static void handle_task(void *argc)
{
    STD_LOGI("interface cmd handle task start");
    for(;;)
    {
        out_data_t * receive = mesh_wait_out(OUT_STR, 5000);
        if(receive == NULL)
            continue;
            
        char *cmd = NULL;
        STD_LOGD("out recv[%s]", receive->data);
        STD_ERROR_CONTIUNE(json_parse_object(receive->data, "Typ", &cmd));
        
        int n = 0;
        for(;;)
        {
            if(cmd == NULL)
                break;

            if(g_cmd_handle[n] == NULL)
            {
                STD_LOGE("receive undefined cmd[%s]", cmd);
                break;
            }

            if(strcmp(cmd, g_cmd_handle[n]->cmd) == 0)
            {
                g_cmd_handle[n]->func((char *)receive->data);
                break;
            }
            n++;
        }
        json_free(cmd, 0);
        free_out_data(receive);
    }
    vTaskDelete(NULL);  
}


void mesh_out_cmd_init()
{
    mesh_interface_add_type(OUT_STR, 5);
    xTaskCreate(handle_task, "handle server", HANDLE_TASK_SIZE, NULL, HANDLE_TASK_PRI, NULL);
}