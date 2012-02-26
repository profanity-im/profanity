/* 
 * input_buffer.c
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

#define BUFMAX 100

static char *_inp_buf[BUFMAX];
static int _buf_size;
static int _buf_prev;

void inpbuf_init(void)
{
    _buf_size = 0;
    _buf_prev = -1;
}

void inpbuf_append(char *inp)
{
    if (_buf_size < BUFMAX) {
        _inp_buf[_buf_size] = (char*) malloc(strlen(inp) * sizeof(char));
        strcpy(_inp_buf[_buf_size], inp);
        _buf_prev = _buf_size;
        _buf_size++;
    }
}

char *inpbuf_get_previous(void)
{
    if (_buf_size == 0 || _buf_prev == -1)
        return NULL;
    return _inp_buf[_buf_prev--];
}


