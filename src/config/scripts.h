/*
 * scripts.h
 * vim: expandtab:ts=4:sts=4:sw=4
 *
 * Copyright (C) 2012 - 2019 James Booth <boothj5@gmail.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later WITH OpenSSL-exception
 */

#ifndef CONFIG_SCRIPTS_H
#define CONFIG_SCRIPTS_H

#include <glib.h>

void scripts_init(void);
GSList* scripts_list(void);
GSList* scripts_read(const char* const script);
gboolean scripts_exec(const char* const script);

#endif
