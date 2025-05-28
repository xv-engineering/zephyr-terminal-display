/*
 * Copyright (c) 2025 Noah Luskey <noah@xv.engineering>
 *
 * SPDX-License-Identifier: MIT
 */
#ifndef __PARTICLE_H__
#define __PARTICLE_H__

#include <lvgl.h>

// forward definition
struct xv_particle;

struct xv_particle *xv_particle_create(lv_obj_t *parent, lv_point_t pos, lv_point_t vel, lv_point_t acc, lv_color_t color);

void xv_particle_delete(struct xv_particle *particle);

void xv_particle_update(struct xv_particle *particle, uint32_t dt_msec);

lv_point_t xv_particle_get_pos(const struct xv_particle *particle);

#endif // __PARTICLE_H__
