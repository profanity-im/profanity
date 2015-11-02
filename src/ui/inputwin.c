/*
 * inputwin.c
 *
 * Copyright (C) 2012 - 2015 James Booth <boothj5@gmail.com>
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
#include "prof_config.h"

#include <stdlib.h>
#include <string.h>
#include <wchar.h>
#include <sys/time.h>
#include <errno.h>

#include <readline/readline.h>
#include <readline/history.h>

#ifdef PROF_HAVE_NCURSESW_NCURSES_H
#include <ncursesw/ncurses.h>
#elif PROF_HAVE_NCURSES_H
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
#include "ui/window.h"
#include "window_list.h"
#include "xmpp/xmpp.h"

static WINDOW *inp_win;
static int pad_start = 0;

static struct timeval p_rl_timeout;
/* Timeout in ms. Shows how long select() may block. */
static gint inp_timeout = 0;
static gint no_input_count = 0;

static fd_set fds;
static int r;
static char *inp_line = NULL;
static gboolean get_password = FALSE;

static void _inp_win_update_virtual(void);
static int _inp_printable(const wint_t ch);
static void _inp_win_handle_scroll(void);
static int _inp_offset_to_col(char *str, int offset);
static void _inp_write(char *line, int offset);

static int _inp_rl_getc(FILE *stream);
static void _inp_rl_linehandler(char *line);
static int _inp_rl_tab_handler(int count, int key);
static int _inp_rl_clear_handler(int count, int key);
static int _inp_rl_win1_handler(int count, int key);
static int _inp_rl_win2_handler(int count, int key);
static int _inp_rl_win3_handler(int count, int key);
static int _inp_rl_win4_handler(int count, int key);
static int _inp_rl_win5_handler(int count, int key);
static int _inp_rl_win6_handler(int count, int key);
static int _inp_rl_win7_handler(int count, int key);
static int _inp_rl_win8_handler(int count, int key);
static int _inp_rl_win9_handler(int count, int key);
static int _inp_rl_win0_handler(int count, int key);
static int _inp_rl_altleft_handler(int count, int key);
static int _inp_rl_altright_handler(int count, int key);
static int _inp_rl_pageup_handler(int count, int key);
static int _inp_rl_pagedown_handler(int count, int key);
static int _inp_rl_altpageup_handler(int count, int key);
static int _inp_rl_altpagedown_handler(int count, int key);
static int _inp_rl_startup_hook(void);

void
create_input_window(void)
{
#ifdef NCURSES_REENTRANT
    set_escdelay(25);
#else
    ESCDELAY = 25;
#endif
    rl_readline_name = "profanity";
    rl_getc_function = _inp_rl_getc;
    rl_startup_hook = _inp_rl_startup_hook;
    rl_callback_handler_install(NULL, _inp_rl_linehandler);

    inp_win = newpad(1, INP_WIN_MAX);
    wbkgd(inp_win, theme_attrs(THEME_INPUT_TEXT));;
    keypad(inp_win, TRUE);
    wmove(inp_win, 0, 0);

    _inp_win_update_virtual();
}

