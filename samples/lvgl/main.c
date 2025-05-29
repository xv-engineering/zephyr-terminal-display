/*
 * Copyright (c) 2025 Noah Luskey <noah@xv.engineering>
 *
 * SPDX-License-Identifier: MIT
 */
#include <zephyr/kernel.h>
#include <zephyr/drivers/display.h>
#include <zephyr/logging/log.h>
#include <zephyr/devicetree.h>
#include <math.h>
#include <lvgl.h>
#include <lvgl_zephyr.h>
#include <zephyr/random/random.h>
#include "particle.h"

LOG_MODULE_REGISTER(sample, CONFIG_SAMPLE_LOG_LEVEL);

struct user_data
{
    struct xv_particle **particles;
    const size_t max_particles;
    size_t num_particles;
};

static uint16_t get_random_u16_between(uint16_t min, uint16_t max)
{
    __ASSERT_NO_MSG(min <= max);
    uint32_t range = max - min + 1;
    uint32_t limit = (32768 / range) * range;
    uint32_t r;
    do
    {
        r = sys_rand16_get();
    } while (r >= limit);
    return min + (r % range);
}

static lv_point_t get_random_position(void)
{
    const lv_coord_t mid_width = lv_obj_get_width(lv_screen_active()) / 2;
    const lv_coord_t mid_height = lv_obj_get_height(lv_screen_active()) / 2;
    return (lv_point_t){
        .x = get_random_u16_between(mid_width - 1, mid_width + 1),
        .y = get_random_u16_between(mid_height - 10, mid_height - 4),
    };
}

static lv_point_t get_random_velocity(void)
{
    return (lv_point_t){
        .x = 20 - get_random_u16_between(0, 40),
        .y = -1 * get_random_u16_between(40, 60),
    };
}

static lv_point_t get_random_acceleration(void)
{
    return (lv_point_t){
        .x = 50 - get_random_u16_between(0, 100),
        .y = 100,
    };
}

static lv_color_t get_random_color(void)
{
    return lv_color_hsv_to_rgb(get_random_u16_between(0, 359), 100, 100);
}

static struct xv_particle *new_random_particle(const lv_point_t acceleration)
{
    return xv_particle_create(
        lv_screen_active(),
        get_random_position(),
        get_random_velocity(),
        acceleration,
        get_random_color());
}

// Animation callback to update all particles, creating new
// particle burst when all particles have fallen off screen
static void particle_update_cb(lv_timer_t *timer)
{
    struct user_data *data = lv_timer_get_user_data(timer);
    // Get screen dimensions
    const lv_coord_t screen_width = lv_obj_get_width(lv_screen_active());
    const lv_coord_t screen_height = lv_obj_get_height(lv_screen_active());

    for (size_t i = 0; i < data->max_particles; i++)
    {
        // particles off-screen are deleted and set to NULL
        if (data->particles[i] == NULL)
        {
            continue;
        }

        xv_particle_update(data->particles[i], CONFIG_PHYSICS_UPDATE_PERIOD_MS);

        lv_point_t pos = xv_particle_get_pos(data->particles[i]);
        if (pos.x < 0 || pos.x >= screen_width || pos.y >= screen_height)
        {
            // delete off-screen particle
            xv_particle_delete(data->particles[i]);
            data->particles[i] = NULL;
            data->num_particles--;
        }
    }

    // create new burst. All particles have consistent acceleration
    // but individual particles have slightly randomized position
    // and initial velocities.
    const lv_point_t acceleration = get_random_acceleration();
    if (data->num_particles == 0)
    {
        for (size_t i = 0; i < data->max_particles; i++)
        {
            data->particles[i] = new_random_particle(acceleration);
            data->num_particles++;
        }
    }
}

int main(void)
{
    const struct device *display = DEVICE_DT_GET(DT_CHOSEN(zephyr_display));
    static struct xv_particle *particles[CONFIG_NUM_PARTICLES];
    static struct user_data user_data = {
        .particles = particles,
        .max_particles = ARRAY_SIZE(particles),
    };

    int res = lvgl_init();
    if (res != 0)
    {
        LOG_ERR("Failed to initialize lvgl");
        return res;
    }

    lv_obj_t *screen = lv_screen_active();

    if (IS_ENABLED(CONFIG_BOARD_NATIVE_SIM))
    {
        // if on native sim, it's hard to connect fast enough, but
        // we want to catch the initial de-blanking and "Zephyr!"
        k_sleep(K_SECONDS(5));
    }

    lv_obj_t *zephyr_label = lv_label_create(screen);
    lv_label_set_text(zephyr_label, "Zephyr!");
    lv_obj_set_style_text_color(zephyr_label, lv_color_make(128, 0, 128), 0);
    lv_obj_align(zephyr_label, LV_ALIGN_CENTER, 0, 0);

    const lv_point_t acceleration = get_random_acceleration();

    for (size_t i = 0; i < ARRAY_SIZE(particles); i++)
    {
        particles[i] = new_random_particle(acceleration);
        user_data.num_particles++;
        __ASSERT_NO_MSG(particles[i] != NULL);
    }

    display_blanking_off(display);

    // Create animation timers
    lv_timer_create(particle_update_cb, CONFIG_PHYSICS_UPDATE_PERIOD_MS, &user_data);

    while (true)
    {
        uint32_t ms = lv_timer_handler();
        k_sleep(K_MSEC(ms));
    }
}
