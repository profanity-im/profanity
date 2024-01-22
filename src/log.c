/*
 * log.c
 * vim: expandtab:ts=4:sts=4:sw=4
 *
 * Copyright (C) 2012 - 2019 James Booth <boothj5@gmail.com>
 * Copyright (C) 2018 - 2024 Michael Vetter <jubalh@iodoru.org>
 *
 * This file is part of Profanity.
 *
 * Profanity is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Profanity is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Profanity.  If not, see <https://www.gnu.org/licenses/>.
 *
 * In addition, as a special exception, the copyright holders give permission to
 * link the code of portions of this program with the OpenSSL library under
 * certain conditions as described in each individual source file, and
 * distribute linked combinations including the two.
 *
 * You must obey the GNU General Public License in all respects for all of the
 * code used other than OpenSSL. If you modify file(s) with this exception, you
 * may extend this exception to your version of the file(s), but you are not
 * obligated to do so. If you do not wish to do so, delete this exception
 * statement from your version. If you delete this exception statement from all
 * source files in the program, then also delete it here.
 *
 */

#include "config.h"

#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "glib.h"
#include "glib/gstdio.h"

#include "log.h"
#include "common.h"
#include "config/files.h"
#include "config/preferences.h"

#define PROF "prof"

static FILE* logp;
static gchar* mainlogfile = NULL;
static gboolean user_provided_log = FALSE;
static log_level_t level_filter;

static int stderr_inited;
static log_level_t stderr_level;
static int stderr_pipe[2];
static char* stderr_buf;
static GString* stderr_msg;

enum {
    STDERR_BUFSIZE = 4000,
    STDERR_RETRY_NR = 5,
};

static void
_rotate_log_file(void)
{
    auto_gchar gchar* log_file = g_strdup(mainlogfile);
    size_t len = strlen(log_file);
    auto_gchar gchar* log_file_new = malloc(len + 5);

    // the mainlog file should always end in '.log', lets remove this last part
    // so that we can have profanity.001.log later
    if (len > 4) {
        log_file[len - 4] = '\0';
    }

    // find an empty name. from .log -> log.001 -> log.999
    for (int i = 1; i < 1000; i++) {
        g_sprintf(log_file_new, "%s.%03d.log", log_file, i);
        if (!g_file_test(log_file_new, G_FILE_TEST_EXISTS))
            break;
    }

    log_close();

    if (len > 4) {
        log_file[len - 4] = '.';
    }

    rename(log_file, log_file_new);

    log_init(log_get_filter(), NULL);

    log_info("Log has been rotated");
}

// abbreviation string is the prefix that's used in the log file
static char*
_log_abbreviation_string_from_level(log_level_t level)
{
    switch (level) {
    case PROF_LEVEL_ERROR:
        return "ERR";
    case PROF_LEVEL_WARN:
        return "WRN";
    case PROF_LEVEL_INFO:
        return "INF";
    case PROF_LEVEL_DEBUG:
        return "DBG";
    default:
        return "LOG";
    }
}

void
log_debug(const char* const msg, ...)
{
    va_list arg;
    va_start(arg, msg);
    GString* fmt_msg = g_string_new(NULL);
    g_string_vprintf(fmt_msg, msg, arg);
    log_msg(PROF_LEVEL_DEBUG, PROF, fmt_msg->str);
    g_string_free(fmt_msg, TRUE);
    va_end(arg);
}

void
log_info(const char* const msg, ...)
{
    va_list arg;
    va_start(arg, msg);
    GString* fmt_msg = g_string_new(NULL);
    g_string_vprintf(fmt_msg, msg, arg);
    log_msg(PROF_LEVEL_INFO, PROF, fmt_msg->str);
    g_string_free(fmt_msg, TRUE);
    va_end(arg);
}

void
log_warning(const char* const msg, ...)
{
    va_list arg;
    va_start(arg, msg);
    GString* fmt_msg = g_string_new(NULL);
    g_string_vprintf(fmt_msg, msg, arg);
    log_msg(PROF_LEVEL_WARN, PROF, fmt_msg->str);
    g_string_free(fmt_msg, TRUE);
    va_end(arg);
}

void
log_error(const char* const msg, ...)
{
    va_list arg;
    va_start(arg, msg);
    GString* fmt_msg = g_string_new(NULL);
    g_string_vprintf(fmt_msg, msg, arg);
    log_msg(PROF_LEVEL_ERROR, PROF, fmt_msg->str);
    g_string_free(fmt_msg, TRUE);
    va_end(arg);
}

void
log_init(log_level_t filter, char* log_file)
{
    level_filter = filter;

    if (log_file) {
        user_provided_log = TRUE;
    }

    mainlogfile = files_get_log_file(log_file);

    logp = fopen(mainlogfile, "a");
    g_chmod(mainlogfile, S_IRUSR | S_IWUSR);
}

