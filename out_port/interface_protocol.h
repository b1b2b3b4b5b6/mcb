#ifndef INTERFACE_PROTOCOL_H
#define INTERFACE_PROTOCOL_H

#include "network.h"
#include "mcb_config.h"

#define INTERFACE_RECV_SIZE 1472
#define INTERFACE_SEND_SIZE 1472

#define INTERFACE_KEY "INTERFACE_KEY"

typedef enum interface_type_t{
    INTERFACE_UDP,
    INTERFACE_MQTT,
    INTERFACE_TCP,
}interface_type_t;

typedef struct interface_config_t{
    int type;
}interface_config_t;

typedef struct interface_receive_t{
    uint8_t *data;
    int len;
}__attribute__((__packed__)) interface_receive_t;


extern int (*interface_protocol_init)();
extern int (*interface_send)(uint8_t *, int);

interface_receive_t *interface_wait_data(int time_ms);
void free_interface_receive(interface_receive_t *receive);
interface_receive_t *build_interface_receive(uint8_t *raw_data, int len);
int interface_push_data(interface_receive_t *data);

//extern func
bool router_is_connect();
#endif