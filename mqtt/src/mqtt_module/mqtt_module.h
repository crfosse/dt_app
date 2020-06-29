/*
- file: mqtt_module.h
- desc: contains the mqtt thread and the means to start it
*/

#ifndef _MQTT_MODULE_H_
#define _MQTT_MODULE_H_

// Initiates MQTT connection and thread
void mqtt_start_thread();

// Publish data to topic specified in prj.conf
int mqtt_data_publish(u8_t *data, size_t len);

// Returns connection status of MQTT
int mqtt_connected(void);

#endif