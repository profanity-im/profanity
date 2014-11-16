/*
 * buffer.c
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

#include "config.h"

#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdlib.h>

#include <glib.h>
#ifdef HAVE_NCURSESW_NCURSES_H
#include <ncursesw/ncurses.h>
#elif HAVE_NCURSES_H
#include <ncurses.h>
#endif

#include "ui/window.h"
#include "ui/buffer.h"

#define BUFF_SIZE 1200

struct prof_buff_t {
    GSList *entries;
};

static void _free_entry(ProfBuffEntry *entry);

ProfBuff
buffer_create()
{
    ProfBuff new_buff = malloc(sizeof(struct prof_buff_t));
    new_buff->entries = NULL;
    return new_buff;
}

int
buffer_size(ProfBuff buffer)
{
    return g_slist_length(buffer->entries);
}

void
buffer_free(ProfBuff buffer)
{
    g_slist_free_full(buffer->entries, (GDestroyNotify)_free_entry);
    free(buffer);
    buffer = NULL;
}

void
buffer_push(ProfBuff buffer, const char show_char, GDateTime *time,
    int flags, int attrs, const char * const from, const char * const message)
{
    ProfBuffEntry *e = malloc(sizeof(struct prof_buff_entry_t));
    e->show_char = show_char;
    e->flags = flags;
    e->attrs = attrs;
    e->time = time;
    e->from = strdup(from);
    e->message = strdup(message);

    if (g_slist_length(buffer->entries) == BUFF_SIZE) {
        _free_entry(buffer->entries->data);
        buffer->entries = g_slist_delete_link(buffer->entries, buffer->entries);
    }

    buffer->entries = g_slist_append(buffer->entries, e);
}

ProfBuffEntry*
buffer_yield_entry(ProfBuff buffer, int entry)
{
    GSList *node = g_slist_nth(buffer->entries, entry);
    return node->data;
}

static void
_free_entry(ProfBuffEntry *entry)
{
    free(entry->message);
    free(entry->from);
    g_date_time_unref(entry->time);
    free(entry);
}