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
static gint inp_timeout = 0;
static gint no_input_count = 0;

static fd_set fds;
static int r;
static gboolean cmd_result = TRUE;

static int pad_start = 0;

static void _inp_win_update_virtual(void);

static int
_printable(const wint_t ch)
{
    char bytes[MB_CUR_MAX+1];
    size_t utf_len = wcrtomb(bytes, ch, NULL);
    bytes[utf_len] = '\0';
    gunichar unichar = g_utf8_get_char(bytes);

    return g_unichar_isprint(unichar) && (ch != KEY_MOUSE);
}

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

int
prof_rl_getc(FILE *filein)
{
    int ch = rl_getc(filein);
    if (_printable(ch)) {
        cmd_reset_autocomplete();
    }
    return ch;
}

void
resize_signal_handler(int signal)
{
    ui_resize();
}

int
tab_handler(int count, int key)
{
    if (rl_point != rl_end || !rl_line_buffer) {
        return 0;
    }

    if ((strncmp(rl_line_buffer, "/", 1) != 0) && (ui_current_win_type() == WIN_MUC)) {
        char *result = muc_autocomplete(rl_line_buffer);
        if (result) {
            rl_replace_line(result, 0);
            rl_point = rl_end;
            inp_write(result, rl_point);
        }
    } else if (strncmp(rl_line_buffer, "/", 1) == 0) {
        char *result = cmd_autocomplete(rl_line_buffer);
        if (result) {
            rl_replace_line(result, 0);
            rl_point = rl_end;
            inp_write(result, rl_point);
        }
    }

    return 0;
}

int
win1_handler(int count, int key)
{
    ui_switch_win(1);
    return 0;
}

int
win2_handler(int count, int key)
{
    ui_switch_win(2);
    return 0;
}

int
win3_handler(int count, int key)
{
    ui_switch_win(3);
    return 0;
}

int
win4_handler(int count, int key)
{
    ui_switch_win(4);
    return 0;
}

int
win5_handler(int count, int key)
{
    ui_switch_win(5);
    return 0;
}

int
win6_handler(int count, int key)
{
    ui_switch_win(6);
    return 0;
}

int
win7_handler(int count, int key)
{
    ui_switch_win(7);
    return 0;
}

int
win8_handler(int count, int key)
{
    ui_switch_win(8);
    return 0;
}

int
win9_handler(int count, int key)
{
    ui_switch_win(9);
    return 0;
}

int
win0_handler(int count, int key)
{
    ui_switch_win(0);
    return 0;
}

int
altleft_handler(int count, int key)
{
    ui_previous_win();
    return 0;
}

int
altright_handler(int count, int key)
{
    ui_next_win();
    return 0;
}

int
pageup_handler(int count, int key)
{
    ui_page_up();
    return 0;
}

int
pagedown_handler(int count, int key)
{
    ui_page_down();
    return 0;
}

int
altpageup_handler(int count, int key)
{
    ui_subwin_page_up();
    return 0;
}

int
altpagedown_handler(int count, int key)
{
    ui_subwin_page_down();
    return 0;
}

int
startup_hook(void)
{
    rl_bind_keyseq("\\e1", win1_handler);
    rl_bind_keyseq("\\e2", win2_handler);
    rl_bind_keyseq("\\e3", win3_handler);
    rl_bind_keyseq("\\e4", win4_handler);
    rl_bind_keyseq("\\e5", win5_handler);
    rl_bind_keyseq("\\e6", win6_handler);
    rl_bind_keyseq("\\e7", win7_handler);
    rl_bind_keyseq("\\e8", win8_handler);
    rl_bind_keyseq("\\e9", win9_handler);
    rl_bind_keyseq("\\e0", win0_handler);

    rl_bind_keyseq("\\eOP", win1_handler);
    rl_bind_keyseq("\\eOQ", win2_handler);
    rl_bind_keyseq("\\eOR", win3_handler);
    rl_bind_keyseq("\\eOS", win4_handler);
    rl_bind_keyseq("\\e[15~", win5_handler);
    rl_bind_keyseq("\\e[17~", win6_handler);
    rl_bind_keyseq("\\e[18~", win7_handler);
    rl_bind_keyseq("\\e[19~", win8_handler);
    rl_bind_keyseq("\\e[20~", win9_handler);
    rl_bind_keyseq("\\e[21~", win0_handler);

    rl_bind_keyseq("\\e[1;3D", altleft_handler);
    rl_bind_keyseq("\\e[1;3C", altright_handler);

    rl_bind_keyseq("\\e[5~", pageup_handler);
    rl_bind_keyseq("\\e[6~", pagedown_handler);

    rl_bind_keyseq("\\e[5;3~", altpageup_handler);
    rl_bind_keyseq("\\e[6;3~", altpagedown_handler);

    rl_bind_key('\t', tab_handler);

    return 0;
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
    p_rl_timeout.tv_usec = inp_timeout * 1000;
    rl_startup_hook = startup_hook;
    rl_getc_function = prof_rl_getc;
    rl_callback_handler_install(NULL, cb_linehandler);

    signal(SIGWINCH, resize_signal_handler);

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
inp_nonblocking(gboolean reset)
{
    if (! prefs_get_boolean(PREF_INPBLOCK_DYNAMIC)) {
        inp_timeout = prefs_get_inpblock();
        return;
    }

    if (reset) {
        inp_timeout = 0;
        no_input_count = 0;
    }

    if (inp_timeout < prefs_get_inpblock()) {
        no_input_count++;

        if (no_input_count % 10 == 0) {
            inp_timeout += no_input_count;

            if (inp_timeout > prefs_get_inpblock()) {
                inp_timeout = prefs_get_inpblock();
            }
        }
    }
}

void
inp_block(void)
{
    wtimeout(inp_win, -1);
}

void
inp_win_handle_scroll(void)
{
    int col = getcurx(inp_win);
    int wcols = getmaxx(stdscr);

    // if lost cursor off screen, move contents to show it
    if (col >= pad_start + (wcols -2)) {
        pad_start = col - (wcols / 2);
        if (pad_start < 0) {
            pad_start = 0;
        }
    } else if (col <= pad_start) {
        pad_start = pad_start - (wcols / 2);
        if (pad_start < 0) {
            pad_start = 0;
        }
    }
}

void
inp_write(char *line, int offset)
{
    int col = offset_to_col(line, offset);
    werase(inp_win);
    waddstr(inp_win, line);
    wmove(inp_win, 0, col);
    inp_win_handle_scroll();

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
        return TRUE;
    }

    if (FD_ISSET(fileno(rl_instream), &fds)) {
        rl_callback_read_char();

        if (rl_line_buffer && rl_line_buffer[0] != '/' && rl_line_buffer[0] != '\0' && rl_line_buffer[0] != '\n') {
            prof_handle_activity();
        }

        ui_reset_idle_time();
        inp_nonblocking(TRUE);
        inp_write(rl_line_buffer, rl_point);
    } else {
        inp_nonblocking(FALSE);
        prof_handle_idle();
    }

    p_rl_timeout.tv_sec = 0;
    p_rl_timeout.tv_usec = inp_timeout * 1000;

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
    pnoutrefresh(inp_win, 0, pad_start, wrows-1, 0, wrows-1, wcols-2);
}