/*
 * log.h
 * vim: expandtab:ts=4:sts=4:sw=4
 *
 * Copyright (C) 2012 - 2019 James Booth <boothj5@gmail.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later WITH OpenSSL-exception
 */

#ifndef LOG_H
#define LOG_H

#include <glib.h>

#define PROF_TMPVAR__(n, l) n##l
#define PROF_TMPVAR_(n, l)  PROF_TMPVAR__(n, l)
#define PROF_TMPVAR(n)      PROF_TMPVAR_(PROF_##n##_, __LINE__)

// log levels
typedef enum {
    PROF_LEVEL_DEBUG,
    PROF_LEVEL_INFO,
    PROF_LEVEL_WARN,
    PROF_LEVEL_ERROR
} log_level_t;

void log_init(log_level_t filter, char* log_file);
log_level_t log_get_filter(void);
void log_close(void);
const gchar* get_log_file_location(void);
void log_debug(const char* const msg, ...);
void log_info(const char* const msg, ...);
void log_warning(const char* const msg, ...);
void log_error(const char* const msg, ...);
void log_msg(log_level_t level, const char* const area, const char* const msg);
int log_level_from_string(char* log_level, log_level_t* level);
const char* log_string_from_level(log_level_t level);

#define _log_Xtype_once(type, ...)                 \
    do {                                           \
        static gboolean PROF_TMPVAR(once) = FALSE; \
        if (!PROF_TMPVAR(once)) {                  \
            log_##type(__VA_ARGS__);               \
            PROF_TMPVAR(once) = TRUE;              \
        }                                          \
    } while (0)

#define log_error_once(...) _log_Xtype_once(error, __VA_ARGS__)

void log_stderr_init(log_level_t level);
void log_stderr_handler(void);

#endif
