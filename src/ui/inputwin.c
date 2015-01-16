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

#define _inp_win_update_virtual() pnoutrefresh(inp_win, 0, pad_start, rows-1, 0, rows-1, cols-1)

#define KEY_CTRL_A 0001
#define KEY_CTRL_B 0002
#define KEY_CTRL_D 0004
#define KEY_CTRL_E 0005
#define KEY_CTRL_F 0006
#define KEY_CTRL_N 0016
#define KEY_CTRL_P 0020
#define KEY_CTRL_U 0025
#define KEY_CTRL_W 0027

#define INP_WIN_MAX 1000

static WINDOW *inp_win;
static int pad_start = 0;
static int rows, cols;
static char line[INP_WIN_MAX];
static int inp_size;

static int _handle_edit(int key_type, const wint_t ch);
static int _handle_alt_key(int key);
static void _handle_backspace(void);
static int _printable(const wint_t ch);
static void _clear_input(void);
static void _go_to_end(void);
static void _delete_previous_word(void);
static int _get_display_length(void);

void
create_input_window(void)
{
#ifdef NCURSES_REENTRANT
    set_escdelay(25);
#else
    ESCDELAY = 25;
#endif
    getmaxyx(stdscr, rows, cols);
    inp_win = newpad(1, INP_WIN_MAX);
    wbkgd(inp_win, theme_attrs(THEME_INPUT_TEXT));;
    keypad(inp_win, TRUE);
    wmove(inp_win, 0, 0);
    _inp_win_update_virtual();
}

void
inp_win_resize(void)
{
    int inp_x;
    getmaxyx(stdscr, rows, cols);
    inp_x = getcurx(inp_win);

    // if lost cursor off screen, move contents to show it
    if (inp_x >= pad_start + cols) {
        pad_start = inp_x - (cols / 2);
        if (pad_start < 0) {
            pad_start = 0;
        }
    }

    wbkgd(inp_win, theme_attrs(THEME_INPUT_TEXT));;
    _inp_win_update_virtual();
}

void
inp_non_block(gint timeout)
{
    wtimeout(inp_win, timeout);
}

void
inp_block(void)
{
    wtimeout(inp_win, -1);
}

char *
inp_read(int *key_type, wint_t *ch)
{
    int display_size = _get_display_length();

    // echo off, and get some more input
    noecho();
    *key_type = wget_wch(inp_win, ch);

    gboolean in_command = FALSE;
    if ((display_size > 0 && line[0] == '/') ||
            (display_size == 0 && *ch == '/')) {
        in_command = TRUE;
    }

    if (*key_type == ERR) {
        prof_handle_idle();
    }
    if ((*key_type != ERR) && (*key_type != KEY_CODE_YES) && !in_command && _printable(*ch)) {
        prof_handle_activity();
    }

    // if it wasn't an arrow key etc
    if (!_handle_edit(*key_type, *ch)) {
        if (_printable(*ch) && *key_type != KEY_CODE_YES) {
            if (inp_size >= INP_WIN_MAX) {
                *ch = ERR;
                return NULL;
            }

            int inp_x = getcurx(inp_win);

            // handle insert if not at end of input
            if (inp_x < display_size) {
                char bytes[MB_CUR_MAX];
                size_t utf_len = wcrtomb(bytes, *ch, NULL);

                char *next_ch = g_utf8_offset_to_pointer(line, inp_x);
                char *offset;
                for (offset = &line[inp_size - 1]; offset >= next_ch; offset--) {
                    *(offset + utf_len) = *offset;
                }
                int i;
                for (i = 0; i < utf_len; i++) {
                     *(next_ch + i) = bytes[i];
                }

                inp_size += utf_len;
                line[inp_size] = '\0';
                waddstr(inp_win, next_ch);
                wmove(inp_win, 0, inp_x + 1);

                if (inp_x - pad_start > cols-3) {
                    pad_start++;
                    _inp_win_update_virtual();
                }

            // otherwise just append
            } else {
                char bytes[MB_CUR_MAX+1];
                size_t utf_len = wcrtomb(bytes, *ch, NULL);

                // wcrtomb can return (size_t) -1
                if (utf_len < MB_CUR_MAX) {
                    int i;
                    for (i = 0 ; i < utf_len; i++) {
                        line[inp_size++] = bytes[i];
                    }
                    line[inp_size] = '\0';

                    bytes[utf_len] = '\0';
                    waddstr(inp_win, bytes);
                    display_size++;

                    // if gone over screen size follow input
                    int rows, cols;
                    getmaxyx(stdscr, rows, cols);
                    if (display_size - pad_start > cols-2) {
                        pad_start++;
                        _inp_win_update_virtual();
                    }
                }
            }

            cmd_reset_autocomplete();
        }
    }

    echo();

    if (*ch == '\n') {
        line[inp_size++] = '\0';
        inp_size = 0;
        return strdup(line);
    } else {
        return NULL;
    }
}

