/*
 * stanza.c
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

#include <stdlib.h>
#include <string.h>

#include <glib.h>
#include <strophe.h>

#include "common.h"
#include "xmpp/connection.h"
#include "xmpp/stanza.h"
#include "xmpp/capabilities.h"

static int _field_compare(FormField *f1, FormField *f2);

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
stanza_create_room_join_presence(xmpp_ctx_t * const ctx,
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
stanza_create_presence(xmpp_ctx_t * const ctx)
{
    xmpp_stanza_t *presence = xmpp_stanza_new(ctx);
    xmpp_stanza_set_name(presence, STANZA_NAME_PRESENCE);

    return presence;
}

xmpp_stanza_t *
stanza_create_software_version_iq(xmpp_ctx_t *ctx, const char * const fulljid)
{
    xmpp_stanza_t *iq = xmpp_stanza_new(ctx);
    xmpp_stanza_set_name(iq, STANZA_NAME_IQ);
    xmpp_stanza_set_type(iq, STANZA_TYPE_GET);
    xmpp_stanza_set_id(iq, "sv");
    xmpp_stanza_set_attribute(iq, "to", fulljid);

    xmpp_stanza_t *query = xmpp_stanza_new(ctx);
    xmpp_stanza_set_name(query, STANZA_NAME_QUERY);
    xmpp_stanza_set_ns(query, STANZA_NS_VERSION);

    xmpp_stanza_add_child(iq, query);
    xmpp_stanza_release(query);

    return iq;
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

xmpp_stanza_t *
stanza_create_disco_info_iq(xmpp_ctx_t *ctx, const char * const id, const char * const to,
    const char * const node)
{
    xmpp_stanza_t *iq = xmpp_stanza_new(ctx);
    xmpp_stanza_set_name(iq, STANZA_NAME_IQ);
    xmpp_stanza_set_type(iq, STANZA_TYPE_GET);
    xmpp_stanza_set_attribute(iq, STANZA_ATTR_TO, to);
    xmpp_stanza_set_id(iq, id);

    xmpp_stanza_t *query = xmpp_stanza_new(ctx);
    xmpp_stanza_set_name(query, STANZA_NAME_QUERY);
    xmpp_stanza_set_ns(query, XMPP_NS_DISCO_INFO);
    if (node != NULL) {
        xmpp_stanza_set_attribute(query, STANZA_ATTR_NODE, node);
    }

    xmpp_stanza_add_child(iq, query);
    xmpp_stanza_release(query);

    return iq;
}

xmpp_stanza_t *
stanza_create_disco_items_iq(xmpp_ctx_t *ctx, const char * const id,
    const char * const jid)
{
    xmpp_stanza_t *iq = xmpp_stanza_new(ctx);
    xmpp_stanza_set_name(iq, STANZA_NAME_IQ);
    xmpp_stanza_set_type(iq, STANZA_TYPE_GET);
    xmpp_stanza_set_attribute(iq, STANZA_ATTR_TO, jid);
    xmpp_stanza_set_id(iq, id);

    xmpp_stanza_t *query = xmpp_stanza_new(ctx);
    xmpp_stanza_set_name(query, STANZA_NAME_QUERY);
    xmpp_stanza_set_ns(query, XMPP_NS_DISCO_ITEMS);

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

char *
stanza_get_status(xmpp_stanza_t *stanza, char *def)
{
    xmpp_stanza_t *status =
        xmpp_stanza_get_child_by_name(stanza, STANZA_NAME_STATUS);

    if (status != NULL) {
        return xmpp_stanza_get_text(status);
    } else {
        return def;
    }
}

char *
stanza_get_show(xmpp_stanza_t *stanza, char *def)
{
    xmpp_stanza_t *show =
        xmpp_stanza_get_child_by_name(stanza, STANZA_NAME_SHOW);

    if (show != NULL) {
        return xmpp_stanza_get_text(show);
    } else {
        return def;
    }
}

gboolean
stanza_is_muc_presence(xmpp_stanza_t * const stanza)
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

    return TRUE;
}

gboolean
stanza_is_muc_self_presence(xmpp_stanza_t * const stanza,
    const char * const self_jid)
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

    // for older server that don't send status 110
    x_children = xmpp_stanza_get_children(x);
    while (x_children != NULL) {
        if (strcmp(xmpp_stanza_get_name(x_children), STANZA_NAME_ITEM) == 0) {
            char *jid = xmpp_stanza_get_attribute(x_children, STANZA_ATTR_JID);
            if (jid != NULL) {
                if (g_str_has_prefix(jid, self_jid)) {
                    return TRUE;
                }
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

int
stanza_get_idle_time(xmpp_stanza_t * const stanza)
{
    xmpp_stanza_t *query = xmpp_stanza_get_child_by_name(stanza, STANZA_NAME_QUERY);

    if (query == NULL) {
        return 0;
    }

    char *ns = xmpp_stanza_get_ns(query);
    if (ns == NULL) {
        return 0;
    }

    if (strcmp(ns, STANZA_NS_LASTACTIVITY) != 0) {
        return 0;
    }

    char *seconds_str = xmpp_stanza_get_attribute(query, STANZA_ATTR_SECONDS);
    if (seconds_str == NULL) {
        return 0;
    }

    int result = atoi(seconds_str);
    if (result < 1) {
        return 0;
    } else {
        return result;
    }
}

gboolean
stanza_contains_caps(xmpp_stanza_t * const stanza)
{
    xmpp_stanza_t *caps = xmpp_stanza_get_child_by_name(stanza, STANZA_NAME_C);

    if (caps == NULL) {
        return FALSE;
    }

    if (strcmp(xmpp_stanza_get_ns(caps), STANZA_NS_CAPS) != 0) {
        return FALSE;
    }

    return TRUE;
}

gboolean
stanza_is_version_request(xmpp_stanza_t * const stanza)
{
    char *type = xmpp_stanza_get_attribute(stanza, STANZA_ATTR_TYPE);

    if (g_strcmp0(type, STANZA_TYPE_GET) != 0) {
        return FALSE;
    }

    xmpp_stanza_t *query = xmpp_stanza_get_child_by_name(stanza, STANZA_NAME_QUERY);

    if (query == NULL) {
        return FALSE;
    }

    char *ns = xmpp_stanza_get_ns(query);

    if (ns == NULL) {
        return FALSE;
    }

    if (strcmp(ns, STANZA_NS_VERSION) != 0) {
        return FALSE;
    }

    return TRUE;
}

gboolean
stanza_is_caps_request(xmpp_stanza_t * const stanza)
{
    char *type = xmpp_stanza_get_attribute(stanza, STANZA_ATTR_TYPE);

    if (g_strcmp0(type, STANZA_TYPE_GET) != 0) {
        return FALSE;
    }

    xmpp_stanza_t *query = xmpp_stanza_get_child_by_name(stanza, STANZA_NAME_QUERY);

    if (query == NULL) {
        return FALSE;
    }

    char *ns = xmpp_stanza_get_ns(query);

    if (ns == NULL) {
        return FALSE;
    }

    if (strcmp(ns, XMPP_NS_DISCO_INFO) != 0) {
        return FALSE;
    }

    return TRUE;
}

char *
stanza_caps_get_hash(xmpp_stanza_t * const stanza)
{
    xmpp_stanza_t *caps = xmpp_stanza_get_child_by_name(stanza, STANZA_NAME_C);

    if (caps == NULL) {
        return NULL;
    }

    if (strcmp(xmpp_stanza_get_ns(caps), STANZA_NS_CAPS) != 0) {
        return NULL;
    }

    char *result = xmpp_stanza_get_attribute(caps, STANZA_ATTR_HASH);

    return result;

}

char *
stanza_get_caps_str(xmpp_stanza_t * const stanza)
{
    xmpp_stanza_t *caps = xmpp_stanza_get_child_by_name(stanza, STANZA_NAME_C);

    if (caps == NULL) {
        return NULL;
    }

    if (strcmp(xmpp_stanza_get_ns(caps), STANZA_NS_CAPS) != 0) {
        return NULL;
    }

    char *node = xmpp_stanza_get_attribute(caps, STANZA_ATTR_NODE);
    char *ver = xmpp_stanza_get_attribute(caps, STANZA_ATTR_VER);

    if ((node == NULL) || (ver == NULL)) {
        return NULL;
    }

    GString *caps_gstr = g_string_new(node);
    g_string_append(caps_gstr, "#");
    g_string_append(caps_gstr, ver);
    char *caps_str = caps_gstr->str;
    g_string_free(caps_gstr, FALSE);

    return  caps_str;
}

DataForm *
stanza_create_form(xmpp_stanza_t * const stanza)
{
    DataForm *result = NULL;
    xmpp_ctx_t *ctx = connection_get_ctx();

    xmpp_stanza_t *child = xmpp_stanza_get_children(stanza);

    if (child != NULL) {
        result = malloc(sizeof(struct data_form_t));
        result->form_type = NULL;
        result->fields = NULL;
    }

    //handle fields
    while (child != NULL) {
        char *var = xmpp_stanza_get_attribute(child, "var");

        // handle FORM_TYPE
        if (g_strcmp0(var, "FORM_TYPE") == 0) {
            xmpp_stanza_t *value = xmpp_stanza_get_child_by_name(child, "value");
            char *value_text = xmpp_stanza_get_text(value);
            result->form_type = strdup(value_text);

        // handle regular fields
        } else {
            FormField *field = malloc(sizeof(struct form_field_t));
            field->var = strdup(var);
            field->values = NULL;
            xmpp_stanza_t *value = xmpp_stanza_get_children(child);

            // handle values
            while (value != NULL) {
                char *text = xmpp_stanza_get_text(value);
                if (text != NULL) {
                    field->values = g_slist_insert_sorted(field->values, strdup(text), (GCompareFunc)octet_compare);
                    xmpp_free(ctx, text);
                }
                value = xmpp_stanza_get_next(value);
            }

            result->fields = g_slist_insert_sorted(result->fields, field, (GCompareFunc)_field_compare);
        }

        child = xmpp_stanza_get_next(child);
    }

    return result;
}

void
stanza_destroy_form(DataForm *form)
{
    if (form != NULL) {
        FREE_SET_NULL(form->form_type);
        if (form->fields != NULL) {
            GSList *curr_field = form->fields;
            while (curr_field != NULL) {
                FormField *field = curr_field->data;
                FREE_SET_NULL(field->var);
                if ((field->values) != NULL) {
                    g_slist_free_full(field->values, free);
                }
            }
        }

        form = NULL;
    }
}

void
stanza_attach_priority(xmpp_ctx_t * const ctx, xmpp_stanza_t * const presence,
    const int pri)
{
    if (pri != 0) {
        xmpp_stanza_t *priority, *value;
        char pri_str[10];

        snprintf(pri_str, sizeof(pri_str), "%d", pri);
        priority = xmpp_stanza_new(ctx);
        value = xmpp_stanza_new(ctx);
        xmpp_stanza_set_name(priority, STANZA_NAME_PRIORITY);
        xmpp_stanza_set_text(value, pri_str);
        xmpp_stanza_add_child(priority, value);
        xmpp_stanza_add_child(presence, priority);
        xmpp_stanza_release(priority);
    }
}

void
stanza_attach_show(xmpp_ctx_t * const ctx, xmpp_stanza_t * const presence,
    const char * const show)
{
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
}

void
stanza_attach_status(xmpp_ctx_t * const ctx, xmpp_stanza_t * const presence,
    const char * const status)
{
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
}

void
stanza_attach_last_activity(xmpp_ctx_t * const ctx,
    xmpp_stanza_t * const presence, const int idle)
{
    if (idle > 0) {
        xmpp_stanza_t *query = xmpp_stanza_new(ctx);
        xmpp_stanza_set_name(query, STANZA_NAME_QUERY);
        xmpp_stanza_set_ns(query, STANZA_NS_LASTACTIVITY);
        char idle_str[10];
        snprintf(idle_str, sizeof(idle_str), "%d", idle);
        xmpp_stanza_set_attribute(query, STANZA_ATTR_SECONDS, idle_str);
        xmpp_stanza_add_child(presence, query);
        xmpp_stanza_release(query);
    }
}

void
stanza_attach_caps(xmpp_ctx_t * const ctx, xmpp_stanza_t * const presence)
{
    xmpp_stanza_t *caps = xmpp_stanza_new(ctx);
    xmpp_stanza_set_name(caps, STANZA_NAME_C);
    xmpp_stanza_set_ns(caps, STANZA_NS_CAPS);
    xmpp_stanza_t *query = caps_create_query_response_stanza(ctx);

    char *sha1 = caps_create_sha1_str(query);
    xmpp_stanza_set_attribute(caps, STANZA_ATTR_HASH, "sha-1");
    xmpp_stanza_set_attribute(caps, STANZA_ATTR_NODE, "http://www.profanity.im");
    xmpp_stanza_set_attribute(caps, STANZA_ATTR_VER, sha1);
    xmpp_stanza_add_child(presence, caps);
    xmpp_stanza_release(caps);
    xmpp_stanza_release(query);
    FREE_SET_NULL(sha1);
}

const char *
stanza_get_presence_string_from_type(resource_presence_t presence_type)
{
    switch(presence_type)
    {
        case RESOURCE_AWAY:
            return STANZA_TEXT_AWAY;
        case RESOURCE_DND:
            return STANZA_TEXT_DND;
        case RESOURCE_CHAT:
            return STANZA_TEXT_CHAT;
        case RESOURCE_XA:
            return STANZA_TEXT_XA;
        default:
            return NULL;
    }
}

static int
_field_compare(FormField *f1, FormField *f2)
{
    return octet_compare((unsigned char *)f1->var, (unsigned char *)f2->var);
}
