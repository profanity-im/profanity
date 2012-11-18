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

#include <string.h>

#include <glib.h>
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
stanza_create_room_join_presence(xmpp_ctx_t *ctx,
    const char * const full_room_jid)
{
    xmpp_stanza_t *presence = xmpp_stanza_new(ctx);
    xmpp_stanza_set_name(presence, STANZA_NAME_PRESENCE);
    xmpp_stanza_set_attribute(presence, STANZA_ATTR_TO, full_room_jid);

    xmpp_stanza_t *x = xmpp_stanza_new(ctx);
    xmpp_stanza_set_name(x, STANZA_NAME_X);
    xmpp_stanza_set_ns(x, STANZA_NS_MUC);

    xmpp_stanza_add_child(presence, x);

    return presence;
}

xmpp_stanza_t *
stanza_create_room_newnick_presence(xmpp_ctx_t *ctx,
    const char * const full_room_jid)
{
    xmpp_stanza_t *presence = xmpp_stanza_new(ctx);
    xmpp_stanza_set_name(presence, STANZA_NAME_PRESENCE);
    xmpp_stanza_set_attribute(presence, STANZA_ATTR_TO, full_room_jid);

    return presence;
}

xmpp_stanza_t *
stanza_create_room_leave_presence(xmpp_ctx_t *ctx, const char * const room,
    const char * const nick)
{
    GString *full_jid = g_string_new(room);
    g_string_append(full_jid, "/");
    g_string_append(full_jid, nick);

    xmpp_stanza_t *presence = xmpp_stanza_new(ctx);
    xmpp_stanza_set_name(presence, STANZA_NAME_PRESENCE);
    xmpp_stanza_set_type(presence, STANZA_TYPE_UNAVAILABLE);
    xmpp_stanza_set_attribute(presence, STANZA_ATTR_TO, full_jid->str);

    g_string_free(full_jid, TRUE);

    return presence;
}

xmpp_stanza_t *
stanza_create_presence(xmpp_ctx_t *ctx, const char * const show,
    const char * const status)
{
    xmpp_stanza_t *presence = xmpp_stanza_new(ctx);
    xmpp_stanza_set_name(presence, STANZA_NAME_PRESENCE);

    if (show != NULL) {
        xmpp_stanza_t *show_stanza = xmpp_stanza_new(ctx);
        xmpp_stanza_set_name(show_stanza, STANZA_NAME_SHOW);
        xmpp_stanza_t *text = xmpp_stanza_new(ctx);
        xmpp_stanza_set_text(text, show);
        xmpp_stanza_add_child(show_stanza, text);
        xmpp_stanza_add_child(presence, show_stanza);
        xmpp_stanza_release(text);
        xmpp_stanza_release(show_stanza);
    }

    if (status != NULL) {
        xmpp_stanza_t *status_stanza = xmpp_stanza_new(ctx);
        xmpp_stanza_set_name(status_stanza, STANZA_NAME_STATUS);
        xmpp_stanza_t *text = xmpp_stanza_new(ctx);
        xmpp_stanza_set_text(text, status);
        xmpp_stanza_add_child(status_stanza, text);
        xmpp_stanza_add_child(presence, status_stanza);
        xmpp_stanza_release(text);
        xmpp_stanza_release(status_stanza);
    }

    return presence;
}

xmpp_stanza_t *
stanza_create_roster_iq(xmpp_ctx_t *ctx)
{
    xmpp_stanza_t *iq = xmpp_stanza_new(ctx);
    xmpp_stanza_set_name(iq, STANZA_NAME_IQ);
    xmpp_stanza_set_type(iq, STANZA_TYPE_GET);
    xmpp_stanza_set_id(iq, "roster");

    xmpp_stanza_t *query = xmpp_stanza_new(ctx);
    xmpp_stanza_set_name(query, STANZA_NAME_QUERY);
    xmpp_stanza_set_ns(query, XMPP_NS_ROSTER);

    xmpp_stanza_add_child(iq, query);
    xmpp_stanza_release(query);

    return iq;
}

gboolean
stanza_contains_chat_state(xmpp_stanza_t *stanza)
{
    return ((xmpp_stanza_get_child_by_name(stanza, STANZA_NAME_ACTIVE) != NULL) ||
            (xmpp_stanza_get_child_by_name(stanza, STANZA_NAME_COMPOSING) != NULL) ||
            (xmpp_stanza_get_child_by_name(stanza, STANZA_NAME_PAUSED) != NULL) ||
            (xmpp_stanza_get_child_by_name(stanza, STANZA_NAME_GONE) != NULL) ||
            (xmpp_stanza_get_child_by_name(stanza, STANZA_NAME_INACTIVE) != NULL));
}

xmpp_stanza_t *
stanza_create_ping_iq(xmpp_ctx_t *ctx)
{
    xmpp_stanza_t *iq = xmpp_stanza_new(ctx);
    xmpp_stanza_set_name(iq, STANZA_NAME_IQ);
    xmpp_stanza_set_type(iq, STANZA_TYPE_GET);
    xmpp_stanza_set_id(iq, "c2s1");

    xmpp_stanza_t *ping = xmpp_stanza_new(ctx);
    xmpp_stanza_set_name(ping, STANZA_NAME_PING);

    xmpp_stanza_set_ns(ping, STANZA_NS_PING);

    xmpp_stanza_add_child(iq, ping);
    xmpp_stanza_release(ping);

    return iq;
}

