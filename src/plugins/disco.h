/*
 * disco.h
 * vim: expandtab:ts=4:sts=4:sw=4
 *
 * Copyright (C) 2012 - 2019 James Booth <boothj5@gmail.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later WITH OpenSSL-exception
 */

#ifndef PLUGINS_DISCO_H
#define PLUGINS_DISCO_H

#include <glib.h>

void disco_add_feature(const char* plugin_name, char* feature);
void disco_remove_features(const char* plugin_name);
GList* disco_get_features(void);
void disco_close(void);

#endif
