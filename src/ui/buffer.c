/*
 * buffer.c
 *
 * Copyright (C) 2012 - 2016 James Booth <boothj5@gmail.com>
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

ProfBuffDate*
buffer_date_new(GDateTime *timestamp, gboolean colour)
{
    ProfBuffDate *date = malloc(sizeof(ProfBuffDate));
    date->timestamp = timestamp == NULL ? g_date_time_new_now_local() : g_date_time_ref(timestamp);
    date->colour_date = colour;

    return date;
}

ProfBuffDate*
buffer_date_new_now(void)
{
    ProfBuffDate *date = malloc(sizeof(ProfBuffDate));
    date->timestamp = g_date_time_new_now_local();
    date->colour_date = TRUE;

    return date;
}

ProfBuffReceipt*
buffer_receipt_new(char *id)
{
    ProfBuffReceipt *receipt = malloc(sizeof(ProfBuffReceipt));
    receipt->id = strdup(id);
    receipt->received = FALSE;

    return receipt;
}

ProfBuffFrom*
buffer_from_new(prof_buff_from_type_t type, const char *const from)
{
    ProfBuffFrom *result = malloc(sizeof(ProfBuffFrom));
    result->type = type;
    result->from = strdup(from);

    return result;
}

ProfBuffEntry*
buffer_entry_create(
    theme_item_t theme_item,
    ProfBuffDate *date,
    const char show_char,
    ProfBuffFrom *from,
    const char *const message,
    int pad_indent,
    gboolean newline,
    ProfBuffReceipt *receipt)
{
    ProfBuffEntry *entry = malloc(sizeof(struct prof_buff_entry_t));
    entry->show_char = show_char;
    entry->pad_indent = pad_indent;
    entry->newline = newline;
    entry->theme_item = theme_item;
    entry->date = date;
    entry->from = from;
    entry->receipt = receipt;
    entry->message = strdup(message);

    return entry;
}

void
buffer_append(ProfWin *window, ProfBuffEntry *entry)
{
    if (g_slist_length(window->layout->entries) == BUFF_SIZE) {
        buffer_free_entry(window->layout->entries->data);
        window->layout->entries = g_slist_delete_link(window->layout->entries, window->layout->entries);
    }

    window->layout->entries = g_slist_append(window->layout->entries, entry);

    win_print_entry(window, entry);
    inp_nonblocking(TRUE);
}

ProfBuffEntry*
buffer_get_entry_by_id(GSList *entries, const char *const id)
{
    GSList *curr = entries;
    while (curr) {
        ProfBuffEntry *entry = curr->data;
        if (entry->receipt && g_strcmp0(entry->receipt->id, id) == 0) {
            return entry;
        }
        curr = g_slist_next(curr);
    }

    return NULL;
}

void
buffer_free_entry(ProfBuffEntry *entry)
{
    free(entry->message);
    if (entry->date) {
        if (entry->date->timestamp) {
            g_date_time_unref(entry->date->timestamp);
        }
        free(entry->date);
    }
    if (entry->from) {
        free(entry->from->from);
        free(entry->from);
    }
    if (entry->receipt) {
        free(entry->receipt->id);
        free(entry->receipt);
    }
    free(entry);
}
