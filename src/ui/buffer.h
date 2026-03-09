/*
 * buffer.h
 * vim: expandtab:ts=4:sts=4:sw=4
 *
 * Copyright (C) 2012 - 2019 James Booth <boothj5@gmail.com>
 * Copyright (C) 2019 - 2026 Michael Vetter <jubalh@iodoru.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later WITH OpenSSL-exception
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
