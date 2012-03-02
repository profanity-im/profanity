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

#include <stdlib.h>
#include <string.h>

#define MAX_HISTORY 100

static char *_history[MAX_HISTORY];
static int _size;
static int _pos;

void history_init(void)
{
    int i;
    for (i = 0; i < _size; i++)
        free(_history[i]);
    
    _size = 0;
    _pos = -1;
}

void history_append(char *inp)
{
    if (_size < MAX_HISTORY) {
        _history[_size] = (char*) malloc(strlen(inp) * sizeof(char));
        strcpy(_history[_size], inp);
        _pos = _size;
        _size++;
    }
}

char *history_previous(void)
{
    if (_size == 0 || _pos == -1)
        return NULL;

    return _history[_pos--];
}

char *history_next(void)
{
    if (_size == 0)
        return NULL;
    if (_pos == (_size - 1))
        return NULL;
    if (_pos + 2 >= _size)
        return NULL;
    
    int pos = _pos + 2;
    _pos++;
 
    return _history[pos];
}

