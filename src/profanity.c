/* 
 * profanity.c
 *
 * Copyright (C) 2012 James Booth <boothj5@gmail.com>
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
 * along with Profanity.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include <glib.h>

#include "config.h"
#include "profanity.h"
#include "log.h"
#include "chat_log.h"
#include "ui.h"
#include "jabber.h"
#include "command.h"
#include "preferences.h"
#include "contact_list.h"
#include "tinyurl.h"

static log_level_t _get_log_level(char *log_level);
static void _profanity_shutdown(void);

void
profanity_run(void)
{
    gboolean cmd_result = TRUE;

    log_msg(PROF_LEVEL_INFO, "prof", "Starting main event loop");

    inp_non_block();
    while(cmd_result == TRUE) {
        int ch = ERR;
        char inp[INP_WIN_MAX];
        int size = 0;

        while(ch != '\n') {
            win_handle_special_keys(&ch);

            if (ch == KEY_RESIZE) {
                gui_resize(ch, inp, size);
            }
            
            gui_refresh();
            jabber_process_events();
            
            inp_get_char(&ch, inp, &size);
        }

        inp[size++] = '\0';
        cmd_result = process_input(inp);
    }

}

void
profanity_init(const int disable_tls, char *log_level)
{
    create_config_directory();
    log_level_t prof_log_level = _get_log_level(log_level);
    log_init(prof_log_level);
    log_msg(PROF_LEVEL_INFO, PROF, "Starting Profanity (%s)...", PACKAGE_VERSION);
    chat_log_init();
    prefs_load();
    gui_init();
    jabber_init(disable_tls);
    cmd_init();
    contact_list_init();
    atexit(_profanity_shutdown);
}

void
_profanity_shutdown(void)
{
    log_msg(PROF_LEVEL_INFO, PROF, "Profanity is shutting down.");
    jabber_disconnect();
    gui_close();
    chat_log_close();
    prefs_close();
    log_msg(PROF_LEVEL_INFO, "prof", "Shutdown complete");
    log_close();
}

static log_level_t 
_get_log_level(char *log_level)
{
    if (strcmp(log_level, "DEBUG") == 0) {
        return PROF_LEVEL_DEBUG;
    } else if (strcmp(log_level, "INFO") == 0) {
        return PROF_LEVEL_INFO;
    } else if (strcmp(log_level, "WARN") == 0) {
        return PROF_LEVEL_WARN;
    } else {
        return PROF_LEVEL_ERROR;
    }
}
