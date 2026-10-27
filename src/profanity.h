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

void prof_run(const gchar* const log_level, const gchar* const account_name, const gchar* const config_file, const gchar* const log_file, const gchar* const theme_name, gchar** commands);
void prof_shutdown(void);
gboolean prof_set_quit(void);

extern pthread_mutex_t lock;

#endif
