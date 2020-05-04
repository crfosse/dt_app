/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <zephyr.h>
#include <zephyr/types.h>
#include <errno.h>
#include <net/socket.h>
#include <string.h>
#include <stdio.h>
#include <device.h>
#include <lte_helpers.h>
#include <at_cmd.h>
#include <at_cmd_parser/at_cmd_parser.h>
#include <at_cmd_parser/at_params.h>
#include <at_notif.h>
#include <logging/log.h>

LOG_MODULE_REGISTER(lte_helpers, CONFIG_APP_LOG_LEVEL);

#define LC_MAX_READ_LENGTH			128
#define AT_CMD_SIZE(x)				(sizeof(x) - 1)
#define AT_RESPONSE_PREFIX_INDEX		0
#define AT_CFUN_READ				"AT+CFUN?"
#define AT_CFUN_RESPONSE_PREFIX			"+CFUN"
#define AT_CFUN_MODE_INDEX			1
#define AT_CFUN_PARAMS_COUNT			2
#define AT_CFUN_RESPONSE_MAX_LEN		20
#define AT_CEREG_5				"AT+CEREG=5"
#define AT_CEREG_READ				"AT+CEREG?"
#define AT_CEREG_RESPONSE_PREFIX		"+CEREG"
#define AT_CEREG_PARAMS_COUNT_MAX		10
#define AT_CEREG_REG_STATUS_INDEX		1
#define AT_CEREG_READ_REG_STATUS_INDEX		2
#define AT_CEREG_ACTIVE_TIME_INDEX		8
#define AT_CEREG_TAU_INDEX			9
#define AT_CEREG_RESPONSE_MAX_LEN		80
#define AT_XSYSTEMMODE_READ			"AT%XSYSTEMMODE?"
#define AT_XSYSTEMMODE_RESPONSE_PREFIX		"%XSYSTEMMODE"
#define AT_XSYSTEMMODE_PROTO			"AT%%XSYSTEMMODE=%d,%d,%d,%d"
/* The indices are for the set command. Add 1 for the read command indices. */
#define AT_XSYSTEMMODE_LTEM_INDEX		0
#define AT_XSYSTEMMODE_NBIOT_INDEX		1
#define AT_XSYSTEMMODE_GPS_INDEX		2
#define AT_XSYSTEMMODE_PARAMS_COUNT		5
#define AT_XSYSTEMMODE_RESPONSE_MAX_LEN		30
/* Parameter values for AT+CEDRXS command. */
#define AT_CEDRXS_ACTT_WB			4
#define AT_CEDRXS_ACTT_NB			5

/* Forward declarations */
static bool response_is_valid(const char *response, size_t response_len,
			      const char *check);

/* Lookup table for T3324 timer used for PSM active time. Unit is seconds.
 * Ref: GPRS Timer 2 IE in 3GPP TS 24.008 Table 10.5.163/3GPP TS 24.008.
 */
static const u32_t t3324_lookup[8] = {2, 60, 600, 60, 60, 60, 60, 0};

/* Lookup table for T3412 timer used for periodic TAU. Unit is seconds.
 * Ref: GPRS Timer 3 IE in 3GPP TS 24.008 Table 10.5.163a/3GPP TS 24.008.
 */
static const u32_t t3412_lookup[8] = {600, 3600, 36000, 2, 30, 60,
				      1152000, 0};


/* Subscribes to notifications with level 5 */
static const char cereg_5_subscribe[] = AT_CEREG_5;

static at_cmd_handler_t at_callback_handler(const char *response) {
    int err;
    struct at_param_list at_resp_list = {0};
	char timer_str[9] = {0};
	char unit_str[4] = {0};
	size_t timer_str_len = sizeof(timer_str) - 1;
	size_t unit_str_len = sizeof(unit_str) - 1;
	size_t index;
	u32_t timer_unit, timer_value;

    int tau, active_time;


    err = at_params_list_init(&at_resp_list, AT_CEREG_PARAMS_COUNT_MAX);
	if (err) {
		LOG_ERR("Could not init AT params list, error: %d", err);
		return err;
	}

	err = at_parser_max_params_from_str(response,
					    NULL,
					    &at_resp_list,
					    AT_CEREG_PARAMS_COUNT_MAX);
	if (err) {
		LOG_ERR("Could not parse AT+CEREG response, error: %d", err);
		goto parse_psm_clean_exit;
	}

	/* Parse periodic TAU string */
	err = at_params_string_get(&at_resp_list,
				   AT_CEREG_TAU_INDEX,
				   timer_str,
				   &timer_str_len);
	if (err) {
		LOG_ERR("Could not get TAU, error: %d", err);
		goto parse_psm_clean_exit;
	}

	memcpy(unit_str, timer_str, unit_str_len);

	index = strtoul(unit_str, NULL, 2);
	if (index > (ARRAY_SIZE(t3412_lookup) - 1)) {
		LOG_ERR("Unable to parse periodic TAU string");
		err = -EINVAL;
		goto parse_psm_clean_exit;
	}

	timer_unit = t3412_lookup[index];
	timer_value = strtoul(timer_str + unit_str_len, NULL, 2);
	tau = timer_unit ? timer_unit * timer_value : -1;

	/* Parse active time string */
	err = at_params_string_get(&at_resp_list,
				   AT_CEREG_ACTIVE_TIME_INDEX,
				   timer_str,
				   &timer_str_len);
	if (err) {
		LOG_ERR("Could not get TAU, error: %d", err);
		goto parse_psm_clean_exit;
	}

	memcpy(unit_str, timer_str, unit_str_len);

	index = strtoul(unit_str, NULL, 2);
	if (index > (ARRAY_SIZE(t3324_lookup) - 1)) {
		LOG_ERR("Unable to parse active time string");
		err = -EINVAL;
		goto parse_psm_clean_exit;
	}

	timer_unit = t3324_lookup[index];
	timer_value = strtoul(timer_str + unit_str_len, NULL, 2);
	active_time = timer_unit ? timer_unit * timer_value : -1;

	LOG_DBG("TAU: %d sec, active time: %d sec\n", tau, active_time);

parse_psm_clean_exit:
	at_params_list_free(&at_resp_list);

	return err;
}

int lte_lc_psm_get_with_cb(void)
{
	int err;
	
	/* Enable network registration status with PSM information */
	err = at_cmd_write(AT_CEREG_5, NULL, 0, NULL);
	if (err) {
		LOG_ERR("Could not set CEREG, error: %d", err);
		return err;
	}

	/* Read network registration status */
	err = at_cmd_write_with_callback(AT_CEREG_READ, at_callback_handler, NULL);
	if (err) {
		LOG_ERR("Could not get CEREG response, error: %d", err);
		return err;
	}
}

/**@brief Helper function to check if a response is what was expected
 *
 * @param response Pointer to response prefix
 * @param response_len Length of the response to be checked
 * @param check The buffer with "truth" to verify the response against,
 *		for example "+CEREG"
 *
 * @return True if the provided buffer and check are equal, false otherwise.
 */
static bool response_is_valid(const char *response, size_t response_len,
			      const char *check)
{
	if ((response == NULL) || (check == NULL)) {
		LOG_ERR("Invalid pointer provided");
		return false;
	}

	if ((response_len < strlen(check)) ||
	    (memcmp(response, check, response_len) != 0)) {
		return false;
	}

	return true;
}
