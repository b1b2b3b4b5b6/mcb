#include "mesh_topo.h"

#define STD_LOCAL_LOG_LEVEL STD_LOG_DEBUG

typedef struct mesh_topo_t{
    bool has_root;
    mesh_addr_t root;
    bool has_parent;
    mesh_addr_t parent;
    mesh_addr_t child[MESH_MAX_CHILD_NODE];
    bool child_status[MESH_MAX_CHILD_NODE];
    uint8_t layer;
}mesh_topo_t;

xSemaphoreHandle g_topo_lock = NULL;

static mesh_topo_t g_mesh_topo = {0};

static inline void topo_lock()
{
    if(g_topo_lock == NULL)
    {
        g_topo_lock = xSemaphoreCreateBinary();
        xSemaphoreGive(g_topo_lock);
    }
    xSemaphoreTake(g_topo_lock, portMAX_DELAY);
}

static inline void topo_unlock()
{
    if(g_topo_lock == NULL)
    {
        g_topo_lock = xSemaphoreCreateBinary();
    }
    xSemaphoreGive(g_topo_lock);
}

int topo_add_child(uint8_t aid, mesh_addr_t *child_mac)
{
    g_mesh_topo.child_status[aid - 1] = true;
    memcpy(&g_mesh_topo.child[aid - 1], child_mac, sizeof(mesh_addr_t));
    return 0;
}

int topo_remove_child(int aid)
{   
    g_mesh_topo.child_status[aid - 1] = false;
    return 0;
}

mac_list_t *topo_get_child_list()
{
    mac_list_t *mac_list = STD_CALLOC(1, sizeof(mac_list_t));
    mac_list->ptr = STD_CALLOC(MESH_MAX_CHILD_NODE, sizeof(mesh_addr_t));
    mac_list->macs = (mesh_addr_t *)mac_list->ptr;

    mac_list->num = 0;
    for(int n = 0; n < MESH_MAX_CHILD_NODE; n++)
    {
        if(g_mesh_topo.child_status[n] == true)
        {
            memcpy(&mac_list->macs[mac_list->num], &g_mesh_topo.child[n], sizeof(mesh_addr_t));
            mac_list->num++;
        }       
    }
    return mac_list;
}

mac_list_t * topo_get_all_list()
{
    int num = esp_mesh_get_routing_table_size();
    
    mac_list_t *mac_list = STD_CALLOC(1, sizeof(mac_list_t));

    int size;
    mesh_addr_t *macs_total = STD_CALLOC(num, sizeof(mesh_addr_t));
    mac_list->ptr = (uint8_t *)macs_total;
    ESP_ERROR_CHECK(esp_mesh_get_routing_table(macs_total, num * sizeof(mesh_addr_t), &size));
    mac_list->num = size;
    mac_list->macs = &macs_total[0];
    
    return mac_list;
}

mac_list_t * topo_get_other_list()
{
    int num = esp_mesh_get_routing_table_size();
    
    mac_list_t *mac_list = STD_CALLOC(1, sizeof(mac_list_t));

    if(num > 1)
    {
        int size;
        mesh_addr_t *macs_total = STD_CALLOC(num, sizeof(mesh_addr_t));
        mac_list->ptr = (uint8_t *)macs_total;
        ESP_ERROR_CHECK(esp_mesh_get_routing_table(macs_total, num * sizeof(mesh_addr_t), &size));
        mac_list->num = size - 1;
        mac_list->macs = &macs_total[1];
    }
    return mac_list;
}

void topo_set_root(mesh_addr_t *mac)
{
    if(mac != NULL)
    {
        g_mesh_topo.root = *mac;
        g_mesh_topo.has_root = true;
    }
    else
    {
        g_mesh_topo.has_root = false;
    }
}

mesh_addr_t *topo_get_root()
{
    if(!g_mesh_topo.has_root)
        return NULL;
    return &g_mesh_topo.root;
}

void topo_set_parent(mesh_addr_t *mac)
{
    if(mac != NULL)
    {
        g_mesh_topo.parent = *mac;\
        g_mesh_topo.has_parent = true;
    }
    else
    {
        g_mesh_topo.has_parent = false;
    }
}

mesh_addr_t *topo_get_parent()
{
    if(!g_mesh_topo.has_parent)
        return NULL;
    return &g_mesh_topo.parent;
}

int topo_get_child_num()
{
    int sum = 0;
    for(int n = 0; n < MESH_MAX_CHILD_NODE; n++)
    {
        if(g_mesh_topo.child_status[n] == true)
            sum++;
    }
    return sum;
}

int topo_get_all_num()
{
    return esp_mesh_get_routing_table_size();
}

void free_mac_list(mac_list_t *mac_list)
{
    if(mac_list == NULL)
        return;

    if(mac_list->ptr == NULL)
    {
        STD_FREE(mac_list);
        return;
    }
        
    STD_FREE(mac_list->ptr);
    STD_FREE(mac_list);
}

void print_mac_list(mac_list_t *mac_list)
{
    if(mac_list == NULL)
        STD_LOGE("mac list error");
    STD_LOGD("----------mac[%d]---------", mac_list->num);
    for(int n = 0; n < mac_list->num; n++)
    {
        STD_LOGD(MACSTR, MAC2STR((uint8_t *)&mac_list->macs[n]));
    }
    STD_LOGD("--------------------------");
}

int topo_calcu_layer(int total)
{
    if(total <= MESH_MAX_CHILD_NODE)
        return 1;
    if(total <= (MESH_MAX_CHILD_NODE + MESH_MAX_CHILD_NODE * MESH_MAX_CHILD_NODE))
        return 3;
    if(total <= (MESH_MAX_CHILD_NODE + MESH_MAX_CHILD_NODE * MESH_MAX_CHILD_NODE + MESH_MAX_CHILD_NODE * MESH_MAX_CHILD_NODE * MESH_MAX_CHILD_NODE))
        return 5;
    return 7;
}

void topo_set_layer(uint8_t layer)
{
    g_mesh_topo.layer = layer;
}

int topo_get_layer()
{
    return g_mesh_topo.layer;
}