gboolean
stanza_get_delay(xmpp_stanza_t * const stanza, GTimeVal *tv_stamp)
{
    // first check for XEP-0203 delayed delivery
    xmpp_stanza_t *delay = xmpp_stanza_get_child_by_name(stanza, STANZA_NAME_DELAY);
    if (delay != NULL) {
        char *xmlns = xmpp_stanza_get_attribute(delay, STANZA_ATTR_XMLNS);
        if ((xmlns != NULL) && (strcmp(xmlns, "urn:xmpp:delay") == 0)) {
            char *stamp = xmpp_stanza_get_attribute(delay, STANZA_ATTR_STAMP);
            if ((stamp != NULL) && (g_time_val_from_iso8601(stamp, tv_stamp))) {
                return TRUE;
            }
        }
    }

    // otherwise check for XEP-0091 legacy delayed delivery
    // stanp format : CCYYMMDDThh:mm:ss
    xmpp_stanza_t *x = xmpp_stanza_get_child_by_name(stanza, STANZA_NAME_X);
    if (x != NULL) {
        char *xmlns = xmpp_stanza_get_attribute(x, STANZA_ATTR_XMLNS);
        if ((xmlns != NULL) && (strcmp(xmlns, "jabber:x:delay") == 0)) {
            char *stamp = xmpp_stanza_get_attribute(x, STANZA_ATTR_STAMP);
            if ((stamp != NULL) && (g_time_val_from_iso8601(stamp, tv_stamp))) {
                return TRUE;
            }
        }
    }

    return FALSE;
}

gboolean
stanza_is_muc_self_presence(xmpp_stanza_t * const stanza)
{
    if (stanza == NULL) {
        return FALSE;
    }
    if (strcmp(xmpp_stanza_get_name(stanza), STANZA_NAME_PRESENCE) != 0) {
        return FALSE;
    }

    xmpp_stanza_t *x = xmpp_stanza_get_child_by_name(stanza, STANZA_NAME_X);

    if (x == NULL) {
        return FALSE;
    }

    char *ns = xmpp_stanza_get_ns(x);
    if (ns == NULL) {
        return FALSE;
    }
    if (strcmp(ns, STANZA_NS_MUC_USER) != 0) {
        return FALSE;
    }

    xmpp_stanza_t *x_children = xmpp_stanza_get_children(x);
    if (x_children == NULL) {
        return FALSE;
    }

    while (x_children != NULL) {
        if (strcmp(xmpp_stanza_get_name(x_children), STANZA_NAME_STATUS) == 0) {
            char *code = xmpp_stanza_get_attribute(x_children, STANZA_ATTR_CODE);
            if (strcmp(code, "110") == 0) {
                return TRUE;
            }
        }
        x_children = xmpp_stanza_get_next(x_children);
    }

    return FALSE;
}

gboolean
stanza_is_room_nick_change(xmpp_stanza_t * const stanza)
{
    if (stanza == NULL) {
        return FALSE;
    }
    if (strcmp(xmpp_stanza_get_name(stanza), STANZA_NAME_PRESENCE) != 0) {
        return FALSE;
    }

    xmpp_stanza_t *x = xmpp_stanza_get_child_by_name(stanza, STANZA_NAME_X);

    if (x == NULL) {
        return FALSE;
    }

    char *ns = xmpp_stanza_get_ns(x);
    if (ns == NULL) {
        return FALSE;
    }
    if (strcmp(ns, STANZA_NS_MUC_USER) != 0) {
        return FALSE;
    }

    xmpp_stanza_t *x_children = xmpp_stanza_get_children(x);
    if (x_children == NULL) {
        return FALSE;
    }

    while (x_children != NULL) {
        if (strcmp(xmpp_stanza_get_name(x_children), STANZA_NAME_STATUS) == 0) {
            char *code = xmpp_stanza_get_attribute(x_children, STANZA_ATTR_CODE);
            if (strcmp(code, "303") == 0) {
                return TRUE;
            }
        }
        x_children = xmpp_stanza_get_next(x_children);
    }

    return FALSE;

}

char *
stanza_get_new_nick(xmpp_stanza_t * const stanza)
{
    if (!stanza_is_room_nick_change(stanza)) {
        return NULL;
    } else {
        xmpp_stanza_t *x = xmpp_stanza_get_child_by_name(stanza, STANZA_NAME_X);
        xmpp_stanza_t *x_children = xmpp_stanza_get_children(x);

        while (x_children != NULL) {
            if (strcmp(xmpp_stanza_get_name(x_children), STANZA_NAME_ITEM) == 0) {
                char *nick = xmpp_stanza_get_attribute(x_children, STANZA_ATTR_NICK);
                if (nick != NULL) {
                    return strdup(nick);
                }
            }
            x_children = xmpp_stanza_get_next(x_children);
        }

        return NULL;
    }
}
