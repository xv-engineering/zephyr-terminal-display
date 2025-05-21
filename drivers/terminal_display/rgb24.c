/*
 * Copyright (c) 2025 Noah Luskey <noah@xv.engineering>
 *
 * SPDX-License-Identifier: MIT
 */
#include "rgb24.h"
#include <zephyr/sys/__assert.h>
#include <stddef.h>
#include <stdlib.h>

bool rgb24_equal(const struct rgb24 *a, const struct rgb24 *b)
{
    __ASSERT_NO_MSG(a != NULL);
    __ASSERT_NO_MSG(b != NULL);
    return a->r == b->r && a->g == b->g && a->b == b->b;
}

bool rgb24_is_grayscale(const struct rgb24 *color)
{
    __ASSERT_NO_MSG(color != NULL);
    return color->r == color->g && color->g == color->b;
}

static uint32_t calc_distance(uint8_t target, uint8_t index)
{
    uint32_t value = index > 215 ? index * 10 - 2152 : index * 40 + (index > 0 ? 55 : 0);
    return abs(target - value);
}

uint8_t rgb24_to_256(const struct rgb24 *color)
{
    __ASSERT_NO_MSG(color != NULL);

    uint32_t min_distance = 240;
    uint8_t best_index = 0;

    // Search through all 216 colors in the 6x6x6 cube (indices 0-215)
    for (uint16_t i = 0; i < 216; i++)
    {
        uint32_t r_dist = calc_distance(color->r, i / 36);
        uint32_t g_dist = calc_distance(color->g, (i / 6) % 6);
        uint32_t b_dist = calc_distance(color->b, i % 6);

        uint32_t total_dist = r_dist + g_dist + b_dist;
        if (total_dist < min_distance)
        {
            min_distance = total_dist;
            best_index = i;
        }
    }

    return best_index + 16;
}