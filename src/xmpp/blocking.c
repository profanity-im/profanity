/*
 * blocking.c
 * vim: expandtab:ts=4:sts=4:sw=4
 *
 * Copyright (C) 2012 - 2019 James Booth <boothj5@gmail.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later WITH OpenSSL-exception
 */

#include "config.h"

#include <stdlib.h>
#include <string.h>

#include <strophe.h>

#include <glib.h>

#include "log.h"
#include "common.h"
#include "ui/ui.h"
#include "xmpp/session.h"
#include "xmpp/connection.h"
#include "xmpp/stanza.h"
#include "xmpp/iq.h"

static int _blocklist_result_handler(xmpp_stanza_t* const stanza, void* const userdata);
static int _block_add_result_handler(xmpp_stanza_t* const stanza, void* const userdata);
static int _block_remove_result_handler(xmpp_stanza_t* const stanza, void* const userdata);

static GList* blocked;
static Autocomplete blocked_ac;

static void
_blocking_shutdown(void)
{
    if (blocked) {
        g_list_free_full(blocked, free);
    }
    autocomplete_free(blocked_ac);
}

void
blocking_request(void)
{
    prof_add_shutdown_routine(_blocking_shutdown);
    if (blocked) {
        g_list_free_full(blocked, free);
        blocked = NULL;
    }

    if (blocked_ac) {
        autocomplete_free(blocked_ac);
    }
    blocked_ac = autocomplete_new();

    auto_char char* id = connection_create_stanza_id();
    iq_id_handler_add(id, _blocklist_result_handler, NULL, NULL);

    xmpp_ctx_t* ctx = connection_get_ctx();
    xmpp_stanza_t* iq = stanza_create_blocked_list_request(ctx);
    xmpp_stanza_set_id(iq, id);

    iq_send_stanza(iq);
    xmpp_stanza_release(iq);
}

GList*
blocked_list(void)
{
    return blocked;
}

char*
blocked_ac_find(const char* const search_str, gboolean previous, void* context)
{
    return autocomplete_complete(blocked_ac, search_str, TRUE, previous);
}

void
blocked_ac_reset(void)
{
    if (blocked_ac) {
        autocomplete_reset(blocked_ac);
    }
}

gboolean
blocked_add(char* jid, blocked_report reportkind, const char* const message)
{
    GList* found = g_list_find_custom(blocked, jid, (GCompareFunc)g_strcmp0);
    if (found) {
        return FALSE;
    }

    xmpp_ctx_t* ctx = connection_get_ctx();

    auto_char char* id = connection_create_stanza_id();
    xmpp_stanza_t* iq = xmpp_iq_new(ctx, STANZA_TYPE_SET, id);

    xmpp_stanza_t* block = xmpp_stanza_new(ctx);
    xmpp_stanza_set_name(block, STANZA_NAME_BLOCK);
    xmpp_stanza_set_ns(block, STANZA_NS_BLOCKING);

    xmpp_stanza_t* item = xmpp_stanza_new(ctx);
    xmpp_stanza_set_name(item, STANZA_NAME_ITEM);
    xmpp_stanza_set_attribute(item, STANZA_ATTR_JID, jid);

    if (reportkind != BLOCKED_NO_REPORT) {
        xmpp_stanza_t* report = xmpp_stanza_new(ctx);
        xmpp_stanza_set_name(report, STANZA_NAME_REPORT);
        xmpp_stanza_set_ns(report, STANZA_NS_REPORTING);
        if (reportkind == BLOCKED_REPORT_ABUSE) {
            xmpp_stanza_set_attribute(report, STANZA_ATTR_REASON, STANZA_REPORTING_ABUSE);
        } else {
            xmpp_stanza_set_attribute(report, STANZA_ATTR_REASON, STANZA_REPORTING_SPAM);
        }

        if (message) {
            xmpp_stanza_t* text = xmpp_stanza_new(ctx);
            xmpp_stanza_set_name(text, STANZA_NAME_TEXT);

            xmpp_stanza_t* txt = xmpp_stanza_new(ctx);
            xmpp_stanza_set_text(txt, message);

            xmpp_stanza_add_child(text, txt);
            xmpp_stanza_add_child(report, text);
            xmpp_stanza_release(txt);
        }

        xmpp_stanza_add_child(item, report);
        xmpp_stanza_release(report);
    }

    xmpp_stanza_add_child(block, item);
    xmpp_stanza_release(item);

    xmpp_stanza_add_child(iq, block);
    xmpp_stanza_release(block);

    iq_id_handler_add(id, _block_add_result_handler, free, strdup(jid));

    iq_send_stanza(iq);
    xmpp_stanza_release(iq);

    return TRUE;
}

