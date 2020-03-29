#include "mesh_online_data.h"

#define STD_LOCAL_LOG_LEVEL STD_LOG_DEBUG

#define BUF_SIZE 1400

char *get_online_data()
{
    char *json = STD_CALLOC(BUF_SIZE, 1);
    int index = 0;
    mac_list_t *list = topo_get_all_list();
    index += sprintf(&json[index], "{\"Devices\":[");
    for(int n = 0; n < list->num; n++)
    {
        index += sprintf(&json[index], "\""MACSTR"\",", MAC2STR((uint8_t *)&list->macs[n]));
    }
    free_mac_list(list);
    if(json[index - 1] == ',')
        index--;
    sprintf(&json[index], "]}");

    char *typ = "online";
    json_pack_object(json, "Typ", &typ);
    
    return json;    
}

void free_online_data(char *json)
{
    STD_FREE(json);
}

