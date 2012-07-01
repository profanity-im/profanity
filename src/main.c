/* 
 * main.c
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

#include <stdio.h>
#include <string.h>

#include "profanity.h"

int main(int argc, char **argv)
{   
    int disable_tls = 0;

    // more than one argument
    if (argc > 2) {
        printf("Usage: profanity [-notls]\n");
        return 1;

    // argument is not -notls
    } else if (argc == 2) {
        char *arg1 = argv[1];
        if (strcmp(arg1, "-notls") != 0) {
            printf("Usage: profanity [-notls]\n");
            return 1;
        } else {
            disable_tls = 1;
        }
    }

    profanity_init(disable_tls);
    profanity_run();

    return 0;
}