void
inp_get_password(char *passwd)
{
    _clear_input();
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
inp_replace_input(char *input, const char * const new_input, int *size)
{
    strncpy(input, new_input, INP_WIN_MAX);
    *size = strlen(input);
    inp_win_reset();
    input[*size] = '\0';
    waddstr(inp_win, input);
    _go_to_end();
}

void
inp_win_reset(void)
{
    _clear_input();
    pad_start = 0;
    _inp_win_update_virtual();
}

static void
_clear_input(void)
{
    werase(inp_win);
    wmove(inp_win, 0, 0);
}

/*
 * Deal with command editing, return 1 if ch was an edit
 * key press: up, down, left, right or backspace
 * return 0 if it wasn't
 */
static int
_handle_edit(int key_type, const wint_t ch)
{
    char *prev = NULL;
    char *next = NULL;
    int inp_x = getcurx(inp_win);
    int next_ch;

    int display_size = _get_display_length();

    // CTRL-LEFT
    if ((key_type == KEY_CODE_YES) && (ch == 547 || ch == 545 || ch == 544 || ch == 540 || ch == 539) && (inp_x > 0)) {
        line[inp_size] = '\0';
        gchar *curr_ch = g_utf8_offset_to_pointer(line, inp_x);
        curr_ch = g_utf8_find_prev_char(line, curr_ch);
        gchar *prev_ch;
        gunichar curr_uni;
        gunichar prev_uni;

        while (curr_ch != NULL) {
            curr_uni = g_utf8_get_char(curr_ch);

            if (g_unichar_isspace(curr_uni)) {
                curr_ch = g_utf8_find_prev_char(line, curr_ch);
            } else {
                prev_ch = g_utf8_find_prev_char(line, curr_ch);
                if (prev_ch == NULL) {
                    curr_ch = NULL;
                    break;
                } else {
                    prev_uni = g_utf8_get_char(prev_ch);
                    if (g_unichar_isspace(prev_uni)) {
                        break;
                    } else {
                        curr_ch = prev_ch;
                    }
                }
            }
        }

        if (curr_ch == NULL) {
            inp_x = 0;
            wmove(inp_win, 0, inp_x);
        } else {
            glong offset = g_utf8_pointer_to_offset(line, curr_ch);
            inp_x = offset;
            wmove(inp_win, 0, inp_x);
        }

        // if gone off screen to left, jump left (half a screen worth)
        if (inp_x <= pad_start) {
            pad_start = pad_start - (cols / 2);
            if (pad_start < 0) {
                pad_start = 0;
            }

            _inp_win_update_virtual();
        }
        return 1;

    // CTRL-RIGHT
    } else if ((key_type == KEY_CODE_YES) && (ch == 562 || ch == 560 || ch == 555 || ch == 559 || ch == 554) && (inp_x < display_size)) {
        line[inp_size] = '\0';
        gchar *curr_ch = g_utf8_offset_to_pointer(line, inp_x);
        gchar *next_ch = g_utf8_find_next_char(curr_ch, NULL);
        gunichar curr_uni;
        gunichar next_uni;
        gboolean moved = FALSE;

        while (g_utf8_pointer_to_offset(line, next_ch) < display_size) {
            curr_uni = g_utf8_get_char(curr_ch);
            next_uni = g_utf8_get_char(next_ch);
            curr_ch = next_ch;
            next_ch = g_utf8_find_next_char(next_ch, NULL);

            if (!g_unichar_isspace(curr_uni) && g_unichar_isspace(next_uni) && moved) {
                break;
            } else {
                moved = TRUE;
            }
        }

        if (next_ch == NULL) {
            inp_x = display_size;
            wmove(inp_win, 0, inp_x);
        } else {
            glong offset = g_utf8_pointer_to_offset(line, curr_ch);
            if (offset == display_size - 1) {
                inp_x = offset + 1;
            } else {
                inp_x = offset;
            }
            wmove(inp_win, 0, inp_x);
        }

        // if gone off screen to right, jump right (half a screen worth)
        if (inp_x > pad_start + cols) {
            pad_start = pad_start + (cols / 2);
            _inp_win_update_virtual();
        }

        return 1;

    // ALT-LEFT
    } else if ((key_type == KEY_CODE_YES) && (ch == 537 || ch == 542)) {
        ui_previous_win();
        return 1;

    // ALT-RIGHT
    } else if ((key_type == KEY_CODE_YES) && (ch == 552 || ch == 557)) {
        ui_next_win();
        return 1;

    // other editing keys
    } else {
        switch(ch) {

        case 27: // ESC
            // check for ALT-key
            next_ch = wgetch(inp_win);
            if (next_ch != ERR) {
                return _handle_alt_key(next_ch);
            } else {
                inp_size = 0;
                inp_win_reset();
                return 1;
            }

        case 127:
            _handle_backspace();
            return 1;
        case KEY_BACKSPACE:
            if (key_type != KEY_CODE_YES) {
                return 0;
            }
            _handle_backspace();
            return 1;

        case KEY_DC: // DEL
            if (key_type != KEY_CODE_YES) {
                return 0;
            }
        case KEY_CTRL_D:
            if (inp_x == display_size-1) {
                gchar *start = g_utf8_substring(line, 0, inp_x);
                for (inp_size = 0; inp_size < strlen(start); inp_size++) {
                    line[inp_size] = start[inp_size];
                }
                line[inp_size] = '\0';

                g_free(start);

                _clear_input();
                waddstr(inp_win, line);
            } else if (inp_x < display_size-1) {
                gchar *start = g_utf8_substring(line, 0, inp_x);
                gchar *end = g_utf8_substring(line, inp_x+1, inp_size);
                GString *new = g_string_new(start);
                g_string_append(new, end);

                for (inp_size = 0; inp_size < strlen(new->str); inp_size++) {
                    line[inp_size] = new->str[inp_size];
                }
                line[inp_size] = '\0';

                g_free(start);
                g_free(end);
                g_string_free(new, FALSE);

                _clear_input();
                waddstr(inp_win, line);
                wmove(inp_win, 0, inp_x);
            }
            return 1;

        case KEY_LEFT:
            if (key_type != KEY_CODE_YES) {
                return 0;
            }
        case KEY_CTRL_B:
            if (inp_x > 0) {
                wmove(inp_win, 0, inp_x-1);

                // current position off screen to left
                if (inp_x - 1 < pad_start) {
                    pad_start--;
                    _inp_win_update_virtual();
                }
            }
            return 1;

        case KEY_RIGHT:
            if (key_type != KEY_CODE_YES) {
                return 0;
            }
        case KEY_CTRL_F:
            if (inp_x < display_size) {
                wmove(inp_win, 0, inp_x+1);

                // current position off screen to right
                if ((inp_x + 1 - pad_start) >= cols) {
                    pad_start++;
                    _inp_win_update_virtual();
                }
            }
            return 1;

        case KEY_UP:
            if (key_type != KEY_CODE_YES) {
                return 0;
            }
        case KEY_CTRL_P:
            prev = cmd_history_previous(line, &inp_size);
            if (prev) {
                inp_replace_input(line, prev, &inp_size);
            }
            return 1;

        case KEY_DOWN:
            if (key_type != KEY_CODE_YES) {
                return 0;
            }
        case KEY_CTRL_N:
            next = cmd_history_next(line, &inp_size);
            if (next) {
                inp_replace_input(line, next, &inp_size);
            } else if (inp_size != 0) {
                line[inp_size] = '\0';
                cmd_history_append(line);
                inp_replace_input(line, "", &inp_size);
            }
            return 1;

        case KEY_HOME:
            if (key_type != KEY_CODE_YES) {
                return 0;
            }
        case KEY_CTRL_A:
            wmove(inp_win, 0, 0);
            pad_start = 0;
            _inp_win_update_virtual();
            return 1;

        case KEY_END:
            if (key_type != KEY_CODE_YES) {
                return 0;
            }
        case KEY_CTRL_E:
            _go_to_end();
            return 1;

        case 9: // tab
            if (inp_size != 0) {
                if ((strncmp(line, "/", 1) != 0) && (ui_current_win_type() == WIN_MUC)) {
                    muc_autocomplete(line, &inp_size);
                } else if (strncmp(line, "/", 1) == 0) {
                    cmd_autocomplete(line, &inp_size);
                }
            }
            return 1;

        case KEY_CTRL_W:
            _delete_previous_word();
            return 1;
            break;

        case KEY_CTRL_U:
            while (getcurx(inp_win) > 0) {
                _delete_previous_word();
            }
            return 1;
            break;

        default:
            return 0;
        }
    }
}

static void
_handle_backspace(void)
{
    int inp_x = getcurx(inp_win);
    int display_size = _get_display_length();
    roster_reset_search_attempts();
    if (display_size > 0) {

        // if at end, delete last char
        if (inp_x >= display_size) {
            gchar *start = g_utf8_substring(line, 0, inp_x-1);
            for (inp_size = 0; inp_size < strlen(start); inp_size++) {
                line[inp_size] = start[inp_size];
            }
            line[inp_size] = '\0';

            g_free(start);

            _clear_input();
            waddstr(inp_win, line);
            wmove(inp_win, 0, inp_x -1);

        // if in middle, delete and shift chars left
        } else if (inp_x > 0 && inp_x < display_size) {
            gchar *start = g_utf8_substring(line, 0, inp_x - 1);
            gchar *end = g_utf8_substring(line, inp_x, inp_size);
            GString *new = g_string_new(start);
            g_string_append(new, end);

            for (inp_size = 0; inp_size < strlen(new->str); inp_size++) {
                line[inp_size] = new->str[inp_size];
            }
            line[inp_size] = '\0';

            g_free(start);
            g_free(end);
            g_string_free(new, FALSE);

            _clear_input();
            waddstr(inp_win, line);
            wmove(inp_win, 0, inp_x -1);
        }

        // if gone off screen to left, jump left (half a screen worth)
        if (inp_x <= pad_start) {
            pad_start = pad_start - (cols / 2);
            if (pad_start < 0) {
                pad_start = 0;
            }

            _inp_win_update_virtual();
        }
    }

}

static int
_handle_alt_key(int key)
{
    switch (key)
    {
        case '1':
            ui_switch_win(1);
            break;
        case '2':
            ui_switch_win(2);
            break;
        case '3':
            ui_switch_win(3);
            break;
        case '4':
            ui_switch_win(4);
            break;
        case '5':
            ui_switch_win(5);
            break;
        case '6':
            ui_switch_win(6);
            break;
        case '7':
            ui_switch_win(7);
            break;
        case '8':
            ui_switch_win(8);
            break;
        case '9':
            ui_switch_win(9);
            break;
        case '0':
            ui_switch_win(0);
            break;
        case KEY_LEFT:
            ui_previous_win();
            break;
        case KEY_RIGHT:
            ui_next_win();
            break;
        case 263:
        case 127:
            _delete_previous_word();
            break;
        default:
            break;
    }
    return 1;
}

static void
_delete_previous_word(void)
{
    int end_del = getcurx(inp_win);
    int start_del = end_del;

    line[inp_size] = '\0';
    gchar *curr_ch = g_utf8_offset_to_pointer(line, end_del);
    curr_ch = g_utf8_find_prev_char(line, curr_ch);
    gchar *prev_ch;
    gunichar curr_uni;
    gunichar prev_uni;

    while (curr_ch != NULL) {
        curr_uni = g_utf8_get_char(curr_ch);

        if (g_unichar_isspace(curr_uni)) {
            curr_ch = g_utf8_find_prev_char(line, curr_ch);
        } else {
            prev_ch = g_utf8_find_prev_char(line, curr_ch);
            if (prev_ch == NULL) {
                curr_ch = NULL;
                break;
            } else {
                prev_uni = g_utf8_get_char(prev_ch);
                if (g_unichar_isspace(prev_uni)) {
                    break;
                } else {
                    curr_ch = prev_ch;
                }
            }
        }
    }

    if (curr_ch == NULL) {
        start_del = 0;
    } else {
        start_del = g_utf8_pointer_to_offset(line, curr_ch);
    }

    gint len = g_utf8_strlen(line, -1);
    gchar *start_string = g_utf8_substring(line, 0, start_del);
    gchar *end_string = g_utf8_substring(line, end_del, len);

    int i;
    for (i = 0; i < strlen(start_string); i++) {
        line[i] = start_string[i];
    }
    for (i = 0; i < strlen(end_string); i++) {
        line[strlen(start_string)+i] = end_string[i];
    }

    inp_size = strlen(start_string)+i;
    line[inp_size] = '\0';

    _clear_input();
    waddstr(inp_win, line);
    wmove(inp_win, 0, start_del);

    // if gone off screen to left, jump left (half a screen worth)
    if (start_del <= pad_start) {
        pad_start = pad_start - (cols / 2);
        if (pad_start < 0) {
            pad_start = 0;
        }

        _inp_win_update_virtual();
    }
}

static void
_go_to_end(void)
{
    int display_size = _get_display_length();
    wmove(inp_win, 0, display_size);
    if (display_size > cols-2) {
        pad_start = display_size - cols + 1;
        _inp_win_update_virtual();
    }
}

static int
_printable(const wint_t ch)
{
    char bytes[MB_CUR_MAX+1];
    size_t utf_len = wcrtomb(bytes, ch, NULL);
    bytes[utf_len] = '\0';
    gunichar unichar = g_utf8_get_char(bytes);
    return g_unichar_isprint(unichar) && (ch != KEY_MOUSE);
}

static int
_get_display_length(void)
{
    if (inp_size != 0) {
        return g_utf8_strlen(line, inp_size);
    } else {
        return 0;
    }
}
