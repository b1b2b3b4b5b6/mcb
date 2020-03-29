#include "interface_udp.h"
#define STD_LOCAL_LOG_LEVEL STD_LOG_INFO

#define TASK_PRI  ESP_TASK_MAIN_PRIO
#define TASK_SIZE 2048

#define MAX_SEND_RETRY 3
#define RECV_TIMEOUT_S 3

#define OK 0
#define BUSY 1


typedef struct interface_prefix_t{
    uint8_t packet_id;
}__attribute__((__packed__)) interface_prefix_t;

static struct sockaddr_in server_addr = {0};

static bool g_has_server = false;
static void set_server_unreachable()
{
    g_has_server = false;
}

static void set_server_reachable()
{
    g_has_server = true;
}

static bool has_server()
{
    return g_has_server;
}

static int _interface_udp_send(uint8_t *data, int len)
{
    if(!has_server())
        return -1;

    STD_ASSERT(len <= 1472);
    static int sockfd = 0;
    static int res;
    static uint8_t buf[100];
    static struct sockaddr_in addrto;
	bzero(&addrto, sizeof(struct in_addr));
	addrto.sin_family=AF_INET;
	addrto.sin_addr.s_addr=server_addr.sin_addr.s_addr;
	addrto.sin_port=htons(UPLOAD_PORT);
    
    if(sockfd == 0)
    {
        static struct timeval tv = {
            .tv_sec = RECV_TIMEOUT_S,
            .tv_usec = 0,
        };
        sockfd = socket(AF_INET, SOCK_DGRAM, 0);
        STD_ASSERT(setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) == 0);
    }
    STD_LOGD("udp start to send[%d]", len);
    for(int n = 0 ; n < MAX_SEND_RETRY; n++)
    {
        res = sendto(sockfd, data, len, 0, (const struct sockaddr *)&addrto, sizeof(addrto));
        if(res == -1)
        {
            STD_LOGE("udp data send fail");
            goto FAIL;
        }

        res = recv(sockfd, buf, 100, 0);
        
        if(res != -1)
        {
            STD_LOGD("udp data send success[%d]", len);
            res = 0;
            break;
        }
        STD_LOGW("udp send retry[%d]", n);
    }

FAIL:
    if(res == -1)
    {
        STD_LOGW("interface send fail");
        set_server_unreachable();
    }
    return res;
}

static int find_ip()
{
    
    int socket_fd;
    socklen_t rcvaddr_len;
    struct sockaddr_in addrto;
	bzero(&addrto, sizeof(struct in_addr));
	addrto.sin_family=AF_INET;
	addrto.sin_addr.s_addr=htonl(INADDR_BROADCAST);
	addrto.sin_port=htons(SERVER_FIND_PORT);
    socket_fd = socket(AF_INET, SOCK_DGRAM, 0);

    static char *buf = NULL;
    if(buf == NULL)
    {
        buf = STD_CALLOC(40, 1);
        network_info_t info;
        std_nvs_load(KEY_NETWORK, &info, sizeof(network_info_t));
        char *mesh_id = info.mesh_id;
        json_pack_object(buf, "MeshID", &mesh_id); 
    }
    
    int res = sendto(socket_fd, buf, strlen(buf), 0, (const struct sockaddr *)&addrto, sizeof(addrto));
    if(res == -1)
    {
        STD_LOGE("find ip send fail");
        goto CLEAN;
    }

    for(int n = 0; n < 3; n++)
    {   
        vTaskDelay(300/portTICK_PERIOD_MS);
        rcvaddr_len = sizeof(addrto);
        res = recvfrom(socket_fd, buf, 100, MSG_DONTWAIT, (struct sockaddr *)&server_addr, &rcvaddr_len);
        if(res != -1)
        {
            break;
        }
    }
    
CLEAN:
    close(socket_fd);
    if(res != -1)
    {
        STD_LOGI("find server ip: %s", inet_ntoa(server_addr.sin_addr.s_addr));
        set_server_reachable();
        return 0;
    }
    STD_LOGW("server ip not found");
    set_server_unreachable();
    return -1;
}

static void task_find_server(void *arg)
{  
    for(;;)
    {
        if(router_is_connect() && !has_server())
        {
            STD_LOGD("start find ip");
            find_ip();
        }
        vTaskDelay(10000/portTICK_PERIOD_MS);        
    }
}

uint8_t get_id()
{
    static uint8_t id = 0;
    id++;
    if(id == 0)
        id = 1;
    return id;
}

void task_receive(void *argc)
{
    static uint8_t last_id = 0;
    int res;
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    struct sockaddr_in addr;
    addr.sin_family=AF_INET;
    addr.sin_addr.s_addr=htonl(INADDR_ANY);
    addr.sin_port=htons(LISTEN_PORT);
    STD_ASSERT(bind(sockfd, (const struct sockaddr *)&addr, sizeof(addr)) == 0);
    struct timeval tv = {
        .tv_sec = 5,
        .tv_usec = 0,
    };  
    STD_ASSERT(setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) == 0);   
    uint8_t *data = STD_MALLOC(INTERFACE_RECV_SIZE);
    for(;;)
    {
        if(!(esp_mesh_is_root() && router_is_connect()))
        {
            vTaskDelay(10000 / portTICK_PERIOD_MS);
            continue;
        }   

        static socklen_t rcvaddr_len = sizeof(server_addr);
        res = recvfrom(sockfd, data, 10000, 0, (struct sockaddr *)&server_addr, &rcvaddr_len);

        if(res == -1)
            continue;
             
        set_server_reachable();
        int data_len = res;
        interface_prefix_t *prefix = (interface_prefix_t *)data;

        uint8_t ret;
        if(prefix->packet_id != last_id)
        {
            last_id = prefix->packet_id;
            interface_receive_t *receive = build_interface_receive(&data[sizeof(interface_prefix_t)], data_len - sizeof(interface_prefix_t));
            if(interface_push_data(receive) == 0)
                ret = OK;
            else
                ret = BUSY;
        }
        else
        {
            ret = OK;
        }

        sendto(sockfd, &ret, sizeof(ret), 0, (const struct sockaddr *)&server_addr, sizeof(server_addr));
    }
}

static int _interface_udp_init()
{
    xTaskCreate(task_find_server, "find server", TASK_SIZE, NULL, TASK_PRI, NULL);
    xTaskCreate(task_receive, "recv server", TASK_SIZE, NULL, TASK_PRI, NULL);
    return 0;   
}

void interface_udp_init()
{
    interface_protocol_init = _interface_udp_init;
    interface_send = _interface_udp_send;
    STD_LOGI("interface_udp_init success");
}

