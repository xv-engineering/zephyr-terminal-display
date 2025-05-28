/*
 * Copyright (c) 2025 Noah Luskey <noah@xv.engineering>
 *
 * SPDX-License-Identifier: MIT
 */
#include "particle.h"
#include <zephyr/logging/log.h>

LOG_MODULE_DECLARE(sample, CONFIG_SAMPLE_LOG_LEVEL);

struct xv_particle
{
    lv_obj_t *obj;
    lv_point_t acc_x_100;
    lv_point_t vel_x_100;
    lv_point_t pos_x_100;
};

struct xv_particle *xv_particle_create(
    lv_obj_t *parent,
    lv_point_t pos,
    lv_point_t vel,
    lv_point_t acc,
    lv_color_t color)
{
    __ASSERT_NO_MSG(parent != NULL);
    struct xv_particle *particle = lv_malloc(sizeof(struct xv_particle));
    if (!particle)
    {
        LOG_ERR("Failed to allocate particle");
        return NULL;
    }

    *particle = (struct xv_particle){
        .obj = lv_obj_create(parent),
        .acc_x_100 = {.x = acc.x * 100, .y = acc.y * 100},
        .vel_x_100 = {.x = vel.x * 100, .y = vel.y * 100},
        .pos_x_100 = {.x = pos.x * 100, .y = pos.y * 100},
    };

    if (!particle->obj)
    {
        LOG_ERR("Failed to create particle object");
        lv_free(particle);
        return NULL;
    }

    // Configure the object as a single pixel
    lv_obj_set_pos(particle->obj, pos.x, pos.y);
    lv_obj_set_size(particle->obj, 1, 1);
    lv_obj_set_style_bg_color(particle->obj, color, 0);
    lv_obj_set_style_border_width(particle->obj, 0, 0);
    lv_obj_set_style_radius(particle->obj, 0, 0);
    lv_obj_set_style_outline_width(particle->obj, 0, 0);
    lv_obj_set_style_shadow_width(particle->obj, 0, 0);
    lv_obj_set_style_pad_all(particle->obj, 0, 0);

    return particle;
}

void xv_particle_delete(struct xv_particle *particle)
{
    __ASSERT_NO_MSG(particle != NULL);
    __ASSERT_NO_MSG(particle->obj != NULL);
    lv_obj_del(particle->obj);
    lv_free(particle);
}

void xv_particle_update(struct xv_particle *particle, uint32_t dt_msec)
{
    __ASSERT_NO_MSG(particle != NULL);

    const int32_t dt_sec_x_100 = dt_msec / 10;
    // things will break very quickly the dt becomes 0
    __ASSERT_NO_MSG(dt_sec_x_100 > 0);

    particle->vel_x_100.x += (particle->acc_x_100.x * dt_sec_x_100) / 100;
    particle->vel_x_100.y += (particle->acc_x_100.y * dt_sec_x_100) / 100;

    particle->pos_x_100.x += (particle->vel_x_100.x * dt_sec_x_100) / 100;
    particle->pos_x_100.y += (particle->vel_x_100.y * dt_sec_x_100) / 100;

    lv_point_t pos = xv_particle_get_pos(particle);
    lv_obj_set_pos(particle->obj, pos.x, pos.y);
}

lv_point_t xv_particle_get_pos(const struct xv_particle *particle)
{
    return (lv_point_t){
        .x = particle->pos_x_100.x / 100,
        .y = particle->pos_x_100.y / 100,
    };
}
