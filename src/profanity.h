/*
 * profanity.h
 * vim: expandtab:ts=4:sts=4:sw=4
 *
 * Copyright (C) 2012 - 2019 James Booth <boothj5@gmail.com>
 * Copyright (C) 2019 - 2026 Michael Vetter <jubalh@iodoru.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later WITH OpenSSL-exception
 */

#ifndef PROFANITY_H
#define PROFANITY_H

#include <pthread.h>
#include <glib.h>

void prof_run(gchar* log_level, gchar* account_name, gchar* config_file, gchar* log_file, gchar* theme_name, gchar** commands);
gboolean prof_set_quit(void);

extern pthread_mutex_t lock;

#endif
