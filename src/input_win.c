/*
 * input_win.c
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

/*
 * Non blocking input char handling
 *
 * *size  - holds the current size of input
 * *input - holds the current input string, NOT null terminated at this point
 * *ch    - getch will put a charater here if there was any input
 *
 * The example below shows the values of size, input, a call to wgetyx to
 * find the current cursor location, and the index of the input string.
 *
 * view         :    |mple|
 * input        : "example te"
 * index        : "0123456789"
 * inp_x        : "0123456789"
 * size         : 10
 * pad_start    : 3
 * cols         : 4
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

#include "common.h"
#include "command.h"
#include "contact_list.h"
#include "history.h"
#include "log.h"
#include "preferences.h"
#include "profanity.h"
#include "theme.h"
#include "ui.h"

static WINDOW *inp_win;
static int pad_start = 0;

static int _handle_edit(const wint_t ch, char *input, int *size);
static int _printable(const wint_t ch);
static gboolean _special_key(const wint_t ch);
static void _inp_clear_no_pad(void);

void
create_input_window(void)
{
#ifdef NCURSES_REENTRANT
    set_escdelay(25);
#else
    ESCDELAY = 25;
#endif

    int rows, cols;
    getmaxyx(stdscr, rows, cols);

    inp_win = newpad(1, INP_WIN_MAX);
    wbkgd(inp_win, COLOUR_INPUT_TEXT);
    keypad(inp_win, TRUE);
    wmove(inp_win, 0, 0);
    prefresh(inp_win, 0, pad_start, rows-1, 0, rows-1, cols-1);
}

void
inp_win_resize(const char * const input, const int size)
{
    int rows, cols, inp_x;
    getmaxyx(stdscr, rows, cols);
    inp_x = getcurx(inp_win);

    // if lost cursor off screen, move contents to show it
    if (inp_x >= pad_start + cols) {
        pad_start = inp_x - (cols / 2);
        if (pad_start < 0) {
            pad_start = 0;
        }
    }

    prefresh(inp_win, pad_start, 0, rows-1, 0, rows-1, cols-1);
}

void
inp_clear(void)
{
    int rows, cols;
    getmaxyx(stdscr, rows, cols);
    _inp_clear_no_pad();
    pad_start = 0;
    prefresh(inp_win, 0, pad_start, rows-1, 0, rows-1, cols-1);
}

void
inp_non_block(void)
{
    wtimeout(inp_win, 20);
}

void
inp_block(void)
{
    wtimeout(inp_win, -1);
}

wint_t
inp_get_char(char *input, int *size)
{
    int inp_y = 0;
    int inp_x = 0;
    int i;
    wint_t ch;
    int display_size = 0;

    if (*size != 0) {
        display_size = g_utf8_strlen(input, *size);
    }

    // echo off, and get some more input
    noecho();
    wget_wch(inp_win, &ch);

    gboolean in_command = FALSE;

    if ((display_size > 0 && input[0] == '/') ||
            (display_size == 0 && ch == '/')) {
        in_command = TRUE;
    }

    if (prefs_get_states()) {
        if (ch == ERR) {
            prof_handle_idle();
        }
        if (prefs_get_outtype() && (ch != ERR) && !in_command
                                                && _printable(ch)) {
            prof_handle_activity();
        }
    }

    // if it wasn't an arrow key etc
    if (!_handle_edit(ch, input, size)) {
        if (_printable(ch)) {
            int rows, cols;
            getmaxyx(stdscr, rows, cols);
            getyx(inp_win, inp_y, inp_x);

            // handle insert if not at end of input
            if (inp_x < display_size) {
                char bytes[MB_CUR_MAX];
                size_t utf_len = wcrtomb(bytes, ch, NULL);

                char *next_ch = g_utf8_offset_to_pointer(input, inp_x);
                char *offset;
                for (offset = &input[*size - 1]; offset >= next_ch; offset--) {
                    *(offset + utf_len) = *offset;
                }
                for (i = 0; i < utf_len; i++) {
                     *(next_ch + i) = bytes[i];
                }

                *size += utf_len;
                input[*size] = '\0';
                wprintw(inp_win, next_ch);
                wmove(inp_win, inp_y, inp_x + 1);

                if (inp_x - pad_start > cols-3) {
                    pad_start++;
                    prefresh(inp_win, 0, pad_start, rows-1, 0, rows-1, cols-1);
                }

            // otherwise just append
            } else {
                char bytes[MB_CUR_MAX];
                size_t utf_len = wcrtomb(bytes, ch, NULL);
                for (i = 0 ; i < utf_len; i++) {
                    input[(*size)++] = bytes[i];
                }
                input[*size] = '\0';
                wprintw(inp_win, bytes);

                display_size++;

                // if gone over screen size follow input
                if (display_size - pad_start > cols-2) {
                    pad_start++;
                    prefresh(inp_win, 0, pad_start, rows-1, 0, rows-1, cols-1);
                }
            }

            cmd_reset_autocomplete();
        }
    }

    echo();

    return ch;
}

void
inp_get_password(char *passwd)
{
    int rows, cols;
    getmaxyx(stdscr, rows, cols);

    wclear(inp_win);
    prefresh(inp_win, 0, pad_start, rows-1, 0, rows-1, cols-1);
    noecho();
    mvwgetnstr(inp_win, 0, 1, passwd, 20);
    wmove(inp_win, 0, 0);
    echo();
    status_bar_clear();
}

void
inp_put_back(void)
{
    int rows, cols;
    getmaxyx(stdscr, rows, cols);
    prefresh(inp_win, 0, pad_start, rows-1, 0, rows-1, cols-1);
}

int
inp_get_next_char(void)
{
    return wgetch(inp_win);
}

void
inp_replace_input(char *input, const char * const new_input, int *size)
{
    strcpy(input, new_input);
    *size = strlen(input);
    inp_clear();
    input[*size] = '\0';
    wprintw(inp_win, input);
}


/*
 * Deal with command editing, return 1 if ch was an edit
 * key press: up, down, left, right or backspace
 * return 0 if it wasnt
 */
