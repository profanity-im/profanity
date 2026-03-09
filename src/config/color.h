/*
 * color.h
 * vim: expandtab:ts=4:sts=4:sw=4
 *
 * Copyright (C) 2019 Aurelien Aptel <aurelien.aptel@gmail.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later WITH OpenSSL-exception
 */

#ifndef _COLOR_H_
#define _COLOR_H_

/* to access color names */
#define COLOR_NAME_SIZE 256

#include <stdint.h>

typedef enum {
    COLOR_PROFILE_DEFAULT,
    COLOR_PROFILE_REDGREEN_BLINDNESS,
    COLOR_PROFILE_BLUE_BLINDNESS,
} color_profile;

struct color_def
{
    uint16_t h;
    uint8_t s, l;
    const char* name;
};
extern const struct color_def color_names[];

/* hash string to color pair */
int color_pair_cache_hash_str(const char* str, color_profile profile);
/* parse fg_bg string to color pair */
int color_pair_cache_get(const char* pair_name);
/* clear cache */
void color_pair_cache_reset(void);
/* free cache */
void color_pair_cache_free(void);

#endif
