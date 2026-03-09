/*
 * chat_state.h
 * vim: expandtab:ts=4:sts=4:sw=4
 *
 * Copyright (C) 2012 - 2019 James Booth <boothj5@gmail.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later WITH OpenSSL-exception
 */

#ifndef XMPP_CHAT_STATE_H
#define XMPP_CHAT_STATE_H

#include <glib.h>

typedef enum {
    CHAT_STATE_ACTIVE,
    CHAT_STATE_COMPOSING,
    CHAT_STATE_PAUSED,
    CHAT_STATE_INACTIVE,
    CHAT_STATE_GONE
} chat_state_type_t;

typedef struct prof_chat_state_t
{
    chat_state_type_t type;
    GTimer* timer;
} ChatState;

ChatState* chat_state_new(void);
void chat_state_free(ChatState* state);

void chat_state_idle(void);
void chat_state_activity(void);

void chat_state_handle_idle(const char* const barejid, ChatState* state);
void chat_state_handle_typing(const char* const barejid, ChatState* state);
void chat_state_active(ChatState* state);
void chat_state_gone(const char* const barejid, ChatState* state);

#endif
