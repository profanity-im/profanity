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
#include <stdlib.h>

void get_time(char *thetime)
{
    time_t rawtime;
    struct tm *timeinfo;

    time(&rawtime);
    timeinfo = localtime(&rawtime);

    strftime(thetime, 80, "%H:%M", timeinfo);
}

char * str_replace (const char *string, const char *substr, 
    const char *replacement) 
{
    char *tok = NULL;
    char *newstr = NULL;
    char *oldstr = NULL;
    char *head = NULL;
 
    if (string == NULL)
        return NULL;

    if ( substr == NULL || 
         replacement == NULL || 
         (strcmp(substr, "") == 0)) 
        return strdup (string);

    newstr = strdup (string);
    head = newstr;

    while ( (tok = strstr ( head, substr ))) {
        oldstr = newstr;
        newstr = malloc ( strlen ( oldstr ) - strlen ( substr ) + 
            strlen ( replacement ) + 1 );
        
        if ( newstr == NULL ) { 
            free (oldstr);
            return NULL;
        }
        
        memcpy ( newstr, oldstr, tok - oldstr );
        memcpy ( newstr + (tok - oldstr), replacement, strlen ( replacement ) );
        memcpy ( newstr + (tok - oldstr) + strlen( replacement ), 
            tok + strlen ( substr ), 
            strlen ( oldstr ) - strlen ( substr ) - ( tok - oldstr ) );
        memset ( newstr + strlen ( oldstr ) - strlen ( substr ) + 
            strlen ( replacement ) , 0, 1 );
        
        head = newstr + (tok - oldstr) + strlen( replacement );
        free (oldstr);
    }
  
    return newstr;
}

int str_contains(char str[], int size, char ch)
{
    int i;
    for (i = 0; i < size; i++) {
        if (str[i] == ch)
            return 1;
    }

    return 0;
}
