/*
 * autocompleters.h
 * vim: expandtab:ts=4:sts=4:sw=4
 *
 * Copyright (C) 2012 - 2019 James Booth <boothj5@gmail.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later WITH OpenSSL-exception
 */

#ifndef PLUGINS_AUTOCOMPLETERS_H
#define PLUGINS_AUTOCOMPLETERS_H

#include <glib.h>

void autocompleters_init(void);
void autocompleters_add(const char* const plugin_name, const char* key, char** items);
void autocompleters_remove(const char* const plugin_name, const char* key, char** items);
void autocompleters_clear(const char* const plugin_name, const char* key);
void autocompleters_filepath_add(const char* const plugin_name, const char* prefix);
char* autocompleters_complete(const char* const input, gboolean previous);
void autocompleters_reset(void);
void autocompleters_destroy(void);

#endif