char*
inp_readline(void)
{
    free(inp_line);
    inp_line = NULL;
    p_rl_timeout.tv_sec = inp_timeout / 1000;
    p_rl_timeout.tv_usec = inp_timeout % 1000 * 1000;
    FD_ZERO(&fds);
    FD_SET(fileno(rl_instream), &fds);
    errno = 0;
    r = select(FD_SETSIZE, &fds, NULL, NULL, &p_rl_timeout);
    if (r < 0) {
        if (errno != EINTR) {
            char *err_msg = strerror(errno);
            log_error("Readline failed: %s", err_msg);
        }
        return NULL;
    }

    if (FD_ISSET(fileno(rl_instream), &fds)) {
        rl_callback_read_char();

        if (rl_line_buffer &&
                rl_line_buffer[0] != '/' &&
                rl_line_buffer[0] != '\0' &&
                rl_line_buffer[0] != '\n') {
            prof_handle_activity();
        }

        ui_reset_idle_time();
        if (!get_password) {
            _inp_write(rl_line_buffer, rl_point);
        }
        inp_nonblocking(TRUE);
    } else {
        inp_nonblocking(FALSE);
        prof_handle_idle();
    }

    if (inp_line) {
        return strdup(inp_line);
    } else {
        return NULL;
    }
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
inp_close(void)
{
    rl_callback_handler_remove();
}

char*
inp_get_line(void)
{
    werase(inp_win);
    wmove(inp_win, 0, 0);
    _inp_win_update_virtual();
    doupdate();
    char *line = NULL;
    while (!line) {
        line = inp_readline();
        ui_update();
    }
    status_bar_clear();
    return line;
}

char*
inp_get_password(void)
{
    werase(inp_win);
    wmove(inp_win, 0, 0);
    _inp_win_update_virtual();
    doupdate();
    char *password = NULL;
    get_password = TRUE;
    while (!password) {
        password = inp_readline();
        ui_update();
    }
    get_password = FALSE;
    status_bar_clear();
    return password;
}

void
inp_put_back(void)
{
    _inp_win_update_virtual();
}

static void
_inp_win_update_virtual(void)
{
    int wrows, wcols;
    getmaxyx(stdscr, wrows, wcols);
    pnoutrefresh(inp_win, 0, pad_start, wrows-1, 0, wrows-1, wcols-2);
}

static void
_inp_write(char *line, int offset)
{
    int col = _inp_offset_to_col(line, offset);
    werase(inp_win);
    waddstr(inp_win, line);
    wmove(inp_win, 0, col);
    _inp_win_handle_scroll();

    _inp_win_update_virtual();
    doupdate();
}

static int
_inp_printable(const wint_t ch)
{
    char bytes[MB_CUR_MAX+1];
    size_t utf_len = wcrtomb(bytes, ch, (mbstate_t*)NULL);
    bytes[utf_len] = '\0';
    gunichar unichar = g_utf8_get_char(bytes);

    return g_unichar_isprint(unichar);
}

static int
_inp_offset_to_col(char *str, int offset)
{
    int i = 0;
    int col = 0;

    while (i < offset && str[i] != '\0') {
        gunichar uni = g_utf8_get_char(&str[i]);
        size_t ch_len = mbrlen(&str[i], 4, NULL);
        i += ch_len;
        col++;
        if (g_unichar_iswide(uni)) {
            col++;
        }
    }

    return col;
}

static void
_inp_win_handle_scroll(void)
{
    int col = getcurx(inp_win);
    int wcols = getmaxx(stdscr);

    if (col == 0) {
        pad_start = 0;
    } else if (col >= pad_start + (wcols -2)) {
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

// Readline callbacks

static int
_inp_rl_startup_hook(void)
{
    rl_bind_keyseq("\\e1", _inp_rl_win1_handler);
    rl_bind_keyseq("\\e2", _inp_rl_win2_handler);
    rl_bind_keyseq("\\e3", _inp_rl_win3_handler);
    rl_bind_keyseq("\\e4", _inp_rl_win4_handler);
    rl_bind_keyseq("\\e5", _inp_rl_win5_handler);
    rl_bind_keyseq("\\e6", _inp_rl_win6_handler);
    rl_bind_keyseq("\\e7", _inp_rl_win7_handler);
    rl_bind_keyseq("\\e8", _inp_rl_win8_handler);
    rl_bind_keyseq("\\e9", _inp_rl_win9_handler);
    rl_bind_keyseq("\\e0", _inp_rl_win0_handler);

    rl_bind_keyseq("\\eOP", _inp_rl_win1_handler);
    rl_bind_keyseq("\\eOQ", _inp_rl_win2_handler);
    rl_bind_keyseq("\\eOR", _inp_rl_win3_handler);
    rl_bind_keyseq("\\eOS", _inp_rl_win4_handler);
    rl_bind_keyseq("\\e[15~", _inp_rl_win5_handler);
    rl_bind_keyseq("\\e[17~", _inp_rl_win6_handler);
    rl_bind_keyseq("\\e[18~", _inp_rl_win7_handler);
    rl_bind_keyseq("\\e[19~", _inp_rl_win8_handler);
    rl_bind_keyseq("\\e[20~", _inp_rl_win9_handler);
    rl_bind_keyseq("\\e[21~", _inp_rl_win0_handler);

    rl_bind_keyseq("\\e[1;9D", _inp_rl_altleft_handler);
    rl_bind_keyseq("\\e[1;3D", _inp_rl_altleft_handler);
    rl_bind_keyseq("\\e\\e[D", _inp_rl_altleft_handler);

    rl_bind_keyseq("\\e[1;9C", _inp_rl_altright_handler);
    rl_bind_keyseq("\\e[1;3C", _inp_rl_altright_handler);
    rl_bind_keyseq("\\e\\e[C", _inp_rl_altright_handler);

    rl_bind_keyseq("\\e\\e[5~", _inp_rl_altpageup_handler);
    rl_bind_keyseq("\\e[5;3~", _inp_rl_altpageup_handler);
    rl_bind_keyseq("\\e\\eOy", _inp_rl_altpageup_handler);

    rl_bind_keyseq("\\e\\e[6~", _inp_rl_altpagedown_handler);
    rl_bind_keyseq("\\e[6;3~", _inp_rl_altpagedown_handler);
    rl_bind_keyseq("\\e\\eOs", _inp_rl_altpagedown_handler);

    rl_bind_keyseq("\\e[5~", _inp_rl_pageup_handler);
    rl_bind_keyseq("\\eOy", _inp_rl_pageup_handler);
    rl_bind_keyseq("\\e[6~", _inp_rl_pagedown_handler);
    rl_bind_keyseq("\\eOs", _inp_rl_pagedown_handler);

    rl_bind_key('\t', _inp_rl_tab_handler);
    rl_bind_key(CTRL('L'), _inp_rl_clear_handler);

    // unbind unwanted mappings
    rl_bind_keyseq("\\e=", NULL);

    return 0;
}

static void
_inp_rl_linehandler(char *line)
{
    if (line && *line) {
        if (!get_password) {
            add_history(line);
        }
    }
    inp_line = line;
}

static int
_inp_rl_getc(FILE *stream)
{
    int ch = rl_getc(stream);
    if (_inp_printable(ch)) {
        ProfWin *window = wins_get_current();
        cmd_reset_autocomplete(window);
    }
    return ch;
}

static int
_inp_rl_clear_handler(int count, int key)
{
    ProfWin *win = wins_get_current();
    win_clear(win);
    return 0;
}

static int
_inp_rl_tab_handler(int count, int key)
{
    if (rl_point != rl_end || !rl_line_buffer) {
        return 0;
    }

    ProfWin *current = wins_get_current();
    if ((strncmp(rl_line_buffer, "/", 1) != 0) && (current->type == WIN_MUC)) {
        char *result = muc_autocomplete(current, rl_line_buffer);
        if (result) {
            rl_replace_line(result, 0);
            rl_point = rl_end;
            free(result);
        }
    } else if (strncmp(rl_line_buffer, "/", 1) == 0) {
        ProfWin *window = wins_get_current();
        char *result = cmd_autocomplete(window, rl_line_buffer);
        if (result) {
            rl_replace_line(result, 0);
            rl_point = rl_end;
            free(result);
        }
    }

    return 0;
}

static void
_go_to_win(int i)
{
    ProfWin *window = wins_get_by_num(i);
    if (window) {
        ui_focus_win(window);
    }
}

static int
_inp_rl_win1_handler(int count, int key)
{
    _go_to_win(1);
    return 0;
}

static int
_inp_rl_win2_handler(int count, int key)
{
    _go_to_win(2);
    return 0;
}

static int
_inp_rl_win3_handler(int count, int key)
{
    _go_to_win(3);
    return 0;
}

static int
_inp_rl_win4_handler(int count, int key)
{
    _go_to_win(4);
    return 0;
}

static int
_inp_rl_win5_handler(int count, int key)
{
    _go_to_win(5);
    return 0;
}

static int
_inp_rl_win6_handler(int count, int key)
{
    _go_to_win(6);
    return 0;
}

static int
_inp_rl_win7_handler(int count, int key)
{
    _go_to_win(7);
    return 0;
}

static int
_inp_rl_win8_handler(int count, int key)
{
    _go_to_win(8);
    return 0;
}

static int
_inp_rl_win9_handler(int count, int key)
{
    _go_to_win(9);
    return 0;
}

static int
_inp_rl_win0_handler(int count, int key)
{
    _go_to_win(0);
    return 0;
}

static int
_inp_rl_altleft_handler(int count, int key)
{
    ProfWin *window = wins_get_previous();
    if (window) {
        ui_focus_win(window);
    }
    return 0;
}

static int
_inp_rl_altright_handler(int count, int key)
{
    ProfWin *window = wins_get_next();
    if (window) {
        ui_focus_win(window);
    }
    return 0;
}

static int
_inp_rl_pageup_handler(int count, int key)
{
    ProfWin *current = wins_get_current();
    win_page_up(current);
    return 0;
}

static int
_inp_rl_pagedown_handler(int count, int key)
{
    ProfWin *current = wins_get_current();
    win_page_down(current);
    return 0;
}

static int
_inp_rl_altpageup_handler(int count, int key)
{
    ProfWin *current = wins_get_current();
    win_sub_page_up(current);
    return 0;
}

static int
_inp_rl_altpagedown_handler(int count, int key)
{
    ProfWin *current = wins_get_current();
    win_sub_page_down(current);
    return 0;
}
