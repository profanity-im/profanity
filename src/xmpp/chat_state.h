/*
 * chat_state.h
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

typedef struct prof_chat_state_t {
    chat_state_type_t type;
    GTimer *timer;
} ChatState;

ChatState* chat_state_new(void);
void chat_state_free(ChatState *state);

void chat_state_idle(void);
void chat_state_activity(void);

void chat_state_handle_idle(const char *const barejid, ChatState *state);
void chat_state_handle_typing(const char *const barejid, ChatState *state);
void chat_state_active(ChatState *state);
void chat_state_gone(const char *const barejid, ChatState *state);

#endif
