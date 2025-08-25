/*
 * buffer.h
 * vim: expandtab:ts=4:sts=4:sw=4
 *
 * Copyright (C) 2012 - 2019 James Booth <boothj5@gmail.com>
 * Copyright (C) 2019 - 2025 Michael Vetter <jubalh@iodoru.org>
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

#ifndef UI_BUFFER_H
#define UI_BUFFER_H

#include <glib.h>

#include "config.h"
#include "config/theme.h"

typedef struct delivery_receipt_t
{
    gboolean received;
} DeliveryReceipt;

typedef struct prof_buff_entry_t
{
    // pointer because it could be a unicode symbol as well
    gchar* show_char;
    int pad_indent;
    int y_start_pos;
    int y_end_pos;
    int _lines;
    GDateTime* time;
    int flags;
    theme_item_t theme_item;
    // from as it is displayed
    // might be nick, jid..
    char* display_from;
    char* from_jid;
    char* message;
    DeliveryReceipt* receipt;
    // message id, in case we have it
    char* id;
} ProfBuffEntry;

typedef struct prof_buff_t* ProfBuff;

ProfBuff buffer_create();
void buffer_free(ProfBuff buffer);
void buffer_append(ProfBuff buffer, const char* show_char, int pad_indent, GDateTime* time, int flags, theme_item_t theme_item, const char* const display_from, const char* const barejid, const char* const message, DeliveryReceipt* receipt, const char* const id, int y_start_pos, int y_end_pos);
void buffer_prepend(ProfBuff buffer, const char* show_char, int pad_indent, GDateTime* time, int flags, theme_item_t theme_item, const char* const display_from, const char* const barejid, const char* const message, DeliveryReceipt* receipt, const char* const id, int y_start_pos, int y_end_pos);
void buffer_remove_entry_by_id(ProfBuff buffer, const char* const id);
void buffer_remove_entry(ProfBuff buffer, int entry);
int buffer_size(ProfBuff buffer);
ProfBuffEntry* buffer_get_entry(ProfBuff buffer, int entry);
ProfBuffEntry* buffer_get_entry_by_id(ProfBuff buffer, const char* const id);
gboolean buffer_mark_received(ProfBuff buffer, const char* const id);

#endif
