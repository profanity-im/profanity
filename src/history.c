/*
 * history.c
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

#include "prof_history.h"

#define MAX_HISTORY 100

static PHistory history;

void _stringify_input(char *inp, int size, char *string);

void
history_init(void)
{
    history = p_history_new(MAX_HISTORY);
}

void
history_append(char *inp)
{
    p_history_append(history, inp);
}

char *
history_previous(char *inp, int *size)
{
    char inp_str[*size + 1];
    _stringify_input(inp, *size, inp_str);

    return p_history_previous(history, inp_str);
}

char *
history_next(char *inp, int *size)
{
    char inp_str[*size + 1];
    _stringify_input(inp, *size, inp_str);

    return p_history_next(history, inp_str);
}

void
_stringify_input(char *inp, int size, char *string)
{
    int i;
    for (i = 0; i < size; i++) {
        string[i] = inp[i];
    }
    string[size] = '\0';
}
