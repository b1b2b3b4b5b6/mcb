#include "mesh_link_data.h"

#define RESERVED_BYTES 8

#define STD_LOCAL_LOG_LEVEL STD_LOG_INFO

int get_max_mac_count(base_shell_t *shell)
{
    int max = MESH_MPS;
    max = max - sizeof(mesh_base_shell_prefix_t); //shell type byte
    max = max - shell->data_len;
    max = max - RESERVED_BYTES; // use for future
    max = max / 6;
    return max;
}

int get_max_data_len(int mac_count)
{
    int max = MESH_MPS;
    max = max - sizeof(mesh_base_shell_prefix_t);
    max = max - mac_count * 6;
    max = max - RESERVED_BYTES;
    return max; 
}

void fill_base_shell(base_shell_t *base_shell, uint8_t *raw_data, int raw_data_len)
{
    base_shell->raw_data = raw_data;
    base_shell->raw_data_len = raw_data_len;
    base_shell->prefix = (mesh_base_shell_prefix_t *)raw_data;
    base_shell->mix_mac = (mesh_addr_t *)&base_shell->raw_data[sizeof(mesh_base_shell_prefix_t)];
    base_shell->data = &base_shell->raw_data[sizeof(mesh_base_shell_prefix_t) + base_shell->prefix->mac_len * 6];
    base_shell->data_len = raw_data_len - sizeof(mesh_base_shell_prefix_t) - base_shell->prefix->mac_len * 6;
}

base_shell_t *build_base_shell(uint8_t type, int mac_count, int data_len)
{
    if(data_len > get_max_data_len(mac_count))
    {
        STD_END("build shell data len overload");
        return NULL;
    }
    base_shell_t *base_shell = STD_CALLOC(1, sizeof(base_shell_t));
    
    base_shell->raw_data_len = data_len + sizeof(mesh_base_shell_prefix_t) + mac_count * 6;
    base_shell->raw_data = STD_CALLOC(base_shell->raw_data_len, 1);
    base_shell->prefix = (mesh_base_shell_prefix_t *)base_shell->raw_data;
    base_shell->prefix->type = type;
    base_shell->prefix->mac_len = mac_count;
    memcpy(base_shell->prefix->self_mac, std_wifi_get_stamac(), 6);

    base_shell->mix_mac = (mesh_addr_t *)&base_shell->raw_data[sizeof(mesh_base_shell_prefix_t)];
    base_shell->data = &base_shell->raw_data[sizeof(mesh_base_shell_prefix_t) + mac_count * 6];
    base_shell->data_len = data_len;
    return base_shell;
}

base_shell_t *copy_base_shell(base_shell_t *base_shell)
{
    base_shell_t *copy = STD_CALLOC(1, sizeof(base_shell_t));
    copy->raw_data = STD_MALLOC(base_shell->raw_data_len);
    memcpy(copy->raw_data, base_shell->raw_data, base_shell->raw_data_len);
    fill_base_shell(copy, copy->raw_data, base_shell->raw_data_len);
    return copy;
}

void print_base_shell(base_shell_t *shell)
{
    STD_LOGD("----------base_shell-----------");
    STD_LOGD("src["MACSTR"], response[%d] index[%d] quality[%d]", \
        MAC2STR(shell->prefix->self_mac),\
        shell->prefix->response,\
        shell->prefix->index,\
        shell->prefix->spread_quality);
    STD_LOGD("type[%d], data_len[%d], mac_len[%d]",\
    shell->prefix->type,\
    shell->data_len,\
    shell->prefix->mac_len);
    if(shell->prefix->mac_len >= 1)
    {
        for(int n = 0; n < shell->prefix->mac_len; n++)
            STD_LOGD("mac[n]: "MACSTR, MAC2STR((uint8_t *)&shell->mix_mac[n]));
    }
    STD_LOGD("-------------------------------");
}

void free_base_shell(base_shell_t *shell)
{
    if(shell == NULL)
        return;
    STD_FREE(shell->raw_data);
    STD_FREE(shell);
}

shell_receive_t *build_shell_receive(base_shell_t *shell)
{
    shell_receive_t *recv = STD_CALLOC(1, sizeof(shell_receive_t));

    memcpy(recv->mac, shell->prefix->self_mac, 6);

    recv->data = STD_MALLOC(shell->data_len);
    recv->len = shell->data_len;
    memcpy(recv->data, shell->data, recv->len);
    
    return recv;
}

void free_shell_receive(shell_receive_t *shell_receive)
{
    if(shell_receive == NULL)
        return;
    if(shell_receive->data == NULL)
    {
        STD_FREE(shell_receive);
        return;
    }
    STD_FREE(shell_receive->data);
    STD_FREE(shell_receive);
}