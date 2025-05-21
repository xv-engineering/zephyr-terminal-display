/*
 * Copyright (c) 2025 Noah Luskey <noah@xv.engineering>
 *
 * SPDX-License-Identifier: MIT
 */
#include <zephyr/kernel.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/drivers/display.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/logging/log.h>
#include <zephyr/pm/device_runtime.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include "rgb24.h"

#define DT_DRV_COMPAT xv_terminal_display

LOG_MODULE_REGISTER(terminal_display, CONFIG_TERMINAL_DISPLAY_LOG_LEVEL);

struct terminal_display_config
{
    LOG_INSTANCE_PTR_DECLARE(log);
    const struct device *terminal;
    const struct display_capabilities capabilities;
};

struct terminal_display_data
{
    struct k_sem thread_sem;
    struct rgb24 *buffer;
    atomic_t *dirty_pixels;
    struct
    {
        bool on;
        bool previously_on;
    } blanking;
};

static int terminal_display_char_out(const struct device *dev, uint8_t *data, size_t length);
static struct rgb24 *terminal_display_get_buffer_pixel(const struct device *dev, const uint16_t x, const uint16_t y);

static int terminal_display_char_out(const struct device *dev, uint8_t *data, size_t length)
{
    __ASSERT_NO_MSG(dev != NULL);
    __ASSERT_NO_MSG(data != NULL);

    const struct terminal_display_config *config = dev->config;
    const struct device *terminal = config->terminal;

    for (size_t i = 0; i < length; i++)
    {
        uart_poll_out(terminal, data[i]);
    }

    return length;
}

static int
terminal_display_blanking_on(const struct device *dev)
{
    __ASSERT_NO_MSG(dev != NULL);
    struct terminal_display_data *data = dev->data;
    data->blanking.on = true;
    k_sem_give(&data->thread_sem);
    return 0;
}

static int terminal_display_blanking_off(const struct device *dev)
{
    __ASSERT_NO_MSG(dev != NULL);
    struct terminal_display_data *data = dev->data;
    data->blanking.on = false;
    k_sem_give(&data->thread_sem);
    return 0;
}

static int terminal_display_write(const struct device *dev, const uint16_t x,
                                  const uint16_t y,
                                  const struct display_buffer_descriptor *desc,
                                  const void *buf)
{
    __ASSERT_NO_MSG(dev != NULL);
    __ASSERT_NO_MSG(desc != NULL);
    __ASSERT_NO_MSG(buf != NULL);

    // Not strictly a requirement, but simplifies the logic below.
    // Since this is mostly a demo, leaving this for now.
    // TODO: actually deal with this or confirm it's not an issue
    // __ASSERT_NO_MSG(desc->pitch == 0);

    const struct terminal_display_config *config = dev->config;
    struct terminal_display_data *data = dev->data;

    // using the descriptor, copy the buffer to the appropriate section of the display
    BUILD_ASSERT(sizeof(struct rgb24) == 3);
    BUILD_ASSERT(__alignof__(struct rgb24) == 1);

    const size_t expected_size = desc->width * desc->height * sizeof(struct rgb24);
    if (desc->buf_size < expected_size)
    {
        LOG_ERR("Buffer size is too small: %d < %d", desc->buf_size, expected_size);
        return -EINVAL;
    }

    const struct rgb24 *source = (const struct rgb24 *)buf;
    for (uint16_t sy = 0; sy < desc->height; sy++)
    {
        for (uint16_t sx = 0; sx < desc->width; sx++)
        {
            const struct rgb24 *color = &source[sy * desc->width + sx];
            const uint16_t dx = x + sx;
            const uint16_t dy = y + sy;

            if (dx >= config->capabilities.x_resolution || dy >= config->capabilities.y_resolution)
            {
                LOG_INST_WRN(config->log, "Out of bounds pixel coordinates: x=%d, y=%d", dx, dy);
                continue;
            }

            struct rgb24 *destination = terminal_display_get_buffer_pixel(dev, dx, dy);
            if (!rgb24_equal(destination, color))
            {
                *destination = *color;
                atomic_set_bit(data->dirty_pixels, dy * config->capabilities.x_resolution + dx);
            }
        }
    }

    if (!desc->frame_incomplete)
    {
        LOG_INST_DBG(config->log, "Complete frame");
        k_sem_give(&data->thread_sem);
    }
    else
    {
        LOG_INST_DBG(config->log, "Partial frame");
    }

    return 0;
}

static int terminal_display_read(const struct device *dev, const uint16_t x,
                                 const uint16_t y,
                                 const struct display_buffer_descriptor *desc,
                                 void *buf)
{
    return -ENOTSUP;
}

