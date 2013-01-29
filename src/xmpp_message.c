/*
 * xmpp_message.c
 *
 * Copyright (C) 2012, 2013 James Booth <boothj5@gmail.com>
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

#include <strophe.h>

#include "chat_session.h"
#include "preferences.h"
#include "xmpp.h"

void
message_send(const char * const msg, const char * const recipient)
{
    xmpp_conn_t * const conn = jabber_get_conn();
    xmpp_ctx_t * const ctx = jabber_get_ctx();
    if (prefs_get_states()) {
        if (!chat_session_exists(recipient)) {
            chat_session_start(recipient, TRUE);
        }
    }

    xmpp_stanza_t *message;
    if (prefs_get_states() && chat_session_get_recipient_supports(recipient)) {
        chat_session_set_active(recipient);
        message = stanza_create_message(ctx, recipient, STANZA_TYPE_CHAT,
            msg, STANZA_NAME_ACTIVE);
    } else {
        message = stanza_create_message(ctx, recipient, STANZA_TYPE_CHAT,
            msg, NULL);
    }

    xmpp_send(conn, message);
    xmpp_stanza_release(message);
}
