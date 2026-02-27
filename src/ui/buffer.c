/*
 * buffer.c
 * vim: expandtab:ts=4:sts=4:sw=4
 *
 * Copyright (C) 2012 - 2019 James Booth <boothj5@gmail.com>
 * Copyright (C) 2019 - 2026 Michael Vetter <jubalh@iodoru.org>
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
#elif HAVE_CURSES_H
#include <curses.h>
#endif

#include "log.h"
#include "ui/window.h"
#include "ui/buffer.h"

#define MAX_BUFFER_SIZE     200
#define STRDUP_OR_NULL(str) ((str) ? strdup(str) : NULL)

struct prof_buff_t
{
    GSList* entries;
    int lines;
};

static void _free_entry(ProfBuffEntry* entry);
static ProfBuffEntry* _create_entry(const char* show_char, int pad_indent, GDateTime* time, int flags, theme_item_t theme_item, const char* const display_from, const char* const from_jid, const char* const message, DeliveryReceipt* receipt, const char* const id, int y_start_pos, int y_end_pos);
static void _buffer_add(ProfBuff buffer, const char* show_char, int pad_indent, GDateTime* time, int flags, theme_item_t theme_item, const char* const display_from, const char* const from_jid, const char* const message, DeliveryReceipt* receipt, const char* const id, int y_start_pos, int y_end_pos, gboolean append);

ProfBuff
buffer_create(void)
{
    ProfBuff new_buff = malloc(sizeof(struct prof_buff_t));
    new_buff->entries = NULL;
    new_buff->lines = 0;
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
buffer_append(ProfBuff buffer, const char* show_char, int pad_indent, GDateTime* time, int flags, theme_item_t theme_item, const char* const display_from, const char* const from_jid, const char* const message, DeliveryReceipt* receipt, const char* const id, int y_start_pos, int y_end_pos)
{
    _buffer_add(buffer, show_char, pad_indent, time, flags, theme_item, display_from, from_jid, message, receipt, id, y_start_pos, y_end_pos, TRUE);
}

void
buffer_prepend(ProfBuff buffer, const char* show_char, int pad_indent, GDateTime* time, int flags, theme_item_t theme_item, const char* const display_from, const char* const from_jid, const char* const message, DeliveryReceipt* receipt, const char* const id, int y_start_pos, int y_end_pos)
{
    _buffer_add(buffer, show_char, pad_indent, time, flags, theme_item, display_from, from_jid, message, receipt, id, y_start_pos, y_end_pos, FALSE);
}

static void
_buffer_add(ProfBuff buffer, const char* show_char, int pad_indent, GDateTime* time, int flags, theme_item_t theme_item, const char* const display_from, const char* const from_jid, const char* const message, DeliveryReceipt* receipt, const char* const id, int y_start_pos, int y_end_pos, gboolean append)
{
    ProfBuffEntry* e = _create_entry(show_char, pad_indent, time, flags, theme_item, display_from, from_jid, message, receipt, id, y_start_pos, y_end_pos);

    buffer->lines += e->_lines;

    while (g_slist_length(buffer->entries) >= MAX_BUFFER_SIZE) {
        GSList* buffer_entry_to_delete = append ? buffer->entries : g_slist_last(buffer->entries);
        ProfBuffEntry* entry_to_delete = (ProfBuffEntry*)buffer_entry_to_delete->data;
        buffer->lines -= entry_to_delete->_lines;
        _free_entry(entry_to_delete);
        buffer->entries = g_slist_delete_link(buffer->entries, buffer_entry_to_delete);
    }

    if (from_jid && y_end_pos == y_start_pos) {
        log_warning("Ncurses Overflow! From: %s, pos: %d, ID: %s, message: %s", from_jid, y_end_pos, id, message);
    }

    buffer->entries = append ? g_slist_append(buffer->entries, e) : g_slist_prepend(buffer->entries, e);
}

void
buffer_remove_entry_by_id(ProfBuff buffer, const char* const id)
{
    GSList* entries = buffer->entries;
    while (entries) {
        ProfBuffEntry* entry = entries->data;
        if (entry->id && (g_strcmp0(entry->id, id) == 0)) {
            buffer->lines -= entry->_lines;
            _free_entry(entry);
            buffer->entries = g_slist_delete_link(buffer->entries, entries);
            break;
        }
        entries = g_slist_next(entries);
    }
}

void
buffer_remove_entry(ProfBuff buffer, int entry)
{
    GSList* node = g_slist_nth(buffer->entries, entry);
    ProfBuffEntry* e = node->data;
    buffer->lines -= e->_lines;
    _free_entry(e);
    buffer->entries = g_slist_delete_link(buffer->entries, node);
}

gboolean
buffer_mark_received(ProfBuff buffer, const char* const id)
{
    GSList* entries = buffer->entries;
    while (entries) {
        ProfBuffEntry* entry = entries->data;
        if (entry->receipt && g_strcmp0(entry->id, id) == 0) {
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
    GSList* node = g_slist_nth(buffer->entries, entry);
    return node->data;
}

ProfBuffEntry*
buffer_get_entry_by_id(ProfBuff buffer, const char* const id)
{
    GSList* entries = buffer->entries;
    while (entries) {
        ProfBuffEntry* entry = entries->data;
        if (g_strcmp0(entry->id, id) == 0) {
            return entry;
        }
        entries = g_slist_next(entries);
    }

    return NULL;
}

static ProfBuffEntry*
_create_entry(const char* show_char, int pad_indent, GDateTime* time, int flags, theme_item_t theme_item, const char* const display_from, const char* const from_jid, const char* const message, DeliveryReceipt* receipt, const char* const id, int y_start_pos, int y_end_pos)
{
    ProfBuffEntry* e = g_new0(ProfBuffEntry, 1);
    e->show_char = STRDUP_OR_NULL(show_char);
    e->pad_indent = pad_indent;
    e->flags = flags;
    e->theme_item = theme_item;
    e->time = g_date_time_ref(time);
    e->display_from = STRDUP_OR_NULL(display_from);
    e->from_jid = STRDUP_OR_NULL(from_jid);
    e->message = STRDUP_OR_NULL(message);
    e->receipt = receipt;
    e->id = STRDUP_OR_NULL(id);
    e->y_start_pos = y_start_pos;
    e->y_end_pos = y_end_pos;
    e->_lines = e->y_end_pos - e->y_start_pos;

    return e;
}

static void
_free_entry(ProfBuffEntry* entry)
{
    free(entry->id);
    free(entry->message);
    free(entry->from_jid);
    free(entry->display_from);
    g_date_time_unref(entry->time);
    free(entry->show_char);
    free(entry->receipt);
    free(entry);
}
