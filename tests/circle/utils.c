/*
 * Copyright (c) 2025 Noah Luskey <noah@xv.engineering>
 *
 * SPDX-License-Identifier: MIT
 */
#include "utils.h"
#include <zephyr/sys/__assert.h>
#include <stdbool.h>

void hsv_to_rgb(double h, double s, double v, uint8_t *r, uint8_t *g, uint8_t *b)
{
    double r_tmp, g_tmp, b_tmp;
    int h_i = (int)(h / 60) % 6;
    double f = (h / 60) - h_i;
    double p = v * (1 - s);
    double q = v * (1 - f * s);
    double t = v * (1 - (1 - f) * s);

    switch (h_i)
    {
    case 0:
        r_tmp = v;
        g_tmp = t;
        b_tmp = p;
        break;
    case 1:
        r_tmp = q;
        g_tmp = v;
        b_tmp = p;
        break;
    case 2:
        r_tmp = p;
        g_tmp = v;
        b_tmp = t;
        break;
    case 3:
        r_tmp = p;
        g_tmp = q;
        b_tmp = v;
        break;
    case 4:
        r_tmp = t;
        g_tmp = p;
        b_tmp = v;
        break;
    case 5:
        r_tmp = v;
        g_tmp = p;
        b_tmp = q;
        break;
    default:
        __ASSERT_NO_MSG(false);
        return;
    }

    // Scale to 0-255 range
    *r = (uint8_t)(r_tmp * 255);
    *g = (uint8_t)(g_tmp * 255);
    *b = (uint8_t)(b_tmp * 255);
}
