/*
 * stanza.c
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

#include <strophe.h>

xmpp_stanza_t *
stanza_create_chat_state(xmpp_ctx_t *ctx, const char * const recipient,
    const char * const state)
{
    xmpp_stanza_t *message, *chat_state;

    message = xmpp_stanza_new(ctx);
    xmpp_stanza_set_name(message, "message");
    xmpp_stanza_set_type(message, "chat");
    xmpp_stanza_set_attribute(message, "to", recipient);

    chat_state = xmpp_stanza_new(ctx);
    xmpp_stanza_set_name(chat_state, state);
    xmpp_stanza_set_ns(chat_state, "http://jabber.org/protocol/chatstates");
    xmpp_stanza_add_child(message, chat_state);

    return message;
}

