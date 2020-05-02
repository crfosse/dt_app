/*
- file: mqtt_module.h
- desc: contains the mqtt thread and the means to start it
*/

#ifndef _MQTT_MODULE_H_
#define _MQTT_MODULE_H_

#define MQTT_THREAD_STACK_SIZE 2048
#define MQTT_THREAD_PRIORITY 8

void mqtt_start_thread();

int mqtt_data_publish(u8_t *data, size_t len);

int mqtt_connected(void);

#endif