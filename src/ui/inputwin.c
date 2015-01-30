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

#define KEY_CTRL_A 0001
#define KEY_CTRL_B 0002
#define KEY_CTRL_D 0004
#define KEY_CTRL_E 0005
#define KEY_CTRL_F 0006
#define KEY_CTRL_N 0016
#define KEY_CTRL_P 0020
#define KEY_CTRL_U 0025
#define KEY_CTRL_W 0027

#define MAX_HISTORY 100

static WINDOW *inp_win;

static struct timeval p_rl_timeout;
static fd_set fds;
static int r;
static gboolean cmd_result = TRUE;

// input line
static char line[INP_WIN_MAX];
// current position in the utf8 string
static int line_utf8_pos;

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
    line_utf8_pos = 0;
    line[0] = '\0';
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
inp_write(char *line, int offset)
{
    int col = offset_to_col(line, offset);

    cons_debug("LEN BYTES: %d", strlen(line));
    cons_debug("LEN UTF8 : %d", g_utf8_strlen(line, -1));
    cons_debug("OFFSET   : %d", offset);
    cons_debug("COL      : %d", col);
    cons_debug("");

    werase(inp_win);
    waddstr(inp_win, line);
    wmove(inp_win, 0, col);
    _inp_win_update_virtual();
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
        inp_write(rl_line_buffer, rl_point);
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

//char *
//inp_read(int *key_type, wint_t *ch)
//{
//    // echo off, and get some more input
//    noecho();
//    *key_type = wget_wch(inp_win, ch);
//
//    int bytes_len = strlen(line);
//
//    gboolean in_command = FALSE;
//    if ((bytes_len > 0 && line[0] == '/') ||
//            (bytes_len == 0 && *ch == '/')) {
//        in_command = TRUE;
//    }
//
//    if (*key_type == ERR) {
//        prof_handle_idle();
//    }
//    if ((*key_type != ERR) && (*key_type != KEY_CODE_YES) && !in_command && utf8_is_printable(*ch)) {
//        prof_handle_activity();
//    }
//
//    // if it wasn't an arrow key etc
//    if (!_handle_edit(*key_type, *ch)) {
//        if (utf8_is_printable(*ch) && *key_type != KEY_CODE_YES) {
//            if (bytes_len >= INP_WIN_MAX) {
//                *ch = ERR;
//                return NULL;
//            }
//
//            int col = getcurx(inp_win);
//            int maxx = getmaxx(stdscr);
//            key_printable(line, &line_utf8_pos, &col, &pad_start, *ch, maxx);
//
//            werase(inp_win);
//            waddstr(inp_win, line);
//            wmove(inp_win, 0, col);
//            _inp_win_update_virtual();
//
//            cmd_reset_autocomplete();
//        }
//    }
//
//    echo();
//
//    char *result = NULL;
//    if (*ch == '\n') {
//        result = strdup(line);
//        line[0] = '\0';
//        line_utf8_pos = 0;
//    }
//
//    if (*ch != ERR && *key_type != ERR) {
//        cons_debug("BYTE LEN = %d", strlen(line));
//        cons_debug("UTF8 LEN = %d", utf8_display_len(line));
//        cons_debug("CURR COL = %d", getcurx(inp_win));
//        cons_debug("CURR UNI = %d", line_utf8_pos);
//        cons_debug("");
//    }
//
//    return result;
//}

void
inp_get_password(char *passwd)
{
    werase(inp_win);
    wmove(inp_win, 0, 0);
    pad_start = 0;
    line[0] = '\0';
    line_utf8_pos = 0;
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
    line[0] = '\0';
    line_utf8_pos = 0;
    _inp_win_update_virtual();
}

/*
 * Deal with command editing, return 1 if ch was an edit
 * key press: up, down, left, right or backspace
 * return 0 if it wasn't
 */
//static int
//_handle_edit(int key_type, const wint_t ch)
//{
//    // ALT-LEFT
//    if ((key_type == KEY_CODE_YES) && (ch == 537 || ch == 542)) {
//        ui_previous_win();
//        return 1;
//
//    // ALT-RIGHT
//    } else if ((key_type == KEY_CODE_YES) && (ch == 552 || ch == 557)) {
//        ui_next_win();
//        return 1;
//
//    // other editing keys
//    } else {
//        int bytes_len = strlen(line);
//        int next_ch;
//
//        switch(ch) {
//
//        case 27: // ESC
//            // check for ALT-key
//            next_ch = wgetch(inp_win);
//            if (next_ch != ERR) {
//                return _handle_alt_key(next_ch);
//            } else {
//                werase(inp_win);
//                wmove(inp_win, 0, 0);
//                pad_start = 0;
//                line[0] = '\0';
//                line_utf8_pos = 0;
//                _inp_win_update_virtual();
//                return 1;
//            }
//
//        case 9: // tab
//            if (bytes_len != 0) {
//                line[bytes_len] = '\0';
//                if ((strncmp(line, "/", 1) != 0) && (ui_current_win_type() == WIN_MUC)) {
//                    char *result = muc_autocomplete(line);
//                    if (result) {
//                        werase(inp_win);
//                        wmove(inp_win, 0, 0);
//                        pad_start = 0;
//                        line[0] = '\0';
//                        line_utf8_pos = 0;
//                        strncpy(line, result, INP_WIN_MAX);
//                        waddstr(inp_win, line);
//
//                        int display_len = utf8_display_len(line);
//                        wmove(inp_win, 0, display_len);
//                        line_utf8_pos = g_utf8_strlen(line, -1);
//
//                        int wcols = getmaxx(stdscr);
//                        if (display_len > wcols-2) {
//                            pad_start = display_len - wcols + 1;
//                            _inp_win_update_virtual();
//                        }
//
//                        free(result);
//                    }
//                } else if (strncmp(line, "/", 1) == 0) {
//                    char *result = cmd_autocomplete(line);
//                    if (result) {
//                        werase(inp_win);
//                        wmove(inp_win, 0, 0);
//                        pad_start = 0;
//                        line[0] = '\0';
//                        line_utf8_pos = 0;
//                        strncpy(line, result, INP_WIN_MAX);
//                        waddstr(inp_win, line);
//
//                        int display_len = utf8_display_len(line);
//                        wmove(inp_win, 0, display_len);
//                        line_utf8_pos = g_utf8_strlen(line, -1);
//
//                        int wcols = getmaxx(stdscr);
//                        if (display_len > wcols-2) {
//                            pad_start = display_len - wcols + 1;
//                            _inp_win_update_virtual();
//                        }
//
//                        free(result);
//                    }
//                }
//            }
//            return 1;
//
//        default:
//            return 0;
//        }
//    }
//}

//static int
//_handle_alt_key(int key)
//{
//    switch (key)
//    {
//        case '1':
//            ui_switch_win(1);
//            break;
//        case '2':
//            ui_switch_win(2);
//            break;
//        case '3':
//            ui_switch_win(3);
//            break;
//        case '4':
//            ui_switch_win(4);
//            break;
//        case '5':
//            ui_switch_win(5);
//            break;
//        case '6':
//            ui_switch_win(6);
//            break;
//        case '7':
//            ui_switch_win(7);
//            break;
//        case '8':
//            ui_switch_win(8);
//            break;
//        case '9':
//            ui_switch_win(9);
//            break;
//        case '0':
//            ui_switch_win(0);
//            break;
//        case KEY_LEFT:
//            ui_previous_win();
//            break;
//        case KEY_RIGHT:
//            ui_next_win();
//            break;
//        default:
//            break;
//    }
//    return 1;
//}
//
static void
_inp_win_update_virtual(void)
{
    int wrows, wcols;
    getmaxyx(stdscr, wrows, wcols);
    pnoutrefresh(inp_win, 0, pad_start, wrows-1, 0, wrows-1, wcols-1);
}