gboolean
blocked_remove(char* jid)
{
    GList* found = g_list_find_custom(blocked, jid, (GCompareFunc)g_strcmp0);
    if (!found) {
        return FALSE;
    }

    xmpp_ctx_t* ctx = connection_get_ctx();

    auto_char char* id = connection_create_stanza_id();
    xmpp_stanza_t* iq = xmpp_iq_new(ctx, STANZA_TYPE_SET, id);

    xmpp_stanza_t* block = xmpp_stanza_new(ctx);
    xmpp_stanza_set_name(block, STANZA_NAME_UNBLOCK);
    xmpp_stanza_set_ns(block, STANZA_NS_BLOCKING);

    xmpp_stanza_t* item = xmpp_stanza_new(ctx);
    xmpp_stanza_set_name(item, STANZA_NAME_ITEM);
    xmpp_stanza_set_attribute(item, STANZA_ATTR_JID, jid);

    xmpp_stanza_add_child(block, item);
    xmpp_stanza_release(item);

    xmpp_stanza_add_child(iq, block);
    xmpp_stanza_release(block);

    iq_id_handler_add(id, _block_remove_result_handler, free, strdup(jid));

    iq_send_stanza(iq);
    xmpp_stanza_release(iq);

    return TRUE;
}

int
blocked_set_handler(xmpp_stanza_t* stanza)
{
    xmpp_stanza_t* block = xmpp_stanza_get_child_by_name(stanza, STANZA_NAME_BLOCK);
    if (block) {
        xmpp_stanza_t* child = xmpp_stanza_get_children(block);
        while (child) {
            if (g_strcmp0(xmpp_stanza_get_name(child), STANZA_NAME_ITEM) == 0) {
                const char* jid = xmpp_stanza_get_attribute(child, STANZA_ATTR_JID);
                if (jid) {
                    blocked = g_list_append(blocked, strdup(jid));
                    autocomplete_add(blocked_ac, jid);
                }
            }

            child = xmpp_stanza_get_next(child);
        }
    }

    xmpp_stanza_t* unblock = xmpp_stanza_get_child_by_name(stanza, STANZA_NAME_UNBLOCK);
    if (unblock) {
        xmpp_stanza_t* child = xmpp_stanza_get_children(unblock);
        if (!child) {
            g_list_free_full(blocked, free);
            blocked = NULL;
            autocomplete_clear(blocked_ac);
        } else {
            while (child) {
                if (g_strcmp0(xmpp_stanza_get_name(child), STANZA_NAME_ITEM) == 0) {
                    const char* jid = xmpp_stanza_get_attribute(child, STANZA_ATTR_JID);
                    if (jid) {
                        GList* found = g_list_find_custom(blocked, jid, (GCompareFunc)g_strcmp0);
                        if (found) {
                            blocked = g_list_remove_link(blocked, found);
                            g_list_free_full(found, free);
                            autocomplete_remove(blocked_ac, jid);
                        }
                    }
                }

                child = xmpp_stanza_get_next(child);
            }
        }
    }

    return 1;
}

static int
_block_add_result_handler(xmpp_stanza_t* const stanza, void* const userdata)
{
    char* jid = (char*)userdata;

    const char* type = xmpp_stanza_get_type(stanza);
    if (!type) {
        log_info("Block response received for %s with no type attribute.", jid);
        return 0;
    }

    if (g_strcmp0(type, "result") != 0) {
        log_info("Block response received for %s with unrecognised type attribute.", jid);
        return 0;
    }

    cons_show("User %s successfully blocked.", jid);

    return 0;
}

static int
_block_remove_result_handler(xmpp_stanza_t* const stanza, void* const userdata)
{
    char* jid = (char*)userdata;

    const char* type = xmpp_stanza_get_type(stanza);
    if (!type) {
        log_info("Unblock response received for %s with no type attribute.", jid);
        return 0;
    }

    if (g_strcmp0(type, "result") != 0) {
        log_info("Unblock response received for %s with unrecognised type attribute.", jid);
        return 0;
    }

    cons_show("User %s successfully unblocked.", jid);

    return 0;
}

static int
_blocklist_result_handler(xmpp_stanza_t* const stanza, void* const userdata)
{
    log_debug("Blocked list result handler fired.");

    const char* type = xmpp_stanza_get_type(stanza);
    if (g_strcmp0(type, "result") != 0) {
        log_info("Received blocklist without result type");
        return 0;
    }

    xmpp_stanza_t* blocklist = xmpp_stanza_get_child_by_name(stanza, STANZA_NAME_BLOCKLIST);
    if (!blocklist) {
        log_info("Received blocklist without blocklist element");
        return 0;
    }

    if (blocked) {
        g_list_free_full(blocked, free);
        blocked = NULL;
    }

    xmpp_stanza_t* items = xmpp_stanza_get_children(blocklist);
    if (!items) {
        log_debug("No blocked users.");
        return 0;
    }

    xmpp_stanza_t* curr = items;
    while (curr) {
        const char* name = xmpp_stanza_get_name(curr);
        if (g_strcmp0(name, "item") == 0) {
            const char* jid = xmpp_stanza_get_attribute(curr, STANZA_ATTR_JID);
            if (jid) {
                blocked = g_list_append(blocked, strdup(jid));
                autocomplete_add(blocked_ac, jid);
            }
        }
        curr = xmpp_stanza_get_next(curr);
    }

    return 0;
}