static void *terminal_display_get_framebuffer(const struct device *dev)
{
    return NULL;
}

static int terminal_display_set_brightness(const struct device *dev, const uint8_t brightness)
{
    return -ENOTSUP;
}

static int terminal_display_set_contrast(const struct device *dev, const uint8_t contrast)
{
    return -ENOTSUP;
}

static struct rgb24 *terminal_display_get_buffer_pixel(const struct device *dev, const uint16_t x, const uint16_t y)
{
    __ASSERT_NO_MSG(dev != NULL);

    const struct terminal_display_config *config = dev->config;
    const struct terminal_display_data *data = dev->data;

    __ASSERT_NO_MSG(x < config->capabilities.x_resolution);
    __ASSERT_NO_MSG(y < config->capabilities.y_resolution);

    return &data->buffer[y * config->capabilities.x_resolution + x];
}

/* actually write out a value to the "physical" display */
static void terminal_display_write_pixel(const struct device *dev, const uint16_t x, const uint16_t y, const struct rgb24 *color)
{
    __ASSERT_NO_MSG(dev != NULL);
    __ASSERT_NO_MSG(color != NULL);

    const struct terminal_display_config *config = dev->config;
    if (x >= config->capabilities.x_resolution || y >= config->capabilities.y_resolution)
    {
        LOG_ERR("Invalid pixel coordinates: x=%d, y=%d", x, y);
        return;
    }

    // first, convert the rgb24 color to a 256 color index
    const uint8_t color_index = rgb24_to_256(color);
    // navigate the cursor to that position within the terminal
    // using the "cursor position" escape sequence
    // Format and send the cursor position escape sequence
    char cursor_pos[32];
    // Note: Terminal coordinates are 1-based
    // Note: each pixel is two characters wide because it looks better
    snprintf(cursor_pos, sizeof(cursor_pos), "\x1b[%d;%dH", y + 1, (x * 2) + 1);
    terminal_display_char_out(dev, (uint8_t *)cursor_pos, strlen(cursor_pos));

    // Set the background color using the 256-color index
    // Note: each pixel is two characters wide because it looks better
    char color_cmd[32];
    snprintf(color_cmd, sizeof(color_cmd), "\x1b[48;5;%dm  ", color_index);
    terminal_display_char_out(dev, (uint8_t *)color_cmd, strlen(color_cmd));

    // Reset colors
    const char *reset = "\x1b[0m";
    terminal_display_char_out(dev, (uint8_t *)reset, strlen(reset));
}

static void terminal_display_get_capabilities(const struct device *dev,
                                              struct display_capabilities *capabilities)
{
    const struct terminal_display_config *config = dev->config;
    *capabilities = config->capabilities;
}

static int terminal_display_write_pixel_format(const struct device *dev,
                                               const enum display_pixel_format pixel_format)
{
    __ASSERT_NO_MSG(dev != NULL);
    if (pixel_format != PIXEL_FORMAT_RGB_888)
    {
        LOG_ERR("Unsupported pixel format: %d", pixel_format);
        return -ENOTSUP;
    }
    return 0;
}

static int terminal_display_set_orientation(const struct device *dev,
                                            const enum display_orientation orientation)
{
    __ASSERT_NO_MSG(dev != NULL);
    if (orientation != DISPLAY_ORIENTATION_NORMAL)
    {
        LOG_ERR("Unsupported orientation: %d", orientation);
        return -ENOTSUP;
    }
    return 0;
}

static DEVICE_API(display, api) = {
    .blanking_on = terminal_display_blanking_on,
    .blanking_off = terminal_display_blanking_off,
    .write = terminal_display_write,
    .read = terminal_display_read,
    .get_framebuffer = terminal_display_get_framebuffer,
    .set_brightness = terminal_display_set_brightness,
    .set_contrast = terminal_display_set_contrast,
    .get_capabilities = terminal_display_get_capabilities,
    .set_pixel_format = terminal_display_write_pixel_format,
    .set_orientation = terminal_display_set_orientation,
};

static int terminal_display_init(const struct device *dev)
{
    const struct terminal_display_config *config = dev->config;
    struct terminal_display_data *data = dev->data;

    if (!device_is_ready(config->terminal))
    {
        LOG_INST_ERR(config->log, "Terminal device is not ready");
        return -ENODEV;
    }

    // giving the sem will let the thread
    // build up the blank screen to start
    k_sem_give(&data->thread_sem);

    return 0;
}

