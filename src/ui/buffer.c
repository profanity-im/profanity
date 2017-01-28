/*
 * buffer.c
 *
 * Copyright (C) 2012 - 2017 James Booth <boothj5@gmail.com>
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
 * along with Profanity.  If not, see <https://www.gnu.org/licenses/>.
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
buffer_create(void)
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
}

void
buffer_append(ProfBuff buffer, const char show_char, int pad_indent, GDateTime *time,
    int flags, theme_item_t theme_item, const char *const from, const char *const message, DeliveryReceipt *receipt)
{
    ProfBuffEntry *e = malloc(sizeof(struct prof_buff_entry_t));
    e->show_char = show_char;
    e->pad_indent = pad_indent;
    e->flags = flags;
    e->theme_item = theme_item;
    e->time = g_date_time_ref(time);
    e->from = from ? strdup(from) : NULL;
    e->message = strdup(message);
    e->receipt = receipt;

    if (g_slist_length(buffer->entries) == BUFF_SIZE) {
        _free_entry(buffer->entries->data);
        buffer->entries = g_slist_delete_link(buffer->entries, buffer->entries);
    }

    buffer->entries = g_slist_append(buffer->entries, e);
}

gboolean
buffer_mark_received(ProfBuff buffer, const char *const id)
{
    GSList *entries = buffer->entries;
    while (entries) {
        ProfBuffEntry *entry = entries->data;
        if (entry->receipt && g_strcmp0(entry->receipt->id, id) == 0) {
            if (!entry->receipt->received) {
                entry->receipt->received = TRUE;
                return TRUE;
            }
        }
        entries = g_slist_next(entries);
    }

    return FALSE;
}

ProfBuffEntry*
buffer_get_entry(ProfBuff buffer, int entry)
{
    GSList *node = g_slist_nth(buffer->entries, entry);
    return node->data;
}

ProfBuffEntry*
buffer_get_entry_by_id(ProfBuff buffer, const char *const id)
{
    GSList *entries = buffer->entries;
    while (entries) {
        ProfBuffEntry *entry = entries->data;
        if (entry->receipt && g_strcmp0(entry->receipt->id, id) == 0) {
            return entry;
        }
        entries = g_slist_next(entries);
    }

    return NULL;
}

static void
_free_entry(ProfBuffEntry *entry)
{
    free(entry->message);
    free(entry->from);
    g_date_time_unref(entry->time);
    if (entry->receipt) {
        free(entry->receipt->id);
        free(entry->receipt);
    }
    free(entry);
}
