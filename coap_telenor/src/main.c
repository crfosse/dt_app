#include <zephyr.h>
#include <logging/log.h>

#include <dk_buttons_and_leds.h>
#include <lte_lc.h>

#include "coap.h"

LOG_MODULE_REGISTER(app_main, CONFIG_APP_LOG_LEVEL);

#define SAMPLE_INTERVAL 1

// The periodic TAU given by the network
static int actual_tau; 

/* Indicates when the application is ready to transmit *
*  according to the periodic TAU					   */
static bool transmit = false;

#define TEST_DATA_SIZE 1024

//255 random characters in an array for upload testing
u8_t * testData = "yK3vQHgUQ1WBUNPGprMSh0o1ZOTpGzC788DMB0OoQytSTDmKo7zeybWdx1DGh3SXIpfkYHSkX3hQuEUdWC8jWBq6qRAzv4NB79aECZwwUsReylQcJOzZ4NW1rY3xbyyaep9DOEWnRsWkjILrSh4crLHlfvmqVLxRjA1dDvHx72JVD4rvhhLbcQ6Gi94lvVF70KmnO4Lh7IRGUm37TVXzQXtRnb228WPngoCC5Ge4GZNmBRFhXWtgeuU9Vt2JJbID"
				  "jvdEpZVL88RszUn7Ah4pnTC7rkHRft6apfuZCUqo0udcvNbEaUFncwjsU8zkw8j7mfiD7QxF2A9Kv7XTztxef2Ryj1MbWe0vDAPXUz3yb4AqfgcxPb3TCocDCgAd2F2SxlAZ69913oxzD0M24Sl0YIztsCnTuUrzrgIOrdXWvjOcEcuEJltiIZMygVx8gxwcpwY4YNybojiLfuRET4w91tbTgn33IvFcY8J7tu5Y8LZjk5ZfkekJg5zhZs6Bo2Jm"
				  "N0mC7eCqYvSBGm4No2TPbLjYD2fB5ERubVuo2rGeZjbnWEx8jcP9jgq049pEjjS9MRXvJnDtpo8hIZcZpz1HKyXbOXbz60baSbpW5RHOhwg1TBh8wrBTOOORMMCBhl4OQApYjcf2w4ZlbyfWUjQY6gkGR21599Wb1IjraQL911QeFjiRFGtcDEpxo5GMWL1OZKM4Gnkp4LP0A0yK9FlHeopsaCOBxOI0dTaq2gWDD8rRRCbYykck0J5IZfQnrBbv"
            	  "AH1MSzQuBq5BLjPC6KhWj519pymLg11fSvhgWlOnhfuSNlmqq9pysYmZIPUNKGOP9gfpEKm8tCuvpUWZvoFsrmxfYQNe9vUznG0PZMhHDSc5C6wDBpFqDBhHEHRdg0KRDf8CU5RsaaviBtI8yFb0plaRQjzTYg2xZcppX4NANeqB0udVdEdfhIxX6iVXcEb5lGY0a35dDRjCgL7ePgZn7oQbLuusUurDbprEu2msxDXz94KJPwhnMldretN5bgq7";


static int message_post(struct coap_resource *resource, struct coap_packet *request, struct sockaddr *addr, socklen_t addr_len) {
	coap_endpoint *coap = resource->user_data;

	u16_t payload_len;
	const u8_t *payload = coap_packet_get_payload(request, &payload_len);

	u8_t *buf = k_calloc(payload_len + 1, 1);
	memcpy(buf, payload, payload_len);
	LOG_INF("Received CoAP POST: %s", log_strdup(buf));
	k_free(buf);

	int err = coap_endpoint_respond(coap, request, COAP_RESPONSE_CODE_CREATED, NULL, 0, addr, addr_len);
	if (err != 0) {
		LOG_ERR("coap_endpoint_respond: %d", err);
	}

	return 0;
}

static const char * const message_path[] = { "message", NULL };
static struct coap_resource resources[] = {
	{
		path: message_path,
		post: message_post,
	},
	{},
};
static coap_endpoint *coap;
struct sockaddr_in 	remote_addr = {
		.sin_family = AF_INET,
		.sin_port = htons(5683),
};
static const char * const path[] = { "straight", "and", "narrow", NULL };


/**@brief Callback for button events from the DK buttons and LEDs library. */
static void button_handler(u32_t button_states, u32_t has_changed)
{
	 static size_t test_index = 0;
	 
	if (has_changed & button_states & DK_BTN1_MSK) {
		LOG_INF("DEV_DBG: button 1 pressed\n");
		LOG_INF("Current test_index: %d", test_index);
		if(test_index < TEST_DATA_SIZE) {
				int ret = coap_endpoint_post(coap, (struct sockaddr *)&remote_addr, sizeof(remote_addr), path, testData, test_index);
				if (ret != COAP_RESPONSE_CODE_CREATED) {
					LOG_ERR("coap_endpoint_post: %d", ret);
					return;
				}
			test_index += 5;
		} else {
			test_index = 0;
		}
	}
	else if (has_changed & button_states & DK_BTN2_MSK) {
		//resetting the test
		test_index = 0;
	}
	return;
}

/**@brief Configures modem to provide LTE link. Blocks until link is
 * successfully established.
 */
static void modem_configure(void)
{
#if defined(CONFIG_LTE_LINK_CONTROL)
	if (IS_ENABLED(CONFIG_LTE_AUTO_INIT_AND_CONNECT)) {
		/* Do nothing, modem is already turned on
		 * and connected.
		 */
	} else {
		int err;

		LOG_INF("LTE Link Connecting ...\n");
		err = lte_lc_init_and_connect();
		__ASSERT(err == 0, "LTE link could not be established.");
		LOG_INF("LTE Link Connected!\n");
	}
#endif /* defined(CONFIG_LTE_LINK_CONTROL) */
}

