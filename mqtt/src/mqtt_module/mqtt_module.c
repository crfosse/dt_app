/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <zephyr.h>
#include <stdio.h>
#include <string.h>

#include <net/mqtt.h>
#include <net/socket.h>

#include "mqtt_module.h"


LOG_MODULE_REGISTER(app_mqtt_module, CONFIG_APP_LOG_LEVEL);

/* Thread specifics */
K_THREAD_STACK_DEFINE(mqtt_stack_area, MQTT_THREAD_STACK_SIZE);
static void mqtt_thread(void *, void *, void *);

static struct k_thread mqtt_thread_data;
static k_tid_t mqtt_thread_tid;

/* Buffers for MQTT client. */
static u8_t rx_buffer[CONFIG_MQTT_MESSAGE_BUFFER_SIZE];
static u8_t tx_buffer[CONFIG_MQTT_MESSAGE_BUFFER_SIZE];
static u8_t payload_buf[CONFIG_MQTT_PAYLOAD_BUFFER_SIZE];

/* The mqtt client struct */
static struct mqtt_client client;

/* MQTT Broker details. */
static struct sockaddr_storage broker;

/* Connected flag */
static bool connected;

/* File descriptor */
static struct pollfd fds;

/**@brief Function to print strings without null-termination
 */
static void data_print(u8_t *prefix, u8_t *data, size_t len)
{
	char buf[len + 1];

	memcpy(buf, data, len);
	buf[len] = 0;
	LOG_DBG("%s%s\n", log_strdup(prefix), log_strdup(buf));
}

/**@brief Function to publish data on the configured topic
 */
static int data_publish(struct mqtt_client *c, enum mqtt_qos qos,
	u8_t *data, size_t len)
{
	struct mqtt_publish_param param;

	param.message.topic.qos = qos;
	param.message.topic.topic.utf8 = CONFIG_MQTT_PUB_TOPIC;
	param.message.topic.topic.size = strlen(CONFIG_MQTT_PUB_TOPIC);
	param.message.payload.data = data;
	param.message.payload.len = len;
	param.message_id = sys_rand32_get();
	param.dup_flag = 0;
	param.retain_flag = 0;

	data_print("Publishing: ", data, len);
	LOG_DBG("to topic: %s len: %u\n",
		log_strdup(CONFIG_MQTT_PUB_TOPIC),
		(unsigned int)strlen(CONFIG_MQTT_PUB_TOPIC));

	return mqtt_publish(c, &param);
}

int mqtt_data_publish(u8_t *data, size_t len) {
	int err;

	err = data_publish(&client, MQTT_QOS_1_AT_LEAST_ONCE,
	data, len);

	return err;
}

/**@brief Function to subscribe to the configured topic
 */
static int subscribe(void)
{
	struct mqtt_topic subscribe_topic = {
		.topic = {
			.utf8 = CONFIG_MQTT_SUB_TOPIC,
			.size = strlen(CONFIG_MQTT_SUB_TOPIC)
		},
		.qos = MQTT_QOS_1_AT_LEAST_ONCE
	};

	const struct mqtt_subscription_list subscription_list = {
		.list = &subscribe_topic,
		.list_count = 1,
		.message_id = 1234
	};

	LOG_INF("Subscribing to: %s len %u\n", log_strdup(CONFIG_MQTT_SUB_TOPIC),
		(unsigned int)strlen(CONFIG_MQTT_SUB_TOPIC));

	return mqtt_subscribe(&client, &subscription_list);
}

/**@brief Function to read the published payload.
 */
static int publish_get_payload(struct mqtt_client *c, size_t length)
{
	u8_t *buf = payload_buf;
	u8_t *end = buf + length;

	if (length > sizeof(payload_buf)) {
		return -EMSGSIZE;
	}

	while (buf < end) {
		int ret = mqtt_read_publish_payload(c, buf, end - buf);

		if (ret < 0) {
			int err;

			if (ret != -EAGAIN) {
				return ret;
			}

			LOG_DBG("mqtt_read_publish_payload: EAGAIN\n");

			err = poll(&fds, 1, K_SECONDS(CONFIG_MQTT_KEEPALIVE));
			if (err > 0 && (fds.revents & POLLIN) == POLLIN) {
				continue;
			} else {
				return -EIO;
			}
		}

		if (ret == 0) {
			return -EIO;
		}

		buf += ret;
	}

	return 0;
}

