/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <stdio.h>
#include <string.h>

#include <zephyr.h>
#include <net/coap.h>
#include <net/socket.h>
#include <lte_lc.h>
#include <logging/log.h>

#include <dk_buttons_and_leds.h>
#include <modem_info.h>

#include "coap_module.h"
#if defined(CONFIG_BSD_LIBRARY)

/**@brief Recoverable BSD library error. */
void bsd_recoverable_error_handler(uint32_t err)
{
	printk("bsdlib recoverable error: %u\n", (unsigned int)err);
}

#endif /* defined(CONFIG_BSD_LIBRARY) */


LOG_MODULE_REGISTER(app_coap_main, CONFIG_APP_LOG_LEVEL);

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

		printk("LTE Link Connecting ...\n");
		err = lte_lc_init_and_connect();
		__ASSERT(err == 0, "LTE link could not be established.");
		printk("LTE Link Connected!\n");
	}
#endif /* defined(CONFIG_LTE_LINK_CONTROL) */
}



void main(void)
{

	LOG_INF("---- Starting CoAP example ----");

	modem_configure();

	coap_start_thread();

	while(!coap_connected) {
		LOG_INF("Waiting for connection...");
		k_sleep(1000);
	}

	while(1) {
		k_sleep(1000);
	}

}
