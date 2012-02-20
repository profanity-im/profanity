/* 
 * util.c
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

#include <time.h>
#include <string.h>
#include <ctype.h>

void get_time(char *thetime)
{
    time_t rawtime;
    struct tm *timeinfo;

    time(&rawtime);
    timeinfo = localtime(&rawtime);

    strftime(thetime, 80, "%H:%M", timeinfo);
}

char *trim(char *str)
{
    char *end;

    while (isspace(*str)) 
        str++;

    if (*str == 0)
      return str;

    end = str + strlen(str) - 1;
    while (end > str && isspace(*end)) 
        end--;

    *(end+1) = 0;

    return str;
}