/**@brief MQTT client event handler
 */
void mqtt_evt_handler(struct mqtt_client *const c,
		      const struct mqtt_evt *evt)
{
	int err;

	switch (evt->type) {
	case MQTT_EVT_CONNACK:
		if (evt->result != 0) {
			LOG_WRN("MQTT connect failed %d\n", evt->result);
			break;
		}

		connected = true;
		LOG_INF("[%s:%d] MQTT client connected!\n", log_strdup(__func__), __LINE__);
		subscribe();
		break;

	case MQTT_EVT_DISCONNECT:
		LOG_WRN("[%s:%d] MQTT client disconnected %d\n", log_strdup(__func__),
		       __LINE__, evt->result);

		connected = false;
		break;

	case MQTT_EVT_PUBLISH: {
		const struct mqtt_publish_param *p = &evt->param.publish;

		LOG_DBG("[%s:%d] MQTT PUBLISH result=%d len=%d\n", log_strdup(__func__),
		       __LINE__, evt->result, p->message.payload.len);
		err = publish_get_payload(c, p->message.payload.len);
		if (err >= 0) {
			data_print("Received:", payload_buf,
				p->message.payload.len);
		} else {
			LOG_WRN("mqtt_read_publish_payload: Failed! %d\n", err);
			LOG_WRN("Disconnecting MQTT client...\n");

			err = mqtt_disconnect(c);
			if (err) {
				LOG_WRN("Could not disconnect: %d\n", err);
			}
		}
	} break;

	case MQTT_EVT_PUBACK:
		if (evt->result != 0) {
			LOG_WRN("MQTT PUBACK error %d\n", evt->result);
			break;
		}

		LOG_DBG("[%s:%d] PUBACK packet id: %u\n", log_strdup(__func__), __LINE__,
				evt->param.puback.message_id);
		break;

	case MQTT_EVT_SUBACK:
		if (evt->result != 0) {
			LOG_WRN("MQTT SUBACK error %d\n", evt->result);
			break;
		}

		LOG_INF("[%s:%d] SUBACK packet id: %u\n", log_strdup(__func__), __LINE__,
				evt->param.suback.message_id);
		break;

	default:
		LOG_DBG("[%s:%d] default: %d\n", log_strdup(__func__), __LINE__,
				evt->type);
		break;
	}
}

/**@brief Resolves the configured hostname and
 * initializes the MQTT broker structure
 */
static void broker_init(void)
{
	int err;
	struct addrinfo *result;
	struct addrinfo *addr;
	struct addrinfo hints = {
		.ai_family = AF_INET,
		.ai_socktype = SOCK_STREAM
	};

	err = getaddrinfo(CONFIG_MQTT_BROKER_HOSTNAME, NULL, &hints, &result);
	if (err) {
		LOG_ERR("ERROR: getaddrinfo failed %d\n", err);

		return;
	}

	addr = result;
	err = -ENOENT;

	/* Look for address of the broker. */
	while (addr != NULL) {
		/* IPv4 Address. */
		if (addr->ai_addrlen == sizeof(struct sockaddr_in)) {
			struct sockaddr_in *broker4 =
				((struct sockaddr_in *)&broker);
			char ipv4_addr[NET_IPV4_ADDR_LEN];

			broker4->sin_addr.s_addr =
				((struct sockaddr_in *)addr->ai_addr)
				->sin_addr.s_addr;
			broker4->sin_family = AF_INET;
			broker4->sin_port = htons(CONFIG_MQTT_BROKER_PORT);

			inet_ntop(AF_INET, &broker4->sin_addr.s_addr,
				  ipv4_addr, sizeof(ipv4_addr));
			LOG_INF("IPv4 Address found %s\n", log_strdup(ipv4_addr));

			break;
		} else {
			LOG_WRN("ai_addrlen = %u should be %u or %u\n",
				(unsigned int)addr->ai_addrlen,
				(unsigned int)sizeof(struct sockaddr_in),
				(unsigned int)sizeof(struct sockaddr_in6));
		}

		addr = addr->ai_next;
		break;
	}

	/* Free the address. */
	freeaddrinfo(result);
}