static int
_handle_edit(const wint_t ch, char *input, int *size)
{
    int rows, cols;
    char *prev = NULL;
    char *next = NULL;
    int inp_y = 0;
    int inp_x = 0;
    int next_ch;
    int display_size = 0;

    if (*size != 0) {
        display_size = g_utf8_strlen(input, *size);
    }

    getmaxyx(stdscr, rows, cols);
    getyx(inp_win, inp_y, inp_x);

    switch(ch) {

    case 27: // ESC
        // check for ALT-num
        next_ch = inp_get_next_char();
        if (next_ch != ERR) {
            switch (next_ch)
            {
                case '1':
                    ui_switch_win(0);
                    break;
                case '2':
                    ui_switch_win(1);
                    break;
                case '3':
                    ui_switch_win(2);
                    break;
                case '4':
                    ui_switch_win(3);
                    break;
                case '5':
                    ui_switch_win(4);
                    break;
                case '6':
                    ui_switch_win(5);
                    break;
                case '7':
                    ui_switch_win(6);
                    break;
                case '8':
                    ui_switch_win(7);
                    break;
                case '9':
                    ui_switch_win(8);
                    break;
                case '0':
                    ui_switch_win(9);
                    break;
                default:
                    break;
            }
            return 1;
        } else {
            *size = 0;
            inp_clear();
            return 1;
        }

    case 127:
    case KEY_BACKSPACE:
        contact_list_reset_search_attempts();
        if (display_size > 0) {

            // if at end, delete last char
            if (inp_x >= display_size) {
                gchar *start = g_utf8_substring(input, 0, inp_x-1);
                for (*size = 0; *size < strlen(start); (*size)++) {
                    input[*size] = start[*size];
                }
                input[*size] = '\0';

                g_free(start);

                _inp_clear_no_pad();
                wprintw(inp_win, input);
                wmove(inp_win, 0, inp_x -1);

            // if in middle, delete and shift chars left
            } else if (inp_x > 0 && inp_x < display_size) {
                gchar *start = g_utf8_substring(input, 0, inp_x - 1);
                gchar *end = g_utf8_substring(input, inp_x, *size);
                GString *new = g_string_new(start);
                g_string_append(new, end);

                for (*size = 0; *size < strlen(new->str); (*size)++) {
                    input[*size] = new->str[*size];
                }
                input[*size] = '\0';

                g_free(start);
                g_free(end);
                g_string_free(new, FALSE);

                _inp_clear_no_pad();
                wprintw(inp_win, input);
                wmove(inp_win, 0, inp_x -1);
            }

            // if gone off screen to left, jump left (half a screen worth)
            if (inp_x <= pad_start) {
                pad_start = pad_start - (cols / 2);
                if (pad_start < 0) {
                    pad_start = 0;
                }
            }

            prefresh(inp_win, 0, pad_start, rows-1, 0, rows-1, cols-1);
        }
        return 1;

    case KEY_DC: // DEL
        if (inp_x == display_size-1) {
            gchar *start = g_utf8_substring(input, 0, inp_x);
            for (*size = 0; *size < strlen(start); (*size)++) {
                input[*size] = start[*size];
            }
            input[*size] = '\0';

            g_free(start);

            _inp_clear_no_pad();
            wprintw(inp_win, input);

            if (pad_start > 0) {
                --pad_start;
            }

        } else if (inp_x < display_size-1) {
            gchar *start = g_utf8_substring(input, 0, inp_x);
            gchar *end = g_utf8_substring(input, inp_x+1, *size);
            GString *new = g_string_new(start);
            g_string_append(new, end);

            for (*size = 0; *size < strlen(new->str); (*size)++) {
                input[*size] = new->str[*size];
            }
            input[*size] = '\0';

            g_free(start);
            g_free(end);
            g_string_free(new, FALSE);

            _inp_clear_no_pad();
            wprintw(inp_win, input);
            wmove(inp_win, 0, inp_x);
        }

        prefresh(inp_win, 0, pad_start, rows-1, 0, rows-1, cols-1);

        return 1;

    case KEY_LEFT:
        if (inp_x > 0)
            wmove(inp_win, inp_y, inp_x-1);

        // current position off screen to left
        if (inp_x - 1 < pad_start) {
            pad_start--;
            prefresh(inp_win, 0, pad_start, rows-1, 0, rows-1, cols-1);
        }
        return 1;

    case KEY_RIGHT:
        if (inp_x < display_size) {
            wmove(inp_win, inp_y, inp_x+1);

            // current position off screen to right
            if ((inp_x + 1 - pad_start) >= cols) {
                pad_start++;
                prefresh(inp_win, 0, pad_start, rows-1, 0, rows-1, cols-1);
            }
        }
        return 1;

    case KEY_UP:
        prev = history_previous(input, size);
        if (prev) {
            inp_replace_input(input, prev, size);
            pad_start = 0;
            prefresh(inp_win, 0, pad_start, rows-1, 0, rows-1, cols-1);
        }
        return 1;

    case KEY_DOWN:
        next = history_next(input, size);
        if (next) {
            inp_replace_input(input, next, size);
            pad_start = 0;
            prefresh(inp_win, 0, pad_start, rows-1, 0, rows-1, cols-1);
        }
        return 1;

    case KEY_HOME:
        wmove(inp_win, inp_y, 0);
        pad_start = 0;
        prefresh(inp_win, 0, pad_start, rows-1, 0, rows-1, cols-1);
        return 1;

    case KEY_END:
        wmove(inp_win, inp_y, display_size);
        if (display_size > cols-2) {
            pad_start = display_size - cols + 1;
            prefresh(inp_win, 0, pad_start, rows-1, 0, rows-1, cols-1);
        }
        return 1;

    case 9: // tab
        cmd_autocomplete(input, size);
        return 1;

    default:
        return 0;
    }
}

static int
_printable(const wint_t ch)
{
   return (ch != ERR && ch != '\n' &&
            ch != KEY_PPAGE && ch != KEY_NPAGE && ch != KEY_MOUSE &&
            ch != KEY_F(1) && ch != KEY_F(2) && ch != KEY_F(3) &&
            ch != KEY_F(4) && ch != KEY_F(5) && ch != KEY_F(6) &&
            ch != KEY_F(7) && ch != KEY_F(8) && ch != KEY_F(9) &&
            ch != KEY_F(10) && ch!= KEY_F(11) && ch != KEY_F(12) &&
            ch != KEY_IC && ch != KEY_EIC && ch != KEY_RESIZE &&
            !_special_key(ch));
}

static gboolean
_special_key(const wint_t ch)
{
    char *str = unctrl(ch);
    return ((strlen(str) > 1) && g_str_has_prefix(str, "^"));
}

static void
_inp_clear_no_pad(void)
{
    wclear(inp_win);
    wmove(inp_win, 0, 0);
}
