/*
 * Copyright (c) 2025 Noah Luskey <noah@xv.engineering>
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef __TESTS_TERMINAL_DISPLAY_UTILS_H__
#define __TESTS_TERMINAL_DISPLAY_UTILS_H__

#include <stdint.h>

void hsv_to_rgb(double h, double s, double v, uint8_t *r, uint8_t *g, uint8_t *b);

#endif
