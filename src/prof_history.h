/*
 * prof_history.h
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

#ifndef PROF_HISTORY_H
#define PROF_HISTORY_H

typedef struct p_history_t  *PHistory;

PHistory p_history_new(unsigned int size);
char * p_history_previous(PHistory history, char *item);
char * p_history_next(PHistory history, char *item);
void p_history_append(PHistory history, char *item);

#endif
