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

#include <ncurses.h>

#include "profanity.h"
#include "log.h"
#include "windows.h"
#include "jabber.h"
#include "command.h"
#include "history.h"

static void _profanity_shutdown(void);

void profanity_run(void)
{
    int cmd_result = TRUE;

    inp_non_block();
    while(cmd_result == TRUE) {
        int ch = ERR;
        char inp[100];
        int size = 0;

        while(ch != '\n') {
            gui_refresh();
            jabber_process_events();
            win_handle_special_keys(&ch);
            inp_get_char(&ch, inp, &size);
        }

        inp[size++] = '\0';
        cmd_result = process_input(inp);
    }

}

void profanity_init(const int disable_tls)
{
    log_init();
    gui_init();
    jabber_init(disable_tls);
    history_init();
    atexit(_profanity_shutdown);
}

void _profanity_shutdown(void)
{
    jabber_disconnect();
    gui_close();
    log_close();
}
