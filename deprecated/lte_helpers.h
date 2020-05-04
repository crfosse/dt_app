/**
 * @file lte_lc.h
 *
 * @brief Public APIs for the LTE Link Control driver.
 */

/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */
#ifndef ZEPHYR_INCLUDE_LTE_HELPERS_H_
#define ZEPHYR_INCLUDE_LTE_HELPERS_H_

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

int lte_lc_psm_get_with_cb(void);

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_LTE_HELPERS_H_ */