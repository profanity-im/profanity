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
 * size  : "       7 "
 * input : " example "
 * inp_x : "012345678"
 * index : " 0123456 " (inp_x - 1)
 */

#include <string.h>
#include <ncurses.h>
#include "windows.h"
#include "history.h"

static WINDOW *inp_win;

static int _handle_edit(int ch, char *input, int *size);
static int _printable(int ch);
static void _replace_input(char *input, char *new_input, int *size);

void create_input_window(void)
{
    int rows, cols;
    getmaxyx(stdscr, rows, cols);

    inp_win = newwin(1, cols, rows-1, 0);
    keypad(inp_win, TRUE);
    wattrset(inp_win, A_BOLD);
    wmove(inp_win, 0, 1);
    wrefresh(inp_win);
}

void inp_clear(void)
{
    wclear(inp_win);
    wmove(inp_win, 0, 1);
    touchwin(inp_win);
    wrefresh(inp_win);
}

void inp_non_block(void)
{
    wtimeout(inp_win, 0);
}

void inp_block(void)
{
    wtimeout(inp_win, -1);
}

void inp_poll_char(int *ch, char *input, int *size)
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
            if (inp_x <= *size) {
                winsch(inp_win, *ch);
                wmove(inp_win, inp_y, inp_x+1);

                for (i = *size; i > inp_x -1; i--)
                    input[i] = input[i-1];
                input[inp_x -1] = *ch;

                (*size)++;

            // otherwise just append
            } else {
                waddch(inp_win, *ch);
                input[(*size)++] = *ch;
            }
        }
    }

    echo();
}

void inp_get_password(char *passwd)
{
    wclear(inp_win);
    noecho();
    mvwgetnstr(inp_win, 0, 1, passwd, 20);
    wmove(inp_win, 0, 1);
    echo();
    status_bar_clear();
}

void inp_put_back(void)
{
    wrefresh(inp_win);
}

/*
 * Deal with command editing, return 1 if ch was an edit
 * key press: up, down, left, right or backspace
 * return 0 if it wasnt
 */
static int _handle_edit(int ch, char *input, int *size)
{
    int i;
    char *prev = NULL;
    char *next = NULL;
    int inp_y = 0;
    int inp_x = 0;
    
    getyx(inp_win, inp_y, inp_x);

    switch(ch) {
    
    case 127:
    case KEY_BACKSPACE:
        if (*size > 0) {

            // if at end, delete last char
            if (inp_x > *size) {
                wmove(inp_win, inp_y, inp_x-1);
                wdelch(inp_win);
                (*size)--;

            // if in middle, delete and shift chars left
            } else if (inp_x > 1 && inp_x <= *size) {
                for (i = inp_x-1; i < *size; i++)
                    input[i-1] = input[i];
                (*size)--;

                inp_clear();
                for (i = 0; i < *size; i++)
                    waddch(inp_win, input[i]);
                wmove(inp_win, 0, inp_x -1);
            }
        }
        return 1;

    case KEY_LEFT:
        if (inp_x > 1)
            wmove(inp_win, inp_y, inp_x-1);
        return 1;

    case KEY_RIGHT:
        if (inp_x <= *size )
            wmove(inp_win, inp_y, inp_x+1);
        return 1;

    case KEY_UP:
        prev = history_previous();
        if (prev)
            _replace_input(input, prev, size);
        return 1;

    case KEY_DOWN:
        next = history_next();
        if (next)
            _replace_input(input, next, size);
        return 1;

    default:
        return 0;
    }
}

static int _printable(int ch)
{
   return (ch != ERR && ch != '\n' &&
            ch != KEY_F(1) && ch != KEY_F(2) && ch != KEY_F(3) && 
            ch != KEY_F(4) && ch != KEY_F(5) && ch != KEY_F(6) &&
            ch != KEY_F(7) && ch != KEY_F(8) && ch != KEY_F(9) &&
            ch != KEY_F(10));
}

static void _replace_input(char *input, char *new_input, int *size)
{
    int i;

    strcpy(input, new_input);
    *size = strlen(input);
    inp_clear();
    for (i = 0; i < *size; i++)
        waddch(inp_win, input[i]);
}
