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

#include "config.h"

#include <stdlib.h>
#include <string.h>

#ifdef HAVE_NCURSES_H
#include <ncurses.h>
#endif
#ifdef HAVE_NCURSES_NCURSES_H
#include <ncurses/ncurses.h>
#endif

#include "common.h"
#include "command.h"
#include "contact_list.h"
#include "history.h"
#include "preferences.h"
#include "ui.h"

typedef char*(*autocomplete_func)(char *);

static WINDOW *inp_win;
static int pad_start = 0;

static int _handle_edit(const int ch, char *input, int *size);
static int _printable(const int ch);
static void _replace_input(char *input, const char * const new_input, int *size);
static void _parameter_autocomplete(char *input, int *size, char *command,
    autocomplete_func func);

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
    wbkgd(inp_win, COLOR_PAIR(1));
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
    wclear(inp_win);
    wmove(inp_win, 0, 0);
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

void
inp_get_char(int *ch, char *input, int *size)
{
    int inp_y = 0;
    int inp_x = 0;
    int i;

    // echo off, and get some more input
    noecho();
    *ch = wgetch(inp_win);

    // if it wasn't an arrow key etc
    if (!_handle_edit(*ch, input, size)) {
        if (_printable(*ch)) {
            getyx(inp_win, inp_y, inp_x);

            // handle insert if not at end of input
            if (inp_x < *size) {
                winsch(inp_win, *ch);
                wmove(inp_win, inp_y, inp_x+1);

                for (i = *size; i > inp_x; i--)
                    input[i] = input[i-1];
                input[inp_x] = *ch;

                (*size)++;

            // otherwise just append
            } else {
                waddch(inp_win, *ch);
                input[(*size)++] = *ch;

                // if gone over screen size follow input
                int rows, cols;
                getmaxyx(stdscr, rows, cols);
                if (*size - pad_start > cols-2) {
                    pad_start++;
                    prefresh(inp_win, 0, pad_start, rows-1, 0, rows-1, cols-1);
                }
            }

            contact_list_reset_search_attempts();
            prefs_reset_login_search();
            prefs_reset_boolean_choice();
            cmd_help_reset_completer();
            cmd_reset_completer();
        }
    }

    echo();
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

/*
 * Deal with command editing, return 1 if ch was an edit
 * key press: up, down, left, right or backspace
 * return 0 if it wasnt
 */
static int
_handle_edit(const int ch, char *input, int *size)
{
    int i, rows, cols;
    char *prev = NULL;
    char *next = NULL;
    char *found = NULL;
    char *auto_msg = NULL;
    int inp_y = 0;
    int inp_x = 0;
    char inp_cpy[*size];

    getmaxyx(stdscr, rows, cols);
    getyx(inp_win, inp_y, inp_x);

    switch(ch) {

    case 27: // ESC
        *size = 0;
        inp_clear();
        return 1;

    case 127:
    case KEY_BACKSPACE:
        contact_list_reset_search_attempts();
        if (*size > 0) {

            // if at end, delete last char
            if (inp_x >= *size) {
                wmove(inp_win, inp_y, inp_x-1);
                wdelch(inp_win);
                (*size)--;

            // if in middle, delete and shift chars left
            } else if (inp_x > 0 && inp_x < *size) {
                for (i = inp_x; i < *size; i++)
                    input[i-1] = input[i];
                (*size)--;

                inp_clear();
                for (i = 0; i < *size; i++)
                    waddch(inp_win, input[i]);
                wmove(inp_win, 0, inp_x -1);
            }

            // if gone off screen to left, jump left (half a screen worth)
            if (inp_x <= pad_start) {
                pad_start = pad_start - (cols / 2);
                if (pad_start < 0) {
                    pad_start = 0;
                }

                prefresh(inp_win, 0, pad_start, rows-1, 0, rows-1, cols-1);
            }
        }
        return 1;

    case KEY_DC: // DEL
        if (inp_x < *size) {
            wdelch(inp_win);

            // if not last char, shift chars left
            if (inp_x < *size - 1)
                for (i = inp_x; i < *size; i++)
                    input[i] = input[i+1];

            (*size)--;
        }
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
        if (inp_x < *size) {
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
            _replace_input(input, prev, size);
            pad_start = 0;
            prefresh(inp_win, 0, pad_start, rows-1, 0, rows-1, cols-1);
        }
        return 1;

    case KEY_DOWN:
        next = history_next(input, size);
        if (next) {
            _replace_input(input, next, size);
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
        wmove(inp_win, inp_y, *size);
        if (*size > cols-2) {
            pad_start = *size - cols + 1;
            prefresh(inp_win, 0, pad_start, rows-1, 0, rows-1, cols-1);
        }
        return 1;

    case 9: // tab

        // autocomplete command
        if ((strncmp(input, "/", 1) == 0) && (!str_contains(input, *size, ' '))) {
            for(i = 0; i < *size; i++) {
                inp_cpy[i] = input[i];
            }
            inp_cpy[i] = '\0';
            found = cmd_complete(inp_cpy);
            if (found != NULL) {
                auto_msg = (char *) malloc((strlen(found) + 1) * sizeof(char));
                strcpy(auto_msg, found);
                _replace_input(input, auto_msg, size);
                free(auto_msg);
                free(found);
            }
        }

        _parameter_autocomplete(input, size, "/msg",
            contact_list_find_contact);
        _parameter_autocomplete(input, size, "/connect",
            prefs_find_login);
        _parameter_autocomplete(input, size, "/help",
            cmd_help_complete);
        _parameter_autocomplete(input, size, "/beep",
            prefs_autocomplete_boolean_choice);
        _parameter_autocomplete(input, size, "/notify",
            prefs_autocomplete_boolean_choice);
        _parameter_autocomplete(input, size, "/typing",
            prefs_autocomplete_boolean_choice);
        _parameter_autocomplete(input, size, "/flash",
            prefs_autocomplete_boolean_choice);
        _parameter_autocomplete(input, size, "/showsplash",
            prefs_autocomplete_boolean_choice);
        _parameter_autocomplete(input, size, "/chlog",
            prefs_autocomplete_boolean_choice);
        _parameter_autocomplete(input, size, "/history",
            prefs_autocomplete_boolean_choice);
        _parameter_autocomplete(input, size, "/vercheck",
            prefs_autocomplete_boolean_choice);

        return 1;

    default:
        return 0;
    }
}

static int
_printable(const int ch)
{
   return (ch != ERR && ch != '\n' &&
            ch != KEY_PPAGE && ch != KEY_NPAGE &&
            ch != KEY_F(1) && ch != KEY_F(2) && ch != KEY_F(3) &&
            ch != KEY_F(4) && ch != KEY_F(5) && ch != KEY_F(6) &&
            ch != KEY_F(7) && ch != KEY_F(8) && ch != KEY_F(9) &&
            ch != KEY_F(10) && ch!= KEY_F(11) && ch != KEY_F(12) &&
            ch != KEY_IC && ch != KEY_EIC && ch != KEY_RESIZE);
}

static void
_replace_input(char *input, const char * const new_input, int *size)
{
    int i;

    strcpy(input, new_input);
    *size = strlen(input);
    inp_clear();
    for (i = 0; i < *size; i++)
        waddch(inp_win, input[i]);
}

static void
_parameter_autocomplete(char *input, int *size, char *command,
    autocomplete_func func)
{
    char *found = NULL;
    char *auto_msg = NULL;
    char inp_cpy[*size];
    int i;
    char *command_cpy = malloc(strlen(command) + 2);
    sprintf(command_cpy, "%s ", command);
    int len = strlen(command_cpy);
    if ((strncmp(input, command_cpy, len) == 0) && (*size > len)) {
        for(i = len; i < *size; i++) {
            inp_cpy[i-len] = input[i];
        }
        inp_cpy[(*size) - len] = '\0';
        found = func(inp_cpy);
        if (found != NULL) {
            auto_msg = (char *) malloc((len + (strlen(found) + 1)) * sizeof(char));
            strcpy(auto_msg, command_cpy);
            strcat(auto_msg, found);
            _replace_input(input, auto_msg, size);
            free(auto_msg);
            free(found);
        }
    }
    free(command_cpy);
}
