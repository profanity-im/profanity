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
#include <unistd.h>

#include <ncurses.h>

#include "profanity.h"
#include "log.h"
#include "windows.h"
#include "jabber.h"
#include "command.h"
#include "history.h"

static void _profanity_init(int disable_tls);
static void _profanity_event_loop(int *ch, char *cmd, int *size);

void profanity_main(int disable_tls)
{
    int cmd_result = TRUE;

    _profanity_init(disable_tls);

    inp_non_block();
    while(cmd_result == TRUE) {
        int ch = ERR;
        char inp[100];
        int size = 0;

        while(ch != '\n')
            _profanity_event_loop(&ch, inp, &size);

        inp[size++] = '\0';
        cmd_result = process_input(inp);
    }

    jabber_disconnect();
}

static void _profanity_init(int disable_tls)
{
    jabber_init(disable_tls);
    history_init();
}

static void _profanity_event_loop(int *ch, char *cmd, int *size)
{
    usleep(1);
    gui_refresh();
    jabber_process_events();
    win_handle_switch(ch);
    inp_poll_char(ch, cmd, size);
} 
