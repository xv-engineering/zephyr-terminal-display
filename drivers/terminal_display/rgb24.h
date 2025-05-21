/*
 * Copyright (c) 2025 Noah Luskey <noah@xv.engineering>
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef __TERMINAL_DISPLAY_RGB24_H__
#define __TERMINAL_DISPLAY_RGB24_H__

#include <stdint.h>
#include <stdbool.h>

struct rgb24
{
    uint8_t r;
    uint8_t g;
    uint8_t b;
};

/* returns true if the colors are the same */
bool rgb24_equal(const struct rgb24 *a, const struct rgb24 *b);

/* returns true if the color is grayscale */
bool rgb24_is_grayscale(const struct rgb24 *color);

/* converts to the closest 256-color code */
uint8_t rgb24_to_256(const struct rgb24 *color);

#endif // RGB24_H
