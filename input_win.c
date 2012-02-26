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

#include <string.h>
#include <ncurses.h>
#include "windows.h"

static WINDOW *inp_win;

void create_input_window(void)
{
    int rows, cols;
    getmaxyx(stdscr, rows, cols);

    inp_win = newwin(1, cols, rows-1, 0);
    keypad(inp_win, TRUE);
    wmove(inp_win, 0, 1);
    wrefresh(inp_win);
}

void inp_clear(void)
{
    wclear(inp_win);
    wmove(inp_win, 0, 1);
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

/*
 * size  : "       7 "
 * input : " example "
 * inp_x : "012345678"
 * index : " 0123456 " (inp_x - 1)
 */
void inp_poll_char(int *ch, char *command, int *size)
{
    int inp_y = 0;
    int inp_x = 0;

    // move cursor back to inp_win
    getyx(inp_win, inp_y, inp_x);
    wmove(inp_win, inp_y, inp_x);

    // echo off, and get some more input
    noecho();
    *ch = wgetch(inp_win);

    // if delete pressed, go back and delete it
    if (*ch == 127) {
        if (*size > 0) {
            getyx(inp_win, inp_y, inp_x);
            wmove(inp_win, inp_y, inp_x-1);
            wdelch(inp_win);
            (*size)--;
        }

    // left arrow
    } else if (*ch == KEY_LEFT) {
        getyx(inp_win, inp_y, inp_x);
        if (inp_x > 1) {
            wmove(inp_win, inp_y, inp_x-1);
        }

    // right arrow
    } else if (*ch == KEY_RIGHT) {
        getyx(inp_win, inp_y, inp_x);
        if (inp_x <= *size ) {
            wmove(inp_win, inp_y, inp_x+1);
        }

    // else if not error, newline or special key, 
    // show it and store it
    } else if (*ch != ERR &&
             *ch != '\n' &&
             *ch != KEY_LEFT &&
             *ch != KEY_RIGHT &&
             *ch != KEY_UP &&
             *ch != KEY_DOWN &&
             *ch != KEY_F(1) &&
             *ch != KEY_F(2) &&
             *ch != KEY_F(3) &&
             *ch != KEY_F(4) &&
             *ch != KEY_F(5) &&
             *ch != KEY_F(6) &&
             *ch != KEY_F(7) &&
             *ch != KEY_F(8) &&
             *ch != KEY_F(9) &&
             *ch != KEY_F(10)) {

        getyx(inp_win, inp_y, inp_x);
       
        // handle insert if not at end of input
        if (inp_x <= *size) {
            winsch(inp_win, *ch);
            wmove(inp_win, inp_y, inp_x+1);

            int i;
            for (i = *size; i > inp_x -1; i--)
                command[i] = command[i-1];
            command[inp_x -1] = *ch;

            (*size)++;

        // otherwise just append
        } else {
            waddch(inp_win, *ch);
            command[(*size)++] = *ch;
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
