#include "interface_tcp.h"

#define STD_LOCAL_LOG_LEVEL STD_LOG_INFO
#define TASK_PRI  ESP_TASK_MAIN_PRIO
#define TASK_SIZE 3096 
#define RECV_TIMEOUT_S 0xffffffff
static int sockfd = 0;

static void close_socket()
{
    if(sockfd != 0)
    {
        close(sockfd);
        STD_LOGW("close sockfd");
        sockfd = 0;
    }
}

static int _interface_tcp_send(uint8_t *data, int len)
{
    if(sockfd == 0)
        return -1;

    STD_ASSERT(len <= 1472);
    static int res;

    STD_LOGD("tcp start to send[%d]", len);

    res = send(sockfd, data, len, 0);

    if(res == -1)
    {
        STD_LOGE("tcp data send fail");
        close_socket();
        goto FAIL;
    }
    
    STD_LOGD("tcp data send success[%d]", len);
    res = 0;

FAIL:
    if(res == -1)
    {
        STD_LOGW("interface send fail");
    }
    return res;  
}

static int try_recv(uint8_t *data, int *len)
{
    STD_LOGD("try tcp recv[%d]", *len);
    int res = recv(sockfd, data, *len, 0);

    if(res > 0)
    {
        STD_LOGD("tcp recv data[%d]", res);
        *len = res;
        interface_receive_t *receive = build_interface_receive(data, res);
        interface_push_data(receive);
        return 0;
    }

    if(res == 0) //socket close
    {
        STD_LOGW("socket close");
        return -1;
    }

    switch(errno)
    {
        case EINTR:
            STD_LOGW("recv time out");
            return 0;
            break;

        default:
            STD_LOGE("recv error[%s]", strerror(errno));
            return -1;
            break;
    }
}

static void recv_loop()
{
    int res;
    static uint8_t *data = NULL;
    if(data == NULL)
        data = STD_MALLOC(INTERFACE_RECV_SIZE);
    
    for(;;)
    {
        int len = INTERFACE_RECV_SIZE;
        res = try_recv(data, &len);
        if(res != 0)
        {
            close_socket();
            return;
        }
    }
}

static void task_recv(void *argc)
{
    STD_LOGD("tcp recv task running");
    for(;;)
    {
        if(!(esp_mesh_is_root() && router_is_connect()))
        {
            vTaskDelay(10000 / portTICK_PERIOD_MS);
            continue;
        }   
        STD_LOGD("start connect tcp");
        if(sockfd == 0)
        {
            int temp_fd = socket(AF_INET, SOCK_STREAM, 0);
            STD_ASSERT(temp_fd > 0);

            struct sockaddr_in addr;
            addr.sin_family=AF_INET;
            addr.sin_addr.s_addr= inet_addr(TCP_SERVER);
            addr.sin_port=htons(TCP_PORT);

            struct timeval tv = {
                .tv_sec = RECV_TIMEOUT_S,
                .tv_usec = 0,
            };
            STD_ASSERT(setsockopt(temp_fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) == 0);

            // int   keepAlive = 1;       //设定KeepAlive 
            // int   keepIdle = 5;        //首次探测开始前的tcp无数据收发空闲时间
            // int   keepInterval = 10;  //每次探测的间隔时间
            // int   keepCount = 2;     //探测次数
                                
            // if(setsockopt(temp_fd, SOL_SOCKET,SO_KEEPALIVE,(void*)&keepAlive,sizeof(keepAlive)) != 0)
            //     printf("Call setsockopt error, errno is %d/n", errno);

            // if(setsockopt(temp_fd, IPPROTO_TCP,TCP_KEEPIDLE,(void   *)&keepIdle,sizeof(keepIdle)) != 0)
            //     printf("Call setsockopt error, errno is %d/n", errno);

            // if(setsockopt(temp_fd, IPPROTO_TCP,TCP_KEEPINTVL,(void   *)&keepInterval,sizeof(keepInterval)) != 0)
            //     printf("Call setsockopt error, errno is %d/n", errno);
                            
            // if(setsockopt(temp_fd, IPPROTO_TCP,TCP_KEEPCNT,(void   *)&keepCount,sizeof(keepCount)) != 0)
            //     printf("Call setsockopt error, errno is %d/n", errno);

            socklen_t rcvaddr_len = sizeof(addr);
            
            if(connect(temp_fd, (struct sockaddr *)&addr, rcvaddr_len) != 0)
            {
                STD_LOGE("tcp connect fail, 5s later retry");
                close(temp_fd);
                vTaskDelay(5000/portTICK_PERIOD_MS);
                continue;
            }
            STD_LOGI("tcp connect success");

            static char *buf = NULL;
            if(buf == NULL)
            {
                buf = STD_CALLOC(40, 1);
                network_info_t info;
                std_nvs_load(KEY_NETWORK, &info, sizeof(network_info_t));
                char *mesh_id = info.mesh_id;
                json_pack_object(buf, "MeshID", &mesh_id); 
            }

            if(send(temp_fd, buf, strlen(buf), 0) <= 0)
            {
                STD_LOGE("tcp connect send mesh id fail");
                close(temp_fd);
                STD_FREE(buf);
                continue;
            }

            STD_LOGD("tcp connect send mesh id success");
            sockfd = temp_fd;
        }
        
        recv_loop();
    }
}

static int _interface_tcp_init()
{
    xTaskCreate(task_recv, "recv server", TASK_SIZE, NULL, TASK_PRI, NULL);
    return 0;   
}

void interface_tcp_init()
{
    interface_protocol_init = _interface_tcp_init;
    interface_send = _interface_tcp_send;
    STD_LOGI("interface_tcp_init success");
}

