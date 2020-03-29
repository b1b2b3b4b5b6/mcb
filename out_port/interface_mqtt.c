// #include "interface_mqtt.h"

// #define STD_LOCAL_LOG_LEVEL STD_LOG_DEBUG

// #define BROKER_URL "lumoschen.cn"
// #define RECV_TOPIC "/xxx/recv"
// #define SEND_TOPIC "/xxx/send"

// #define TASK_PRI  ESP_TASK_MAIN_PRIO
// #define TASK_SIZE 2048

// static esp_mqtt_client_handle_t g_client = NULL;

// static int _interafce_mqtt_send(uint8_t *data, int len)
// {
//     int msg_id = esp_mqtt_client_publish(g_client, SEND_TOPIC, data, len, 1, 0);
//     if(msg_id < 0)
//     {
//         STD_LOGE("mqtt publish fail");
//         return -1;
//     }
//     return 0;
// }   

// static esp_err_t mqtt_event_handler(esp_mqtt_event_handle_t event)
// {
//     int msg_id;
//     // your_context_t *context = event->context;
//     switch (event->event_id) {
//         case MQTT_EVENT_CONNECTED:
//             ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");

//             msg_id = esp_mqtt_client_subscribe(g_client, RECV_TOPIC, 1);
//             ESP_LOGI(TAG, "sent subscribe successful, msg_id=%d", msg_id);
//             break;

//         case MQTT_EVENT_DISCONNECTED:
//             ESP_LOGI(TAG, "MQTT_EVENT_DISCONNECTED");
//             break;

//         case MQTT_EVENT_SUBSCRIBED:
//             ESP_LOGI(TAG, "MQTT_EVENT_SUBSCRIBED, msg_id=%d", event->msg_id);
//             break;

//         case MQTT_EVENT_UNSUBSCRIBED:
//             ESP_LOGI(TAG, "MQTT_EVENT_UNSUBSCRIBED, msg_id=%d", event->msg_id);
//             break;

//         case MQTT_EVENT_PUBLISHED:
//             ESP_LOGI(TAG, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
//             break;

//         case MQTT_EVENT_DATA:
//             ESP_LOGI(TAG, "MQTT_EVENT_DATA");
//             printf("TOPIC=%.*s\r\n", event->topic_len, event->topic);
//             printf("DATA=%.*s\r\n", event->data_len, event->data);
//             break;

//         case MQTT_EVENT_ERROR:
//             ESP_LOGI(TAG, "MQTT_EVENT_ERROR");
//             break;

//         default:
//             ESP_LOGI(TAG, "Other event id:%d", event->event_id);
//             break;
//     }
//     return ESP_OK;
// }

// static void loop_task(void *argc)
// {
//     for(;;)
//     {
//         if(router_is_connect() && mesh_is_connect() && esp_mesh_is_root())
//         {
//             if(g_client == NULL)
//             {
//                 esp_mqtt_client_config_t mqtt_cfg = {
//                     .uri = BROKER_URL,
//                     .event_handle = mqtt_event_handler,
//                 };

//                 g_client = esp_mqtt_client_init(&mqtt_cfg);
//                 esp_mqtt_client_start(g_client);
//             }
//         }

//         if(!esp_mesh_is_root() && g_client == NULL) 
//         {
            
//             esp_mqtt_client_stop(g_client);
//             esp_mqtt_client_destroy(g_client);
//             g_client = NULL;
//         }

//         vTaskDelay(10000 / portTICK_PERIOD_MS);
//     }
// }

// static int _interface_mqtt_init(void)
// {
//     xTaskCreate(loop_task, "looptask", TASK_SIZE, NULL, TASK_PRI, NULL);
// }

// void interface_mqtt_init(void)
// {
//     interface_protocol_init = _interface_mqtt_init;
//     interface_send = _interafce_mqtt_send;
//     STD_LOGI("interface_mqtt_init success");
// }

