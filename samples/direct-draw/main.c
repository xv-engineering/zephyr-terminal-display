/*
 * Copyright (c) 2025 Noah Luskey <noah@xv.engineering>
 *
 * SPDX-License-Identifier: MIT
 */
#include <zephyr/ztest.h>
#include <zephyr/drivers/display.h>
#include <zephyr/logging/log.h>
#include <zephyr/devicetree.h>
#include <math.h>
#include "utils.h"

LOG_MODULE_REGISTER(test, CONFIG_TEST_LOG_LEVEL);

ZTEST(terminal_display, test_hue_circle)
{
    const struct device *dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_display));
    zassert_true(device_is_ready(dev), "Device is not ready");

    int ret;
    ret = display_blanking_off(dev);
    zassert_equal(ret, 0);

    struct display_capabilities caps = {};
    display_get_capabilities(dev, &caps);
    zassert_true(caps.supported_pixel_formats & PIXEL_FORMAT_RGB_888);
    zassert_true(caps.current_pixel_format == PIXEL_FORMAT_RGB_888);
    zassert_true(caps.current_orientation == DISPLAY_ORIENTATION_NORMAL);
    zassert_equal(caps.x_resolution, DT_PROP(DT_CHOSEN(zephyr_display), width));
    zassert_equal(caps.y_resolution, DT_PROP(DT_CHOSEN(zephyr_display), height));

    uint8_t buf[9 * 3] = {0};
    struct display_buffer_descriptor desc = {
        .buf_size = sizeof(buf),
        .width = 3,
        .height = 3,
        .pitch = 3,
        .frame_incomplete = false,
    };

    for (double theta = 0.0; theta <= 10 * 3.1415; theta += 0.1)
    {
        uint8_t r, g, b;
        double h = fmod(theta * 100, 360.0);
        double s = 1.0;
        double v = 1.0;
        hsv_to_rgb(h, s, v, &r, &g, &b);

        for (int i = 0; i < 9; i++)
        {
            buf[i * 3] = r;
            buf[i * 3 + 1] = g;
            buf[i * 3 + 2] = b;
        }

        double x = (sin(theta) + 1.0) * ((caps.x_resolution - 1) / 2.5);
        double y = (cos(theta) + 1.0) * ((caps.y_resolution - 1) / 2.5);
        LOG_INF("Writing at %d, %d", (uint16_t)x, (uint16_t)y);
        ret = display_write(dev, (uint16_t)x, (uint16_t)y, &desc, buf);
        zassert_equal(ret, 0);
        k_sleep(K_MSEC(10));
    }
}

ZTEST_SUITE(terminal_display, NULL, NULL, NULL, NULL, NULL);
