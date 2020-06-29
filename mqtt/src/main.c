/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <zephyr.h>
#include <stdio.h>
#include <string.h>
#include <logging/log.h>

#include <net/mqtt.h>
#include <net/socket.h>
#include <lte_lc.h>

#include <dk_buttons_and_leds.h>

#include "mqtt_module.h"

LOG_MODULE_REGISTER(app_mqtt_main, CONFIG_APP_LOG_LEVEL);

/* Application settings */
#define WITH_PSM
#define TEST_PERIODIC
//#define TEST_SWEEP


/* Test settings */
#define SAMPLE_INTERVAL      1 //Seconds
#define SWEEP_INTERVAL      40 //Seconds
#define SIZE_STEP           64 //Bytes  
#define MAX_SAMPLES          5
#define PERIODIC_MSG_SIZE 4096 //Bytes

// The periodic TAU given by the network
static int actual_tau = 10; 

/* Indicates when the application is ready to transmit *
*  according to the periodic TAU					   */
static bool transmit = false;

// Actual maximum payload size
#define TEST_DATA_SIZE 4096

//255 random characters in an array for upload testing
u8_t * testData = "yK3vQHgUQ1WBUNPGprMSh0o1ZOTpGzC788DMB0OoQytSTDmKo7zeybWdx1DGh3SX"
				  "IpfkYHSkX3hQuEUdWC8jWBq6qRAzv4NB79aECZwwUsReylQcJOzZ4NW1rY3xbyya"
				  "ep9DOEWnRsWkjILrSh4crLHlfvmqVLxRjA1dDvHx72JVD4rvhhLbcQ6Gi94lvVF7"
				  "0KmnO4Lh7IRGUm37TVXzQXtRnb228WPngoCC5Ge4GZNmBRFhXWtgeuU9Vt2JJbID"
				  "jvdEpZVL88RszUn7Ah4pnTC7rkHRft6apfuZCUqo0udcvNbEaUFncwjsU8zkw8j7"
				  "mfiD7QxF2A9Kv7XTztxef2Ryj1MbWe0vDAPXUz3yb4AqfgcxPb3TCocDCgAd2F2S"
				  "xlAZ69913oxzD0M24Sl0YIztsCnTuUrzrgIOrdXWvjOcEcuEJltiIZMygVx8gxwc"
				  "pwY4YNybojiLfuRET4w91tbTgn33IvFcY8J7tu5Y8LZjk5ZfkekJg5zhZs6Bo2Jm"
				  "N0mC7eCqYvSBGm4No2TPbLjYD2fB5ERubVuo2rGeZjbnWEx8jcP9jgq049pEjjS9"
				  "MRXvJnDtpo8hIZcZpz1HKyXbOXbz60baSbpW5RHOhwg1TBh8wrBTOOORMMCBhl4O"
				  "QApYjcf2w4ZlbyfWUjQY6gkGR21599Wb1IjraQL911QeFjiRFGtcDEpxo5GMWL1O"
				  "ZKM4Gnkp4LP0A0yK9FlHeopsaCOBxOI0dTaq2gWDD8rRRCbYykck0J5IZfQnrBbv"
            	  "AH1MSzQuBq5BLjPC6KhWj519pymLg11fSvhgWlOnhfuSNlmqq9pysYmZIPUNKGOP"
				  "9gfpEKm8tCuvpUWZvoFsrmxfYQNe9vUznG0PZMhHDSc5C6wDBpFqDBhHEHRdg0KR"
				  "Df8CU5RsaaviBtI8yFb0plaRQjzTYg2xZcppX4NANeqB0udVdEdfhIxX6iVXcEb5"
				  "lGY0a35dDRjCgL7ePgZn7oQbLuusUurDbprEu2msxDXz94KJPwhnMldretN5bgq7"
				  "yK3vQHgUQ1WBUNPGprMSh0o1ZOTpGzC788DMB0OoQytSTDmKo7zeybWdx1DGh3SX"
				  "IpfkYHSkX3hQuEUdWC8jWBq6qRAzv4NB79aECZwwUsReylQcJOzZ4NW1rY3xbyya"
				  "ep9DOEWnRsWkjILrSh4crLHlfvmqVLxRjA1dDvHx72JVD4rvhhLbcQ6Gi94lvVF7"
				  "0KmnO4Lh7IRGUm37TVXzQXtRnb228WPngoCC5Ge4GZNmBRFhXWtgeuU9Vt2JJbID"
				  "jvdEpZVL88RszUn7Ah4pnTC7rkHRft6apfuZCUqo0udcvNbEaUFncwjsU8zkw8j7"
				  "mfiD7QxF2A9Kv7XTztxef2Ryj1MbWe0vDAPXUz3yb4AqfgcxPb3TCocDCgAd2F2S"
				  "xlAZ69913oxzD0M24Sl0YIztsCnTuUrzrgIOrdXWvjOcEcuEJltiIZMygVx8gxwc"
				  "pwY4YNybojiLfuRET4w91tbTgn33IvFcY8J7tu5Y8LZjk5ZfkekJg5zhZs6Bo2Jm"
				  "N0mC7eCqYvSBGm4No2TPbLjYD2fB5ERubVuo2rGeZjbnWEx8jcP9jgq049pEjjS9"
				  "MRXvJnDtpo8hIZcZpz1HKyXbOXbz60baSbpW5RHOhwg1TBh8wrBTOOORMMCBhl4O"
				  "QApYjcf2w4ZlbyfWUjQY6gkGR21599Wb1IjraQL911QeFjiRFGtcDEpxo5GMWL1O"
				  "ZKM4Gnkp4LP0A0yK9FlHeopsaCOBxOI0dTaq2gWDD8rRRCbYykck0J5IZfQnrBbv"
            	  "AH1MSzQuBq5BLjPC6KhWj519pymLg11fSvhgWlOnhfuSNlmqq9pysYmZIPUNKGOP"
				  "9gfpEKm8tCuvpUWZvoFsrmxfYQNe9vUznG0PZMhHDSc5C6wDBpFqDBhHEHRdg0KR"
				  "Df8CU5RsaaviBtI8yFb0plaRQjzTYg2xZcppX4NANeqB0udVdEdfhIxX6iVXcEb5"
				  "lGY0a35dDRjCgL7ePgZn7oQbLuusUurDbprEu2msxDXz94KJPwhnMldretN5bgq7"
				  "yK3vQHgUQ1WBUNPGprMSh0o1ZOTpGzC788DMB0OoQytSTDmKo7zeybWdx1DGh3SX"
				  "IpfkYHSkX3hQuEUdWC8jWBq6qRAzv4NB79aECZwwUsReylQcJOzZ4NW1rY3xbyya"
				  "ep9DOEWnRsWkjILrSh4crLHlfvmqVLxRjA1dDvHx72JVD4rvhhLbcQ6Gi94lvVF7"
				  "0KmnO4Lh7IRGUm37TVXzQXtRnb228WPngoCC5Ge4GZNmBRFhXWtgeuU9Vt2JJbID"
				  "jvdEpZVL88RszUn7Ah4pnTC7rkHRft6apfuZCUqo0udcvNbEaUFncwjsU8zkw8j7"
				  "mfiD7QxF2A9Kv7XTztxef2Ryj1MbWe0vDAPXUz3yb4AqfgcxPb3TCocDCgAd2F2S"
				  "xlAZ69913oxzD0M24Sl0YIztsCnTuUrzrgIOrdXWvjOcEcuEJltiIZMygVx8gxwc"
				  "pwY4YNybojiLfuRET4w91tbTgn33IvFcY8J7tu5Y8LZjk5ZfkekJg5zhZs6Bo2Jm"
				  "N0mC7eCqYvSBGm4No2TPbLjYD2fB5ERubVuo2rGeZjbnWEx8jcP9jgq049pEjjS9"
				  "MRXvJnDtpo8hIZcZpz1HKyXbOXbz60baSbpW5RHOhwg1TBh8wrBTOOORMMCBhl4O"
				  "QApYjcf2w4ZlbyfWUjQY6gkGR21599Wb1IjraQL911QeFjiRFGtcDEpxo5GMWL1O"
				  "ZKM4Gnkp4LP0A0yK9FlHeopsaCOBxOI0dTaq2gWDD8rRRCbYykck0J5IZfQnrBbv"
            	  "AH1MSzQuBq5BLjPC6KhWj519pymLg11fSvhgWlOnhfuSNlmqq9pysYmZIPUNKGOP"
				  "9gfpEKm8tCuvpUWZvoFsrmxfYQNe9vUznG0PZMhHDSc5C6wDBpFqDBhHEHRdg0KR"
				  "Df8CU5RsaaviBtI8yFb0plaRQjzTYg2xZcppX4NANeqB0udVdEdfhIxX6iVXcEb5"
				  "lGY0a35dDRjCgL7ePgZn7oQbLuusUurDbprEu2msxDXz94KJPwhnMldretN5bgq7"
				  "yK3vQHgUQ1WBUNPGprMSh0o1ZOTpGzC788DMB0OoQytSTDmKo7zeybWdx1DGh3SX"
				  "IpfkYHSkX3hQuEUdWC8jWBq6qRAzv4NB79aECZwwUsReylQcJOzZ4NW1rY3xbyya"
				  "ep9DOEWnRsWkjILrSh4crLHlfvmqVLxRjA1dDvHx72JVD4rvhhLbcQ6Gi94lvVF7"
				  "0KmnO4Lh7IRGUm37TVXzQXtRnb228WPngoCC5Ge4GZNmBRFhXWtgeuU9Vt2JJbID"
				  "jvdEpZVL88RszUn7Ah4pnTC7rkHRft6apfuZCUqo0udcvNbEaUFncwjsU8zkw8j7"
				  "mfiD7QxF2A9Kv7XTztxef2Ryj1MbWe0vDAPXUz3yb4AqfgcxPb3TCocDCgAd2F2S"
				  "xlAZ69913oxzD0M24Sl0YIztsCnTuUrzrgIOrdXWvjOcEcuEJltiIZMygVx8gxwc"
				  "pwY4YNybojiLfuRET4w91tbTgn33IvFcY8J7tu5Y8LZjk5ZfkekJg5zhZs6Bo2Jm"
				  "N0mC7eCqYvSBGm4No2TPbLjYD2fB5ERubVuo2rGeZjbnWEx8jcP9jgq049pEjjS9"
				  "MRXvJnDtpo8hIZcZpz1HKyXbOXbz60baSbpW5RHOhwg1TBh8wrBTOOORMMCBhl4O"
				  "QApYjcf2w4ZlbyfWUjQY6gkGR21599Wb1IjraQL911QeFjiRFGtcDEpxo5GMWL1O"
				  "ZKM4Gnkp4LP0A0yK9FlHeopsaCOBxOI0dTaq2gWDD8rRRCbYykck0J5IZfQnrBbv"
            	  "AH1MSzQuBq5BLjPC6KhWj519pymLg11fSvhgWlOnhfuSNlmqq9pysYmZIPUNKGOP"
				  "9gfpEKm8tCuvpUWZvoFsrmxfYQNe9vUznG0PZMhHDSc5C6wDBpFqDBhHEHRdg0KR"
				  "Df8CU5RsaaviBtI8yFb0plaRQjzTYg2xZcppX4NANeqB0udVdEdfhIxX6iVXcEb5"
				  "lGY0a35dDRjCgL7ePgZn7oQbLuusUurDbprEu2msxDXz94KJPwhnMldretN5bgq";		



