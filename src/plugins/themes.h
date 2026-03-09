/*
 * themes.h
 * vim: expandtab:ts=4:sts=4:sw=4
 *
 * Copyright (C) 2012 - 2019 James Booth <boothj5@gmail.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later WITH OpenSSL-exception
 */

#ifndef PLUGINS_THEMES_H
#define PLUGINS_THEMES_H

void plugin_themes_init(void);
void plugin_themes_close(void);
theme_item_t plugin_themes_get(const char* const group, const char* const key, const char* const def);

#endif