const gchar*
get_log_file_location(void)
{
    return mainlogfile;
}

log_level_t
log_get_filter(void)
{
    return level_filter;
}

void
log_close(void)
{
    g_free(mainlogfile);
    mainlogfile = NULL;
    if (logp) {
        fclose(logp);
    }
}

void
log_msg(log_level_t level, const char* const area, const char* const msg)
{
    if (level >= level_filter && logp) {
        GDateTime* dt = g_date_time_new_now_local();

        char* level_str = _log_abbreviation_string_from_level(level);

        auto_gchar gchar* date_fmt = g_date_time_format_iso8601(dt);

        fprintf(logp, "%s: %s: %s: %s\n", date_fmt, area, level_str, msg);
        g_date_time_unref(dt);

        fflush(logp);

        if (prefs_get_boolean(PREF_LOG_ROTATE) && !user_provided_log) {
            long result = ftell(logp);
            if (result != -1 && result >= prefs_get_max_log_size()) {
                _rotate_log_file();
            }
        }
    }
}

int
log_level_from_string(char* log_level, log_level_t* level)
{
    int ret = 0;
    assert(log_level != NULL);
    assert(level != NULL);
    if (strcmp(log_level, "DEBUG") == 0) {
        *level = PROF_LEVEL_DEBUG;
    } else if (strcmp(log_level, "INFO") == 0) {
        *level = PROF_LEVEL_INFO;
    } else if (strcmp(log_level, "WARN") == 0) {
        *level = PROF_LEVEL_WARN;
    } else if (strcmp(log_level, "ERROR") == 0) {
        *level = PROF_LEVEL_ERROR;
    } else { // default logging is warn
        *level = PROF_LEVEL_WARN;
        ret = -1;
    }
    return ret;
}

const char*
log_string_from_level(log_level_t level)
{
    switch (level) {
    case PROF_LEVEL_ERROR:
        return "ERROR";
    case PROF_LEVEL_WARN:
        return "WARN";
    case PROF_LEVEL_INFO:
        return "INFO";
    case PROF_LEVEL_DEBUG:
        return "DEBUG";
    default:
        return "LOG";
    }
}

void
log_stderr_handler(void)
{
    GString* const s = stderr_msg;
    char* const buf = stderr_buf;
    ssize_t size;
    int retry = 0;

    if (!stderr_inited)
        return;

    do {
        size = read(stderr_pipe[0], buf, STDERR_BUFSIZE);
        if (size == -1 && errno == EINTR && retry++ < STDERR_RETRY_NR)
            continue;
        if (size <= 0 || retry++ >= STDERR_RETRY_NR)
            break;

        for (int i = 0; i < size; ++i) {
            if (buf[i] == '\n') {
                log_msg(stderr_level, "stderr", s->str);
                g_string_assign(s, "");
            } else
                g_string_append_c(s, buf[i]);
        }
    } while (1);

    if (s->len > 0 && s->str[0] != '\0') {
        log_msg(stderr_level, "stderr", s->str);
        g_string_assign(s, "");
    }
}

static int
log_stderr_nonblock_set(int fd)
{
    int rc;

    rc = fcntl(fd, F_GETFL);
    if (rc >= 0)
        rc = fcntl(fd, F_SETFL, rc | O_NONBLOCK);

    return rc;
}

void
log_stderr_init(log_level_t level)
{
    int rc;

    rc = pipe(stderr_pipe);
    if (rc != 0)
        goto err;

    close(STDERR_FILENO);
    rc = dup2(stderr_pipe[1], STDERR_FILENO);
    if (rc < 0)
        goto err_close;

    rc = log_stderr_nonblock_set(stderr_pipe[0])
             ?: log_stderr_nonblock_set(stderr_pipe[1]);
    if (rc != 0)
        goto err_close;

    stderr_buf = malloc(STDERR_BUFSIZE);
    stderr_msg = g_string_sized_new(STDERR_BUFSIZE);
    stderr_level = level;
    stderr_inited = 1;

    if (stderr_buf == NULL || stderr_msg == NULL) {
        errno = ENOMEM;
        goto err_free;
    }
    return;

err_free:
    if (stderr_msg != NULL)
        g_string_free(stderr_msg, TRUE);
    free(stderr_buf);
err_close:
    close(stderr_pipe[0]);
    close(stderr_pipe[1]);
err:
    stderr_inited = 0;
    log_error("Unable to init stderr log handler: %s", strerror(errno));
}

void
log_stderr_close(void)
{
    if (!stderr_inited)
        return;

    /* handle remaining logs before close */
    log_stderr_handler();
    stderr_inited = 0;
    free(stderr_buf);
    g_string_free(stderr_msg, TRUE);
    close(stderr_pipe[0]);
    close(stderr_pipe[1]);
}
