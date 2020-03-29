#include "interface_protocol.h"

#define STD_LOCAL_LOG_LEVEL STD_LOG_INFO
#define RECV_SIZE 1

static xQueueHandle g_recv_queue = NULL;

int (*interface_protocol_init)() = NULL;
int (*interface_send)(uint8_t *, int) = NULL;

//static xSemaphoreHandle g_send_lock = NULL;
// static void send_lock()
// {
//     xSemaphoreTake(g_send_lock, portMAX_DELAY);
// }

// static void send_unlock()
// {
//     xSemaphoreGive(g_send_lock);
// }

int interface_push_data(interface_receive_t *data)
{
    if(xQueueSend(g_recv_queue, &data, 0) == pdFALSE)
    {
        STD_LOGW("interface queue full");
        return -1;
    }
    else
        return 0;
}

interface_receive_t *interface_wait_data(int time_ms)
{
    interface_receive_t *receive;
    if(g_recv_queue == NULL)
        g_recv_queue = xQueueCreate(RECV_SIZE, sizeof(interface_receive_t *));

    if(xQueueReceive(g_recv_queue, &receive, time_ms / portTICK_PERIOD_MS) == pdTRUE)
        return receive;
    else
        return NULL;
}

void free_interface_receive(interface_receive_t *receive)
{
    if(receive == NULL)
        return;
    
    if(receive->data == NULL)
    {
        STD_FREE(receive);
        return;
    }
    
    STD_FREE(receive->data);
    STD_FREE(receive);   
}

interface_receive_t *build_interface_receive(uint8_t *raw_data, int len)
{
    if(raw_data == NULL)
        return NULL;
    interface_receive_t *receive;
    receive = STD_CALLOC(1, sizeof(interface_receive_t));
    receive->data = STD_CALLOC(len, 1);
    receive->len = len;
    memcpy(receive->data, raw_data, receive->len);
    return receive;
}


#ifdef PRODUCT_TEST

#endif