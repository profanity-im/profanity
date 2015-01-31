/*
 * inputwin.c
 *
 * Copyright (C) 2012 - 2014 James Booth <boothj5@gmail.com>
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

#define _XOPEN_SOURCE_EXTENDED
#include "config.h"

#include <stdlib.h>
#include <string.h>
#include <wchar.h>

#include <readline/readline.h>
#include <readline/history.h>

#ifdef HAVE_NCURSESW_NCURSES_H
#include <ncursesw/ncurses.h>
#elif HAVE_NCURSES_H
#include <ncurses.h>
#endif

#include "command/command.h"
#include "common.h"
#include "config/accounts.h"
#include "config/preferences.h"
#include "config/theme.h"
#include "log.h"
#include "muc.h"
#include "profanity.h"
#include "roster_list.h"
#include "ui/ui.h"
#include "ui/statusbar.h"
#include "ui/inputwin.h"
#include "ui/windows.h"
#include "xmpp/xmpp.h"

static WINDOW *inp_win;

static struct timeval p_rl_timeout;
static fd_set fds;
static int r;
static gboolean cmd_result = TRUE;

static int pad_start = 0;

static void _inp_win_update_virtual(void);

static void
cb_linehandler(char *line)
{
    if (*line) {
        add_history(line);
    }
    rl_redisplay();
    cmd_result = cmd_process_input(line);
    free(line);
}

void
create_input_window(void)
{
#ifdef NCURSES_REENTRANT
    set_escdelay(25);
#else
    ESCDELAY = 25;
#endif
	p_rl_timeout.tv_sec = 0;
    p_rl_timeout.tv_usec = 500000;
    rl_callback_handler_install(NULL, cb_linehandler);

    inp_win = newpad(1, INP_WIN_MAX);
    wbkgd(inp_win, theme_attrs(THEME_INPUT_TEXT));;
    keypad(inp_win, TRUE);
    wmove(inp_win, 0, 0);
    _inp_win_update_virtual();
}

void
inp_win_resize(void)
{
    int col = getcurx(inp_win);
    int wcols = getmaxx(stdscr);

    // if lost cursor off screen, move contents to show it
    if (col >= pad_start + wcols) {
        pad_start = col - (wcols / 2);
        if (pad_start < 0) {
            pad_start = 0;
        }
    }

    wbkgd(inp_win, theme_attrs(THEME_INPUT_TEXT));;
    _inp_win_update_virtual();
}

static int
offset_to_col(char *str, int offset)
{
    int i = 0;
    int col = 0;
    mbstate_t internal;

    while (i != offset && str[i] != '\n') {
        gunichar uni = g_utf8_get_char(&str[i]);
        size_t ch_len = mbrlen(&str[i], 4, &internal);
        i += ch_len;
        col++;
        if (g_unichar_iswide(uni)) {
            col++;
        }
    }

    return col;
}

void
inp_non_block(gint block_timeout)
{
    wtimeout(inp_win, block_timeout);
}

void
inp_block(void)
{
    wtimeout(inp_win, -1);
}

void
inp_write(char *line, int offset)
{
    int col = offset_to_col(line, offset);
    werase(inp_win);
    waddstr(inp_win, line);
    wmove(inp_win, 0, col);
    _inp_win_update_virtual();
}

gboolean
inp_readline(void)
{
    FD_ZERO(&fds);
    FD_SET(fileno(rl_instream), &fds);
    r = select(FD_SETSIZE, &fds, NULL, NULL, &p_rl_timeout);
    if (r < 0) {
        log_error("Readline failed.");
        return false;
    }

    if (FD_ISSET(fileno(rl_instream), &fds)) {
        rl_callback_read_char();
        if (rl_line_buffer && rl_line_buffer[0] != '/') {
            prof_handle_activity();
        }
        inp_write(rl_line_buffer, rl_point);
    } else {
        prof_handle_idle();
    }

    p_rl_timeout.tv_sec = 0;
    p_rl_timeout.tv_usec = 500000;

    return cmd_result;
}

void
inp_close(void)
{
    rl_callback_handler_remove();
}

void
inp_get_password(char *passwd)
{
    werase(inp_win);
    wmove(inp_win, 0, 0);
    pad_start = 0;
    _inp_win_update_virtual();
    doupdate();
    noecho();
    mvwgetnstr(inp_win, 0, 1, passwd, MAX_PASSWORD_SIZE);
    wmove(inp_win, 0, 0);
    echo();
    status_bar_clear();
}

void
inp_put_back(void)
{
    _inp_win_update_virtual();
}

void
inp_win_clear(void)
{
    werase(inp_win);
    wmove(inp_win, 0, 0);
    pad_start = 0;
    _inp_win_update_virtual();
}

static void
_inp_win_update_virtual(void)
{
    int wrows, wcols;
    getmaxyx(stdscr, wrows, wcols);
    pnoutrefresh(inp_win, 0, pad_start, wrows-1, 0, wrows-1, wcols-1);
}