static void terminal_display_thread_entry(void *d, void *p2, void *p3)
{
    __ASSERT_NO_MSG(d != NULL);
    ARG_UNUSED(p2);
    ARG_UNUSED(p3);

    const struct device *dev = d;
    const struct terminal_display_config *const config = dev->config;
    struct terminal_display_data *const data = dev->data;

    while (true)
    {
        LOG_INST_DBG(config->log, "Waiting for semaphore");
        k_sem_take(&data->thread_sem, K_FOREVER);
        LOG_INST_DBG(config->log, "Semaphore taken");

        // If blanking is on, and it wasn't previously on,
        // clear the whole display by setting the color to black.
        if (data->blanking.on && !data->blanking.previously_on)
        {
            LOG_INST_INF(config->log, "Blanking terminal_display - blanking on");
            const struct rgb24 black = {0, 0, 0};
            // clear the whole display
            for (uint16_t y = 0; y < config->capabilities.y_resolution; y++)
            {
                for (uint16_t x = 0; x < config->capabilities.x_resolution; x++)
                {
                    terminal_display_write_pixel(dev, x, y, &black);
                }
            }
        }
        else if (!data->blanking.on && data->blanking.previously_on)
        {
            LOG_INST_INF(config->log, "Restoring terminal_display - blanking off");
            for (uint16_t y = 0; y < config->capabilities.y_resolution; y++)
            {
                for (uint16_t x = 0; x < config->capabilities.x_resolution; x++)
                {
                    const struct rgb24 *color = terminal_display_get_buffer_pixel(dev, x, y);
                    terminal_display_write_pixel(dev, x, y, color);
                    // clear all of the dirty bits
                    atomic_clear_bit(data->dirty_pixels, y * config->capabilities.x_resolution + x);
                }
            }
        }
        else
        {
            // normal write - go through the whole display and write every
            // dirty bit out do the display
            for (uint16_t y = 0; y < config->capabilities.y_resolution; y++)
            {
                for (uint16_t x = 0; x < config->capabilities.x_resolution; x++)
                {
                    if (atomic_test_and_clear_bit(data->dirty_pixels, y * config->capabilities.x_resolution + x))
                    {
                        LOG_INST_DBG(config->log, "Writing pixel at %d, %d", x, y);
                        const struct rgb24 *color = terminal_display_get_buffer_pixel(dev, x, y);
                        terminal_display_write_pixel(dev, x, y, color);
                    }
                }
            }
        }

        data->blanking.previously_on = data->blanking.on;
    }
}

#define TERMINAL_DISPLAY_BUFFER_SIZE(inst) (DT_INST_PROP(inst, width) * DT_INST_PROP(inst, height))

#define TERMINAL_DISPLAY_DEFINE(inst)                                                                            \
    LOG_INSTANCE_REGISTER(terminal_display, inst, CONFIG_TERMINAL_DISPLAY_LOG_LEVEL);                            \
    K_KERNEL_THREAD_DEFINE(terminal_display_thread##inst, 2048, terminal_display_thread_entry,                   \
                           DEVICE_DT_INST_GET(inst), NULL, NULL, CONFIG_TERMINAL_DISPLAY_THREAD_PRIORITY, 0, 0); \
    static struct rgb24 buffer##inst[TERMINAL_DISPLAY_BUFFER_SIZE(inst)] = {0};                                  \
    static ATOMIC_DEFINE(dirty_pixels##inst, TERMINAL_DISPLAY_BUFFER_SIZE(inst));                                \
    static const struct terminal_display_config config##inst = {                                                 \
        .terminal = DEVICE_DT_GET(DT_INST_PROP(inst, terminal)),                                                 \
        .capabilities = {                                                                                        \
            .x_resolution = DT_INST_PROP(inst, width),                                                           \
            .y_resolution = DT_INST_PROP(inst, height),                                                          \
            .supported_pixel_formats = PIXEL_FORMAT_RGB_888,                                                     \
            .current_pixel_format = PIXEL_FORMAT_RGB_888,                                                        \
            .current_orientation = DISPLAY_ORIENTATION_NORMAL,                                                   \
        },                                                                                                       \
        LOG_INSTANCE_PTR_INIT(log, terminal_display, inst)};                                                     \
    static struct terminal_display_data data##inst = {                                                           \
        .thread_sem = Z_SEM_INITIALIZER(data##inst.thread_sem, 0, 1),                                            \
        .buffer = buffer##inst,                                                                                  \
        .dirty_pixels = dirty_pixels##inst,                                                                      \
        .blanking = {                                                                                            \
            .on = true,                                                                                          \
            .previously_on = false,                                                                              \
        }};                                                                                                      \
    DEVICE_DT_INST_DEFINE(inst, terminal_display_init, NULL, &data##inst, &config##inst,                         \
                          POST_KERNEL, CONFIG_TERMINAL_DISPLAY_INIT_PRIORITY, &api);

DT_INST_FOREACH_STATUS_OKAY(TERMINAL_DISPLAY_DEFINE);
