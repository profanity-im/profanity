/*
 * history.c
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

#include "tools/history.h"

#define MAX_HISTORY 100

static History history;

void _stringify_input(char *inp, int size, char *string);

void
cmd_history_init(void)
{
    history = history_new(MAX_HISTORY);
}

void
cmd_history_append(char *inp)
{
    history_append(history, inp);
}

char *
cmd_history_previous(char *inp, int *size)
{
    char inp_str[*size + 1];
    _stringify_input(inp, *size, inp_str);

    return history_previous(history, inp_str);
}

char *
cmd_history_next(char *inp, int *size)
{
    char inp_str[*size + 1];
    _stringify_input(inp, *size, inp_str);

    return history_next(history, inp_str);
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
