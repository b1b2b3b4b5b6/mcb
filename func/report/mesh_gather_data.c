#include "mesh_gather_data.h"
#define STD_LOCAL_LOG_LEVEL STD_LOG_DEBUG
#define MAX_BUF 1400

device_info_raw_t g_device_info_raw;

static struct{
    uint8_t buf[MAX_BUF];
    int offset;
}g_device_info_buf;

void reset_gather_data()
{
    device_info_raw_t *raw  = get_device_raw();
    memcpy(g_device_info_buf.buf, raw, sizeof(g_device_info_raw));
    g_device_info_buf.offset = sizeof(g_device_info_raw);
}

void empty_gather_data()
{
    g_device_info_buf.offset = 0; 
}  

int add_gather_data(uint8_t *child_info, int len)
{
    if((len + g_device_info_buf.offset) >= MAX_BUF)
        return -1;
    memcpy(&g_device_info_buf.buf[g_device_info_buf.offset], child_info, len);
    g_device_info_buf.offset += len;
    return 0;
}

int copy_gather_data(uint8_t *buf)
{
    if(buf != NULL)
        memcpy(buf, g_device_info_buf.buf, g_device_info_buf.offset);
    return g_device_info_buf.offset;
}

uint8_t *get_gather_data()
{
    return g_device_info_buf.buf;
}

int get_gather_data_len()
{
    return g_device_info_buf.offset;
}

