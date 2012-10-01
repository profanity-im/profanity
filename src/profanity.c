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

#include "config.h"

#include <stdlib.h>
#include <string.h>

#include <glib.h>

#include "chat_log.h"
#include "command.h"
#include "common.h"
#include "contact_list.h"
#include "history.h"
#include "log.h"
#include "preferences.h"
#include "profanity.h"
#include "ui.h"

static log_level_t _get_log_level(char *log_level);
gboolean _process_input(char *inp);
static void _create_config_directory();

void
profanity_run(void)
{
    gboolean cmd_result = TRUE;

    log_info("Starting main event loop");

    inp_non_block();
    
    GTimer *timer = g_timer_new();

    while(cmd_result == TRUE) {
        int ch = ERR;
        char inp[INP_WIN_MAX];
        int size = 0;

        while(ch != '\n') {

            gdouble elapsed = g_timer_elapsed(timer, NULL);
            
            gint remind_period = prefs_get_remind();

            // 0 means to not remind
            if (remind_period > 0 && elapsed >= remind_period) {
                win_remind();
                g_timer_start(timer);
            }

            win_handle_special_keys(&ch);

            if (ch == KEY_RESIZE) {
                gui_resize(ch, inp, size);
            }
            
            gui_refresh();
            jabber_process_events();
            
            inp_get_char(&ch, inp, &size);
        }

        inp[size++] = '\0';
        cmd_result = _process_input(inp);
    }
}

void
profanity_init(const int disable_tls, char *log_level)
{
    _create_config_directory();
    log_level_t prof_log_level = _get_log_level(log_level);
    log_init(prof_log_level);
    log_info("Starting Profanity (%s)...", PACKAGE_VERSION);
    chat_log_init();
    prefs_load();
    gui_init();
    jabber_init(disable_tls);
    cmd_init();
    log_info("Initialising contact list");
    contact_list_init();
    atexit(profanity_shutdown_init);
}

void
profanity_shutdown_init(void)
{
    gboolean wait_response = jabber_disconnect();

    if (wait_response) {
        while (jabber_connection_status() == JABBER_DISCONNECTING) {
            jabber_process_events();
        }
    }

    profanity_shutdown();
}

void
profanity_shutdown(void)
{
    gui_close();                                                          
    chat_log_close();                                                    
    prefs_close();                                                       
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

/* 
 * Take a line of input and process it, return TRUE if profanity is to 
 * continue, FALSE otherwise
 */
gboolean
_process_input(char *inp)
{
    log_debug("Input recieved: %s", inp);
    gboolean result = FALSE;
    g_strstrip(inp);
    
    // add line to history if something typed
    if (strlen(inp) > 0) {
        history_append(inp);
    }

    // just carry on if no input
    if (strlen(inp) == 0) {
        result = TRUE;

    // habdle command if input starts with a '/'
    } else if (inp[0] == '/') {
        char inp_cpy[strlen(inp) + 1];
        strcpy(inp_cpy, inp);
        char *command = strtok(inp_cpy, " ");
        result = cmd_execute(command, inp);

    // call a default handler if input didn't start with '/'
    } else {
        result = cmd_execute_default(inp);
    }

    inp_clear();
    reset_search_attempts();
    win_page_off();

    return result;
}

void
prof_handle_typing(char *from)
{
    win_show_typing(from);
    win_page_off();
}

void
prof_handle_incoming_message(char *from, char *message)
{
    win_show_incomming_msg(from, message);
    win_page_off();
}

void
prof_handle_login_success(const char *jid)
{
    const char *msg = " logged in successfully.";
    char line[strlen(jid) + 1 + strlen(msg) + 1];
    sprintf(line, "%s %s", jid, msg);
    title_bar_set_status(PRESENCE_ONLINE);
    cons_show(line);
    log_info(line);
    win_page_off();
    status_bar_print_message(jid);
    status_bar_refresh();
    prefs_add_login(jid);
}

void
prof_handle_lost_connection(void)
{
    cons_bad_show("Lost connection.");
    log_info("Lost connection");
    win_disconnected();
    win_page_off();
    log_info("disconnected");
}

void
prof_handle_failed_login(void)
{
    cons_bad_show("Login failed.");
    log_info("Login failed");
    win_page_off();
    log_info("disconnected");
}

static void
_create_config_directory()
{
    GString *dir = g_string_new(getenv("HOME"));
    g_string_append(dir, "/.profanity");
    create_dir(dir->str);
    g_string_free(dir, TRUE);
}