/**@brief Initializes buttons and LEDs, using the DK buttons and LEDs
 * library.
 */
static void buttons_leds_init(void)
{
	int err;

	LOG_INF("DEV_DBG: Initalizing buttons and leds.\n");

	err = dk_buttons_init(button_handler);
	if (err) {
		LOG_ERR("Could not initialize buttons, err code: %d\n", err);
	}

	err = dk_leds_init();
	if (err) {
		LOG_ERR("Could not initialize leds, err code: %d\n", err);
	}

	err = dk_set_leds_state(DK_ALL_LEDS_MSK, DK_NO_LEDS_MSK);
	if (err) {
		LOG_ERR("Could not set leds state, err code: %d\n", err);
	}
}

/* @brief returns a random "sample"*/
static u8_t sensor_data_get() {
	u8_t random_sample;
	
	random_sample = sys_rand32_get() % 255;

	return random_sample;
}


void setup_psm(void)
{
	/*
	* GPRS Timer 3 value (octet 3)
	*
	* Bits 5 to 1 represent the binary coded timer value.
	*
	* Bits 6 to 8 defines the timer value unit for the GPRS timer as follows:
	* Bits 
	* 8 7 6
	* 0 0 0 value is incremented in multiples of 10 minutes 
	* 0 0 1 value is incremented in multiples of 1 hour 
	* 0 1 0 value is incremented in multiples of 10 hours
	* 0 1 1 value is incremented in multiples of 2 seconds
	* 1 0 0 value is incremented in multiples of 30 seconds
	* 1 0 1 value is incremented in multiples of 1 minute
	* 1 1 0 value is incremented in multiples of 320 hours (NOTE 1)
	* 1 1 1 value indicates that the timer is deactivated (NOTE 2).
	*/
	char psm_settings[] = CONFIG_LTE_PSM_REQ_RPTAU;
	printk("PSM bits: %c%c%c\n", psm_settings[0], psm_settings[1],
	       psm_settings[2]);
	printk("PSM Interval: %c%c%c%c%c\n", psm_settings[3], psm_settings[4],
	       psm_settings[5], psm_settings[6], psm_settings[7]);
	int err = lte_lc_psm_req(true);
	if (err < 0) {
		printk("Error setting PSM: %d Errno: %d\n", err, errno);
	}
}

void app_timer_handler(struct k_timer *dummy)
{
	static u32_t minutes;

	minutes++;
	/* This shall match the PSM interval*/
	if (minutes % actual_tau == 0) {
		transmit = true;
		LOG_INF("Ready for transmit");
	}
	LOG_INF("Elapsed time: %d\n", minutes);
}

K_TIMER_DEFINE(app_timer, app_timer_handler, NULL);

/* @brief initializes timer that triggers every minute */
void timer_init(void)
{
	k_timer_start(&app_timer, K_MINUTES(1), K_MINUTES(1));
}

void init_endpoint(void) {
	struct sockaddr_in local_addr = {
		.sin_family = AF_INET,
		.sin_port = htons(5683),
	};
	coap = coap_endpoint_init((struct sockaddr *)&local_addr, sizeof(local_addr), resources);
	if (coap == NULL) {
		LOG_ERR("coap_endpoint_init");
		return;
	}

	resources[0].user_data = coap;

	net_addr_pton(AF_INET, "172.16.15.14", &remote_addr.sin_addr);
}

void main() {
	LOG_INF("CoAP sample application started");
	buttons_leds_init();


	setup_psm();


	modem_configure();
	init_endpoint();

	//Waiting for the network to respond to the PSM request
	//LOG_INF("Waiting for network response for PSM");
	//k_sleep(K_SECONDS(30));

	// The network can provide other PSM values. So we fetch the actual values of the network 
	int curr_active;
	lte_lc_psm_get(&actual_tau, &curr_active);
	//lte_lc_psm_get_with_cb();
	LOG_INF("Reqested: TAU = %s | AT = %s", log_strdup(CONFIG_LTE_PSM_REQ_RPTAU), log_strdup(CONFIG_LTE_PSM_REQ_RAT));
	LOG_INF("Got: TAU = %d | AT = %d", actual_tau, curr_active);

	// Converting TAU to minutes
	actual_tau = actual_tau/60;

	timer_init();

	u8_t current_sample;
	bool transmit_finished = false;

	//Lighting LED1 to indicate that the application is connected and enterin main loop.
	dk_set_led(DK_LED1, 0);
	while(1) {
		current_sample = sensor_data_get();
		if (transmit) {
			//Lighting LED2 to indicate that transmission is initiated
			dk_set_led(DK_LED2, 0);

			//Data upload
			int ret = coap_endpoint_post(coap, (struct sockaddr *)&remote_addr, sizeof(remote_addr), path, &current_sample, 1);
				if (ret != COAP_RESPONSE_CODE_CREATED) {
					LOG_ERR("coap_endpoint_post: %d", ret);
					return;
			}

			//LOG_INF("periodic,%s,%d,%d", log_strdup(modem_info_buff), rsrp.value, sample_acc/sample_cnt);
			LOG_INF("periodic : %d", current_sample);
			
			transmit_finished = true;
		}

		k_sleep(K_SECONDS(SAMPLE_INTERVAL));

		if(transmit && transmit_finished) {
			//Transmission phase over.
			dk_set_led(DK_LED2, 1);
			transmit = false;
			transmit_finished = false;
		}
	}
}
