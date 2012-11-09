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

#include "common.h"
#include "stanza.h"

xmpp_stanza_t *
stanza_create_chat_state(xmpp_ctx_t *ctx, const char * const recipient,
    const char * const state)
{
    xmpp_stanza_t *msg, *chat_state;

    msg = xmpp_stanza_new(ctx);
    xmpp_stanza_set_name(msg, STANZA_NAME_MESSAGE);
    xmpp_stanza_set_type(msg, STANZA_TYPE_CHAT);
    xmpp_stanza_set_attribute(msg, STANZA_ATTR_TO, recipient);

    chat_state = xmpp_stanza_new(ctx);
    xmpp_stanza_set_name(chat_state, state);
    xmpp_stanza_set_ns(chat_state, STANZA_NS_CHATSTATES);
    xmpp_stanza_add_child(msg, chat_state);

    return msg;
}

xmpp_stanza_t *
stanza_create_message(xmpp_ctx_t *ctx, const char * const recipient,
    const char * const type, const char * const message,
    const char * const state)
{
    char *encoded_xml = encode_xml(message);

    xmpp_stanza_t *msg, *body, *text;

    msg = xmpp_stanza_new(ctx);
    xmpp_stanza_set_name(msg, STANZA_NAME_MESSAGE);
    xmpp_stanza_set_type(msg, type);
    xmpp_stanza_set_attribute(msg, STANZA_ATTR_TO, recipient);

    body = xmpp_stanza_new(ctx);
    xmpp_stanza_set_name(body, STANZA_NAME_BODY);

    text = xmpp_stanza_new(ctx);
    xmpp_stanza_set_text(text, encoded_xml);
    xmpp_stanza_add_child(body, text);
    xmpp_stanza_add_child(msg, body);

    if (state != NULL) {
        xmpp_stanza_t *chat_state = xmpp_stanza_new(ctx);
        xmpp_stanza_set_name(chat_state, state);
        xmpp_stanza_set_ns(chat_state, STANZA_NS_CHATSTATES);
        xmpp_stanza_add_child(msg, chat_state);
    }

    g_free(encoded_xml);

    return msg;
}

xmpp_stanza_t *
stanza_create_room_presence(xmpp_ctx_t *ctx, const char * const room,
    const char * const nick)
{
    GString *to = g_string_new(room);
    g_string_append(to, "/");
    g_string_append(to, nick);

    xmpp_stanza_t *presence = xmpp_stanza_new(ctx);
    xmpp_stanza_set_name(presence, STANZA_NAME_PRESENCE);
    xmpp_stanza_set_attribute(presence, STANZA_ATTR_TO, to->str);

    xmpp_stanza_t *x = xmpp_stanza_new(ctx);
    xmpp_stanza_set_name(x, STANZA_NAME_X);
    xmpp_stanza_set_ns(x, STANZA_NS_MUC);

    xmpp_stanza_add_child(presence, x);

    return presence;
}