#if defined(CONFIG_BSD_LIBRARY)
/**@brief Recoverable BSD library error. */
void bsd_recoverable_error_handler(uint32_t err)
{
	LOG_ERR("bsdlib recoverable error: %u\n", (unsigned int)err);
}

#endif /* defined(CONFIG_BSD_LIBRARY) */

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

/* @brief returns a random "sample"*/
static u8_t sensor_data_get() {
	u8_t random_sample;
	
	random_sample = sys_rand32_get() % 255;

	return random_sample;
}

/**@brief Callback for button events from the DK buttons and LEDs library. */
static void button_handler(u32_t button_states, u32_t has_changed)
{
	u8_t sample = 0;
	int err;
	 
	if (has_changed & button_states & DK_BTN1_MSK) {
		LOG_INF("DEV_DBG: button 1 pressed\n");

		// alarm inducer
		sample = sensor_data_get();
		err = mqtt_data_publish(&sample,1);

		if (err < 0) {
			LOG_ERR("MQTT_PUBLISH ret %d", err);
			return;
		}
	}
	else if (has_changed & button_states & DK_BTN2_MSK) {
	
	}

	return;
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

/* Requests the configured PSM values from the network */
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

/* @brief triggers every minute. Set transmit true if one periodic tau period has passed */ 
void app_timer_handler(struct k_timer *dummy)
{
	static u32_t minutes;

	minutes++;
	/* This shall match the PSM interval*/
	if (minutes % actual_tau == 0) {
		LOG_INF("Awake - transmit true");
		transmit = true;
	}
	LOG_INF("Elapsed time: %d\n", minutes);
}

K_TIMER_DEFINE(app_timer, app_timer_handler, NULL);

/* @brief initializes timer that triggers every minute */
void timer_init(void)
{
	k_timer_start(&app_timer, K_MINUTES(1), K_MINUTES(1));
}


#ifdef TEST_PERIODIC
static int run_periodic(void) {
	static bool transmit_finished = false;
	static u8_t sample_cnt = 1;

		if (transmit) {
			//Lighting LED2 to indicate that transmission is initiated
			dk_set_led(DK_LED2, 0);

			//Data upload
			int err = mqtt_data_publish(testData,PERIODIC_MSG_SIZE);
			if (err < 0) {
				LOG_ERR("MQTT publish error: %d", err);
				return err;
			}

			transmit_finished = true;
		}

		k_sleep(K_SECONDS(SAMPLE_INTERVAL));

		if(transmit && transmit_finished) {
			//Transmission phase over.
			dk_set_led(DK_LED2, 1);
			transmit = false;
			transmit_finished = false;


			if(sample_cnt >= MAX_SAMPLES) {
				//exit test
				return -1;
			}

			sample_cnt++;
		}

		return 0;
}
#endif

#ifdef TEST_SWEEP
static int run_size_sweep(void) {
	static size_t test_index = 0;

	//Lighting LED2 to indicate that transmission is initiated

	LOG_INF("Current test_index: %d", test_index);
	if(test_index < TEST_DATA_SIZE) {
		LOG_INF("Transmit");
		int err = mqtt_data_publish(testData,test_index);
		if (err < 0) {
				LOG_ERR("MQTT_PUBLISH ret %d", err);
				return err;
		}
		test_index += SIZE_STEP;
	} else {
		//Ending test
		test_index = 0;
		return -1;
	}

	return 0;
}
#endif

void main(void)
{
	LOG_INF("\nDT MQTT application example started\n");

	int err;

	buttons_leds_init();

	#ifdef WITH_PSM
	setup_psm();
	#else
	/* Force PSM off in case the modem uses old PSM settings */
	err = lte_lc_psm_req(false);
	if (err) {
        LOG_ERR("ERROR: set psm %d\n", err);
        return;
    }
	#endif

	modem_configure();

	#ifdef WITH_PSM
	// The network can provide other PSM values. So we fetch the actual values of the network 
	int curr_active;
	err = lte_lc_psm_get(&actual_tau, &curr_active);
	LOG_INF("Reqested: TAU = %s | AT = %s", log_strdup(CONFIG_LTE_PSM_REQ_RPTAU), log_strdup(CONFIG_LTE_PSM_REQ_RAT));
	LOG_INF("Got: TAU = %d | AT = %d", actual_tau, curr_active);
	#endif

	/* Initialize MQTT connection */
	mqtt_start_thread();
	while(!mqtt_connected()) {
		k_sleep(100);
	}

	#ifdef TEST_PERIODIC
	// Converting TAU to minutes
	actual_tau = actual_tau/60;
	timer_init();
	#endif

	if(err) {
		LOG_ERR("Initialization error");
		return;
	}

	#ifdef TEST_SWEEP
	k_sleep(K_SECONDS(SWEEP_INTERVAL));
	#endif

	//Lighting LED1 to indicate that the application is connected and enterin main loop.
	dk_set_led(DK_LED1, 0);

	while(1) {
		#ifdef TEST_PERIODIC
			err = run_periodic();
			if (err < 0) {
				dk_set_led(DK_LED3, 0);
				LOG_ERR("Error or finished periodic");
				return;
			}
		#elif defined(TEST_SWEEP)
			dk_set_led(DK_LED2, 0);
			if (run_size_sweep() < 0) {
				dk_set_led(DK_LED3, 0);
				LOG_ERR("Error or finished sweep");
				return;
			}
			dk_set_led(DK_LED2, 1);
			k_sleep(K_SECONDS(SWEEP_INTERVAL));
		#else
			k_sleep(K_SECONDS(SAMPLE_INTERVAL));
		#endif
	}
}