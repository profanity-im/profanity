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
#include <glib.h>

#include "profanity.h"
#include "log.h"
#include "ui.h"
#include "jabber.h"
#include "command.h"
#include "preferences.h"
#include "contact_list.h"

static void _profanity_shutdown(void);

void profanity_run(void)
{
    gboolean cmd_result = TRUE;

    inp_non_block();
    while(cmd_result == TRUE) {
        int ch = ERR;
        char inp[200];
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

void profanity_init(const int disable_tls)
{
    prefs_load();
    log_init();
    gui_init();
    jabber_init(disable_tls);
    command_init();
    contact_list_init();
    atexit(_profanity_shutdown);
}

void _profanity_shutdown(void)
{
    jabber_disconnect();
    gui_close();
    log_close();
}
