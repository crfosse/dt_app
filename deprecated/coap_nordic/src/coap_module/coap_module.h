
/*
- file: coap_module.h
- desc: contains the coap thread and the means to start it
*/

#ifndef _MQTT_MODULE_H_
#define _MQTT_MODULE_H_


#define COAP_THREAD_STACK_SIZE 4096
#define COAP_THREAD_PRIORITY 8

#define APP_COAP_SEND_INTERVAL_MS K_MSEC(5000)
#define APP_COAP_MAX_MSG_LEN 2048
#define APP_COAP_VERSION 1

void coap_start_thread();

int client_get_send(void);

int coap_connected(void);

#endif