/**@brief Initialize the MQTT client structure
 */
static void client_init(struct mqtt_client *client)
{
	mqtt_client_init(client);

	broker_init();

	/* MQTT client configuration */
	client->broker = &broker;
	client->evt_cb = mqtt_evt_handler;
	client->client_id.utf8 = (u8_t *)CONFIG_MQTT_CLIENT_ID;
	client->client_id.size = strlen(CONFIG_MQTT_CLIENT_ID);
	client->password = NULL;
	client->user_name = NULL;
	client->protocol_version = MQTT_VERSION_3_1_1;

	/* MQTT buffers configuration */
	client->rx_buf = rx_buffer;
	client->rx_buf_size = sizeof(rx_buffer);
	client->tx_buf = tx_buffer;
	client->tx_buf_size = sizeof(tx_buffer);

	/* MQTT transport configuration */
	client->transport.type = MQTT_TRANSPORT_NON_SECURE;
}

/**@brief Initialize the file descriptor structure used by poll.
 */
static int fds_init(struct mqtt_client *c)
{
	if (c->transport.type == MQTT_TRANSPORT_NON_SECURE) {
		fds.fd = c->transport.tcp.sock;
	} else {
#if defined(CONFIG_MQTT_LIB_TLS)
		fds.fd = c->transport.tls.sock;
#else
		return -ENOTSUP;
#endif
	}

	fds.events = POLLIN;

	return 0;
}


void mqtt_start_thread() {
    LOG_INF("--- starting mqtt thread ---\n");
	
	mqtt_thread_tid =
    k_thread_create(&mqtt_thread_data,
                    mqtt_stack_area,
                    K_THREAD_STACK_SIZEOF(mqtt_stack_area),
                    mqtt_thread,
                    NULL, NULL, NULL,
                    MQTT_THREAD_PRIORITY, 0, K_NO_WAIT);

}

int mqtt_connected(void) {
	return connected;
}

static void mqtt_thread(void *blank1, void *blank2, void *blank3)
{
	int err;

	client_init(&client);

	err = mqtt_connect(&client);
	if (err != 0) {
		LOG_ERR("ERROR: mqtt_connect %d\n", err);
		return;
	}

	err = fds_init(&client);
	if (err != 0) {
		LOG_ERR("ERROR: fds_init %d\n", err);
		return;
	}

	while (1) {
		err = poll(&fds, 1, mqtt_keepalive_time_left(&client));
		if (err < 0) {
			LOG_ERR("ERROR: poll %d\n", errno);
			break;
		}

		err = mqtt_live(&client);
		if ((err != 0) && (err != -EAGAIN)) {
			LOG_ERR("ERROR: mqtt_live %d\n", err);
			break;
		}

		if ((fds.revents & POLLIN) == POLLIN) {
			err = mqtt_input(&client);
			if (err != 0) {
				LOG_ERR("ERROR: mqtt_input %d\n", err);
				break;
			}
		}

		if ((fds.revents & POLLERR) == POLLERR) {
			LOG_ERR("POLLERR\n");
			break;
		}

		if ((fds.revents & POLLNVAL) == POLLNVAL) {
			LOG_ERR("POLLNVAL\n");
			break;
		}
	}

	LOG_INF("Disconnecting MQTT client...\n");

	err = mqtt_disconnect(&client);
	if (err) {
		LOG_ERR("Could not disconnect MQTT client. Error: %d\n", err);
	}
}
