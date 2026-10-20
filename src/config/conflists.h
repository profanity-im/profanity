/*
 * conflists.h
 * vim: expandtab:ts=4:sts=4:sw=4
 *
 * Copyright (C) 2012 - 2019 James Booth <boothj5@gmail.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later WITH OpenSSL-exception
 */

#ifndef CONFIG_CONFLISTS_H
#define CONFIG_CONFLISTS_H

#include <glib.h>

gboolean conf_string_list_add(GKeyFile* keyfile, const gchar* const group, const gchar* const key,
                              const gchar* const item);
gboolean conf_string_list_remove(GKeyFile* keyfile, const gchar* const group, const gchar* const key,
                                 const gchar* const item);

#endif
