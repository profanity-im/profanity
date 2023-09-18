/*
 * jingle.c
 * vim: expandtab:ts=4:sts=4:sw=4
 *
 * Copyright (C) 2023 Michael Vetter <jubalh@iodoru.org>
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
 * @file
 *
 * Jingle Protocol (XEP-0166) Implementation
 */

#include "assert.h"
#include "common.h"
#include "log.h"
#include "ui/ui.h"
#include "xmpp/connection.h"
#include "xmpp/ibb.h"
#include "xmpp/iq.h"
#include "xmpp/jingle.h"
#include "xmpp/stanza.h"

#include <string.h>
#include <glib.h>
#include <strophe.h>
#include <sys/stat.h>
#include <sys/types.h>

// Recommended by XEP-0047 documentation. Soft TODO: maybe pref?
#define IBB_BLOCK_SIZE 4096

// Handlers
static void _handle_session_init(xmpp_stanza_t* const stanza);
static void _handle_session_accept(xmpp_stanza_t* const stanza);
static void _handle_session_terminate(xmpp_stanza_t* const stanza);

// Send XMPP
static void _accept_session(prof_jingle_session_t* session);
static void _terminate_session(prof_jingle_session_t* session, const char* reason);
static void _send_ack(const char* id, const char* target);

// Utils
prof_jingle_creator_t _parse_content_creator(const char* creator_raw);
prof_jingle_senders_t _parse_content_senders(const char* senders_raw);
static const char* _stringify_senders(prof_jingle_senders_t senders);
static const char* _stringify_creator(prof_jingle_creator_t creator);
static const char* _jingle_description_type_to_ns(prof_jingle_description_type_t description_type);
static const char* _jingle_transport_type_to_ns(prof_jingle_transport_type_t transport_type);
static char* _uint_to_str(uint value);
static void* _get_item_by_transport_id(const char* transport_id, gboolean retrieve_content);
static prof_jingle_session_t* _get_session_by_id(const char* session_id);
static prof_jingle_file_info_t* _get_file_info(char* file_path);
static gboolean _string_to_off_t(const char* str, off_t* result);
static char* _off_t_to_string(off_t value);

// XMPP Utils
static char* _get_child_text(xmpp_stanza_t* const stanza, char* child_name);
static void _add_child_with_text(xmpp_stanza_t* parent, const char* child_name, const char* child_text);
static xmpp_stanza_t* _xmpp_jingle_new(const char* action, const char* sid);
static xmpp_stanza_t* _convert_session_to_stanza(prof_jingle_session_t* session, const char* action);

// Cleanup
static void _jingle_session_destroy(prof_jingle_session_t* session);
static void _jingle_content_destroy(prof_jingle_content_t* content);
static void _jingle_description_destroy(prof_jingle_description_t* description);
static void _jingle_file_info_destroy(prof_jingle_file_info_t* file_info);
static void _jingle_transport_destroy(prof_jingle_transport_t* transport);
static void _jingle_transport_candidates_destroy(void** transport_candidates);

GHashTable* jingle_sessions;

void
jingle_init(void)
{
    log_info("Jingle initialising");
    assert(jingle_sessions == NULL);
    jingle_sessions = g_hash_table_new_full(g_str_hash, g_str_equal, free, (GDestroyNotify)_jingle_session_destroy);
}

void
jingle_close(void)
{
    if (jingle_sessions) {
        g_hash_table_destroy(jingle_sessions);
        jingle_sessions = NULL;
    }
}

const prof_jingle_content_t*
get_content_by_transport_id(const char* transport_id)
{
    return (const prof_jingle_content_t*)_get_item_by_transport_id(transport_id, TRUE);
}

static void*
_get_item_by_transport_id(const char* transport_id, gboolean retrieve_content)
{
    GHashTableIter iter;
    gpointer key, value;

    g_hash_table_iter_init(&iter, jingle_sessions);
    while (g_hash_table_iter_next(&iter, &key, &value)) {
        prof_jingle_session_t* session = (prof_jingle_session_t*)value;

        GHashTableIter content_iter;
        gpointer content_key, content_value;
        g_hash_table_iter_init(&content_iter, session->content_table);
        while (g_hash_table_iter_next(&content_iter, &content_key, &content_value)) {
            const prof_jingle_content_t* content = (prof_jingle_content_t*)content_value;

            if (content->transport != NULL && strcmp(content->transport->sid, transport_id) == 0) {
                return retrieve_content ? (void*)content : (void*)session;
            }
        }
    }

    return NULL;
}

void
set_content_state_by_transport_id(const char* transport_id, prof_jingle_state_t state)
{
    prof_jingle_session_t* session = (prof_jingle_session_t*)_get_item_by_transport_id(transport_id, FALSE);

    gboolean all_downloads_finished = TRUE;
    GHashTableIter content_iter;
    gpointer content_key, content_value;
    g_hash_table_iter_init(&content_iter, session->content_table);
    while (g_hash_table_iter_next(&content_iter, &content_key, &content_value)) {
        prof_jingle_content_t* content = (prof_jingle_content_t*)content_value;

        if (content->transport != NULL && strcmp(content->transport->sid, transport_id) == 0) {
            content->state = state;
        }

        if (content->state != PROF_JINGLE_STATE_TRANSFER_FINISHED) {
            all_downloads_finished = FALSE;
        }
    }
    if (all_downloads_finished) {
        _terminate_session(session, "success");
    }
}

gboolean
jingle_accept_session(const char* session_id)
{
    prof_jingle_session_t* session = _get_session_by_id(session_id);
    if (!session) {
        return FALSE;
    }

    _accept_session(session);

    return TRUE;
}

gboolean
jingle_reject_session(const char* session_id)
{
    prof_jingle_session_t* session = _get_session_by_id(session_id);
    if (!session) {
        return FALSE;
    }

    _terminate_session(session, "cancel");

    return TRUE;
}

// TODO: also cleanup transports (IBB etc.)
gboolean
jingle_cancel_session(const char* session_id)
{
    return jingle_reject_session(session_id);
}

const char*
jingle_find_unique_session_by_jid(char* jid)
{
    gboolean found = FALSE;
    GHashTableIter iter;
    gpointer key, value;
    const char* session_id = NULL;

    g_hash_table_iter_init(&iter, jingle_sessions);
    while (g_hash_table_iter_next(&iter, &key, &value)) {
        prof_jingle_session_t* session = (prof_jingle_session_t*)value;

        if (g_str_has_prefix(session->initiator, jid)) {
            if (found) {
                cons_show_error("%s has more than 1 session open with you. Please, use direct session ID.", jid);
                return NULL;
            }
            session_id = key;
            found = TRUE;
        }
    }
    if (!found) {
        cons_show_error("Session with %s not found.", jid);
        return NULL;
    }

    return session_id;
}

void
jingle_send_files(char* recipient_fulljid, GList* files)
{
    cons_show("Sending files to %s", recipient_fulljid);
    xmpp_ctx_t* ctx = connection_get_ctx();
    // open a file (check permissions, existence etc.)

    // setup
    const char* my_jid = connection_get_fulljid();
    // make a Jingle session
    // TODO: make it a function
    prof_jingle_session_t* session = malloc(sizeof(prof_jingle_session_t));
    session->initiator = strdup(my_jid);
    cons_show("Initiator: %s", session->initiator);
    // soft TODO: generate more readable jingle sid
    session->recipient_jid = recipient_fulljid;
    session->jingle_sid = connection_create_stanza_id();
    session->state = PROF_JINGLE_STATE_INITIATED;
    session->content_table = g_hash_table_new_full(g_str_hash, g_str_equal, free, (GDestroyNotify)_jingle_content_destroy);
    g_hash_table_insert(jingle_sessions, strdup(session->jingle_sid), session);

    GList* iter = files;
    while (iter != NULL) {
        char* file = (char*)(iter->data);

        // soft TODO: generate more readable content name
        auto_char char* content_name = connection_create_stanza_id();
        prof_jingle_file_info_t* file_info = _get_file_info(file);

        prof_jingle_description_t* description = malloc(sizeof(prof_jingle_description_t));
        description->type = PROF_JINGLE_DESCRIPTION_TYPE_FILETRANSFER;
        description->description = file_info;

        prof_jingle_transport_t* transport = malloc(sizeof(prof_jingle_transport_t));
        transport->type = PROF_JINGLE_TRANSPORT_TYPE_INBANDBYTESTREAM;
        // soft TODO: generate more readable transport sid
        transport->sid = connection_create_stanza_id();
        transport->blocksize = IBB_BLOCK_SIZE;
        transport->candidates = NULL;

        prof_jingle_content_t* content = malloc(sizeof(prof_jingle_content_t));
        content->name = strdup(content_name);
        content->creator = PROF_JINGLE_CREATOR_INITIATOR;
        content->senders = PROF_JINGLE_SENDERS_INITIATOR;
        content->description = description;
        content->transport = transport;

        g_hash_table_insert(session->content_table, strdup(content_name), content);

        iter = g_list_next(iter);
    }

    xmpp_stanza_t* jingle_stanza = _convert_session_to_stanza(session, "session-initiate");

    xmpp_stanza_t* iq = xmpp_iq_new(ctx, STANZA_TYPE_SET, connection_create_stanza_id());
    xmpp_stanza_set_to(iq, recipient_fulljid);
    xmpp_stanza_add_child_ex(iq, jingle_stanza, 0);
    iq_send_stanza(iq);
    xmpp_stanza_release(iq);
}

// Handlers

/*
 * XEP-0166 IQ stanzas handling.
 * @param stanza Stanza to handle
 * @returns true in case if the stanza was handled
 */
gboolean
handle_jingle_iq(xmpp_stanza_t* const stanza)
{
    xmpp_stanza_t* jingle = xmpp_stanza_get_child_by_name_and_ns(stanza, "jingle", STANZA_NS_JINGLE);
    if (!jingle) {
        return FALSE;
    }

    // Check if the "jingle" element has the action attribute set to "session-initiate"
    const char* action = xmpp_stanza_get_attribute(jingle, "action");

    if (!action) {
        return FALSE;
    }

    // todo: initiator check

    if (strcmp(action, "session-initiate") == 0) {
        _handle_session_init(stanza);
    } else if (strcmp(action, "session-terminate") == 0) {
        _handle_session_terminate(stanza);
    } else if (strcmp(action, "session-info") == 0) {
        // session info
    } else if (strcmp(action, "session-accept") == 0) {
        _handle_session_accept(stanza);
    } else if (strcmp(action, "transport-accept") == 0) {
    } else if (strcmp(action, "transport-info") == 0) {
    } else if (strcmp(action, "transport-reject") == 0) {
    } else if (strcmp(action, "transport-replace") == 0) {
    }
    return TRUE;
}

/*
 * XEP-0353 message handling stub.
 * @param stanza Stanza to handle
 * @returns true in case if it was XEP-0353 stanza
 */
gboolean
handle_jingle_message(xmpp_stanza_t* const stanza)
{
    xmpp_stanza_t* propose = xmpp_stanza_get_child_by_name_and_ns(stanza, STANZA_NAME_PROPOSE, STANZA_NS_JINGLE_MESSAGE);
    if (!propose) {
        return FALSE;
    }

    xmpp_stanza_t* description_stanza = xmpp_stanza_get_child_by_ns(propose, STANZA_NS_JINGLE_RTP);
    if (!description_stanza) {
        return FALSE;
    }

    const char* const from = xmpp_stanza_get_from(stanza);
    cons_show("Ring ring: %s is trying to call you", from);
    cons_alert(NULL);
    return TRUE;
}

static void
_handle_session_init(xmpp_stanza_t* const stanza)
{
    const char* from = xmpp_stanza_get_from(stanza);

    xmpp_stanza_t* jingle = xmpp_stanza_get_child_by_name_and_ns(stanza, "jingle", STANZA_NS_JINGLE);
    const char* sid = xmpp_stanza_get_attribute(jingle, "sid");
    if (!sid) {
        cons_debug("JINGLE: malformed stanza, no jingle sid.");
        return;
    }

    const char* initiator = xmpp_stanza_get_attribute(jingle, "initiator");
    if (!initiator) {
        cons_debug("JINGLE: malformed stanza, no jingle initiator.");
        return;
    }
    // TODO: correct check (ignore resource part if not present in any)
    if (g_strcmp0(initiator, from) != 0) {
        cons_debug("JINGLE: malformed stanza, initiator on opening stanza does not match IQ sender. (Initiator: %s; IQ Sender: %s)", initiator, from);
        return;
    }

    xmpp_stanza_t* content_stanza = xmpp_stanza_get_children(jingle);

    _send_ack(xmpp_stanza_get_id(stanza), from);

    prof_jingle_session_t* session = malloc(sizeof(prof_jingle_session_t));
    session->initiator = strdup(initiator);
    session->recipient_jid = strdup(initiator);
    session->jingle_sid = strdup(sid);
    session->state = PROF_JINGLE_STATE_INITIATED;
    session->content_table = g_hash_table_new_full(g_str_hash, g_str_equal, free, (GDestroyNotify)_jingle_content_destroy);
    g_hash_table_insert(jingle_sessions, strdup(sid), session);

    // TODO: check content creator (file request/file send)

    if (!content_stanza) {
        _terminate_session(session, "cancel");
        cons_debug("JINGLE: malformed stanza, no content.");
        return;
    }

    cons_show("File Offer Received from %s", from);

    while (content_stanza) {
        const char* tag = xmpp_stanza_get_name(content_stanza);
        if (tag == NULL || strcmp(tag, "content") != 0) {
            cons_debug("skipped iteration for %s", tag);
            content_stanza = xmpp_stanza_get_next(content_stanza);
            continue;
        }

        cons_debug("jingle: iterating content");
        xmpp_stanza_t* description_stanza = xmpp_stanza_get_child_by_name(content_stanza, "description");
        if (!description_stanza) {
            cons_show("Jingle: No description, malformed.");
            continue;
        }

        xmpp_stanza_t* transport_stanza = xmpp_stanza_get_child_by_name(content_stanza, "transport");
        if (!transport_stanza) {
            cons_show("Jingle: No transport, malformed.");
            continue;
        }

        const char* transport_ns = xmpp_stanza_get_ns(transport_stanza);
        if (!transport_ns) {
            cons_show("Jingle: malformed, transport don't have NS.");
            content_stanza = xmpp_stanza_get_next(content_stanza);
            continue;
        }

        const char* description_ns = xmpp_stanza_get_ns(description_stanza);
        if (!description_ns) {
            cons_show("Jingle: malformed, description don't have NS.");
            continue;
        }

        if (strcmp(description_ns, STANZA_NS_JINGLE_FT5) != 0) {
            cons_show("Jingle: unsupported content (description) provided (NS: %s).", description_ns);
            continue;
        }

        const char* content_name = xmpp_stanza_get_attribute(content_stanza, "name");
        if (!content_name) {
            cons_show("Jingle: malformed content, no name provided.");
            continue;
        }

        const char* content_creator_raw = xmpp_stanza_get_attribute(content_stanza, "creator");
        prof_jingle_creator_t content_creator = _parse_content_creator(content_creator_raw);
        if (content_creator == PROF_JINGLE_CREATOR_UNKNOWN) {
            cons_show("Jingle: malformed content, invalid creator provided.");
            continue;
        }

        const char* content_senders_raw = xmpp_stanza_get_attribute(content_stanza, "senders");
        prof_jingle_senders_t content_senders = _parse_content_senders(content_senders_raw);

        xmpp_stanza_t* file_stanza = xmpp_stanza_get_child_by_name(description_stanza, "file");
        if (!file_stanza) {
            cons_show("JINGLE: Malformed stanza, no file data in the file transfer description.");
            content_stanza = xmpp_stanza_get_next(content_stanza);
            continue;
        }

        off_t file_size;
        const char* size_raw = _get_child_text(file_stanza, "size");
        if (!_string_to_off_t(size_raw, &file_size)) {
            log_error("JINGLE: Malformed stanza, unable to parse the file size (%s)", size_raw);
            continue;
        }
        prof_jingle_file_info_t* file_info = malloc(sizeof(prof_jingle_file_info_t));
        file_info->type = _get_child_text(file_stanza, "media-type");
        file_info->date = _get_child_text(file_stanza, "date");
        file_info->name = _get_child_text(file_stanza, "name");
        file_info->size = file_size;
        file_info->hash = _get_child_text(file_stanza, "hash");
        file_info->location = NULL;
        // TODO: make a function to display this data
        cons_show("    File name: %s\n    Date: %s\n    File type: %s\n    Size: %llu\n    Hash: %s", file_info->name, file_info->date, file_info->type, file_info->size, file_info->hash);

        // save file data in struct
        prof_jingle_description_t* description = malloc(sizeof(prof_jingle_description_t));
        description->type = PROF_JINGLE_DESCRIPTION_TYPE_FILETRANSFER;
        description->description = file_info;

        if (strcmp(transport_ns, STANZA_NS_JINGLE_TRANSPORTS_IBB) == 0) {
            log_debug("Transport is supported");
        } else {
            cons_show_error("Jingle: unsupported transport was offered (wrong NS: %s).", transport_ns);
            content_stanza = xmpp_stanza_get_next(content_stanza);
            continue; // cleanup?
        }

        const char* transport_sid = xmpp_stanza_get_attribute(transport_stanza, "sid");                   // check empty
        const char* transport_block_size_raw = xmpp_stanza_get_attribute(transport_stanza, "block-size"); // check empty
        // if not null (if candidates are present, it can be null)
        uint transport_block_size = atoi(transport_block_size_raw);

        log_debug("Transport SID: %s\nBlock Size: %s\nBlock size converted: %u", transport_sid, transport_block_size_raw, transport_block_size);

        prof_jingle_transport_t* transport = malloc(sizeof(prof_jingle_transport_t));
        transport->type = PROF_JINGLE_TRANSPORT_TYPE_INBANDBYTESTREAM;
        transport->sid = strdup(transport_sid);
        transport->blocksize = transport_block_size;
        transport->candidates = NULL;

        prof_jingle_content_t* content = malloc(sizeof(prof_jingle_content_t));
        content->name = strdup(content_name);
        content->creator = content_creator;
        content->senders = content_senders;
        content->description = description;
        content->transport = transport;

        g_hash_table_insert(session->content_table, strdup(content_name), content);

        content_stanza = xmpp_stanza_get_next(content_stanza);
    }
    // todo: terminate a session if the content table is empty
    cons_show("Do you want to receive it? Use `/jingle session accept %s` to accept it or `/jingle session reject %s` to decline transfer.", session->jingle_sid, session->jingle_sid);
}

static void
_handle_session_accept(xmpp_stanza_t* const stanza)
{
    // const char* from = xmpp_stanza_get_from(stanza);
    //  todo: check from && session recipient

    xmpp_stanza_t* jingle_stanza = xmpp_stanza_get_child_by_name_and_ns(stanza, "jingle", STANZA_NS_JINGLE);
    const char* sid = xmpp_stanza_get_attribute(jingle_stanza, "sid");
    if (!sid) {
        log_warning("[Jingle] Can't accept the session, no SID provided.");
        return;
    }

    prof_jingle_session_t* session = g_hash_table_lookup(jingle_sessions, sid);
    if (!session) {
        log_warning("[Jingle] Can't accept the session, no SID provided.");
        return;
    }

    GHashTableIter content_iter;
    gpointer content_key, content_value;
    g_hash_table_iter_init(&content_iter, session->content_table);

    while (g_hash_table_iter_next(&content_iter, &content_key, &content_value)) {
        prof_jingle_content_t* content = (prof_jingle_content_t*)content_value;
        if (!content || !content->transport || !content->description) {
            continue;
        }

        if (content->transport->type == PROF_JINGLE_TRANSPORT_TYPE_INBANDBYTESTREAM) {
            ibb_send_file(session->recipient_jid, content);
        }
    }
}

static void
_handle_session_terminate(xmpp_stanza_t* const stanza)
{
    // SECURITY TODO: check if sid session's recipient == from
    const char* from = xmpp_stanza_get_from(stanza);
    const char* id = xmpp_stanza_get_id(stanza);
    _send_ack(id, from);

    xmpp_stanza_t* jingle = xmpp_stanza_get_child_by_name_and_ns(stanza, "jingle", STANZA_NS_JINGLE);
    const char* sid = xmpp_stanza_get_attribute(jingle, "sid"); // filter?

    if (!sid || !g_hash_table_contains(jingle_sessions, sid)) {
        // todo: proper (not-found)
        return;
    }

    g_hash_table_remove(jingle_sessions, sid);
}

// XMPP Utils

/**
 * Sends an IQ stanza to accept a Jingle session.
 *
 * @param session The Jingle session to accept.
 */
static void
_accept_session(prof_jingle_session_t* session)
{
    xmpp_ctx_t* ctx = connection_get_ctx();

    const char* my_jid = connection_get_fulljid();

    auto_char char* id = connection_create_stanza_id();
    xmpp_stanza_t* iq_stanza = xmpp_iq_new(ctx, STANZA_TYPE_SET, id);
    xmpp_stanza_set_attribute(iq_stanza, "to", session->initiator);

    xmpp_stanza_t* jingle_stanza = _convert_session_to_stanza(session, "session-accept");
    xmpp_stanza_set_attribute(jingle_stanza, "responder", my_jid);

    xmpp_stanza_add_child(iq_stanza, jingle_stanza);

    iq_send_stanza(iq_stanza);

    session->state = PROF_JINGLE_STATE_ACCEPTED;

    xmpp_stanza_release(jingle_stanza);
    xmpp_stanza_release(iq_stanza);
}

static void
_terminate_session(prof_jingle_session_t* session, const char* reason)
{
    xmpp_ctx_t* ctx = connection_get_ctx();

    auto_char char* id = connection_create_stanza_id();
    xmpp_stanza_t* iq_stanza = xmpp_iq_new(ctx, STANZA_TYPE_SET, id);
    xmpp_stanza_set_attribute(iq_stanza, "to", session->initiator);

    xmpp_stanza_t* jingle_stanza = _xmpp_jingle_new("session-terminate", session->jingle_sid);
    xmpp_stanza_add_child(iq_stanza, jingle_stanza);

    xmpp_stanza_t* reason_stanza = xmpp_stanza_new(ctx);
    xmpp_stanza_set_name(reason_stanza, "reason");
    xmpp_stanza_add_child(jingle_stanza, reason_stanza);

    xmpp_stanza_t* reason_name_stanza = xmpp_stanza_new(ctx);
    xmpp_stanza_set_name(reason_name_stanza, reason);
    xmpp_stanza_add_child(reason_stanza, reason_name_stanza);

    iq_send_stanza(iq_stanza);

    // cleanup
    xmpp_stanza_release(iq_stanza);
    xmpp_stanza_release(jingle_stanza);
    xmpp_stanza_release(reason_stanza);
    xmpp_stanza_release(reason_name_stanza);

    g_hash_table_remove(jingle_sessions, session->jingle_sid);
}

/**
 * Sends an acknowledgment IQ stanza.
 *
 * @param id The identifier of the original stanza to acknowledge.
 * @param target The target JID to send the acknowledgment to.
 */
static void
_send_ack(const char* id, const char* target)
{
    xmpp_ctx_t* ctx = connection_get_ctx();
    xmpp_stanza_t* iq = xmpp_iq_new(ctx, STANZA_TYPE_RESULT, id);
    xmpp_stanza_set_to(iq, target);
    iq_send_stanza(iq);
    xmpp_stanza_release(iq);
}

static xmpp_stanza_t*
_convert_session_to_stanza(prof_jingle_session_t* session, const char* action)
{
    xmpp_ctx_t* ctx = connection_get_ctx();
    xmpp_stanza_t* jingle_stanza = _xmpp_jingle_new(action, session->jingle_sid);
    xmpp_stanza_set_attribute(jingle_stanza, "initiator", session->initiator);

    GHashTableIter content_iter;
    gpointer content_key, content_value;
    g_hash_table_iter_init(&content_iter, session->content_table);

    while (g_hash_table_iter_next(&content_iter, &content_key, &content_value)) {
        prof_jingle_content_t* content = (prof_jingle_content_t*)content_value;
        auto_char char* block_size = _uint_to_str(content->transport->blocksize);

        xmpp_stanza_t* content_stanza = xmpp_stanza_new(ctx);
        xmpp_stanza_set_name(content_stanza, "content");
        xmpp_stanza_set_attribute(content_stanza, "creator", _stringify_creator(content->creator));
        xmpp_stanza_set_attribute(content_stanza, "senders", _stringify_senders(content->senders));
        xmpp_stanza_set_attribute(content_stanza, "name", content->name);

        xmpp_stanza_t* description_stanza = xmpp_stanza_new(ctx);
        xmpp_stanza_set_name(description_stanza, "description");
        xmpp_stanza_set_ns(description_stanza, _jingle_description_type_to_ns(content->description->type));

        xmpp_stanza_t* description_data_stanza = xmpp_stanza_new(ctx);
        if (content->description->type == PROF_JINGLE_DESCRIPTION_TYPE_FILETRANSFER) {
            prof_jingle_file_info_t* file_info = (prof_jingle_file_info_t*)content->description->description;
            xmpp_stanza_set_name(description_data_stanza, "file");

            _add_child_with_text(description_data_stanza, "name", file_info->name);
            _add_child_with_text(description_data_stanza, "media-type", file_info->type);
            _add_child_with_text(description_data_stanza, "date", file_info->date);
            _add_child_with_text(description_data_stanza, "size", _off_t_to_string(file_info->size));
            _add_child_with_text(description_data_stanza, "hash", file_info->hash);
        }

        xmpp_stanza_t* transport_stanza = xmpp_stanza_new(ctx);
        xmpp_stanza_set_name(transport_stanza, "transport");
        xmpp_stanza_set_ns(transport_stanza, _jingle_transport_type_to_ns(content->transport->type));
        xmpp_stanza_set_attribute(transport_stanza, "block-size", block_size);
        xmpp_stanza_set_attribute(transport_stanza, "sid", content->transport->sid);

        xmpp_stanza_add_child_ex(description_stanza, description_data_stanza, 0);
        xmpp_stanza_add_child_ex(content_stanza, description_stanza, 0);
        xmpp_stanza_add_child_ex(content_stanza, transport_stanza, 0);
        xmpp_stanza_add_child_ex(jingle_stanza, content_stanza, 0);
    }

    return jingle_stanza;
}

// Utils

static prof_jingle_file_info_t*
_get_file_info(char* file_path)
{
    // mime-type skipped for now since it would probably require dependencies
    // name, date, size
    prof_jingle_file_info_t* file_info = malloc(sizeof(prof_jingle_file_info_t));

    const gchar* file_name = g_basename(file_path);
    file_info->name = strdup(file_name);
    file_info->size = 0;
    file_info->date = NULL;
    file_info->hash = NULL;
    file_info->type = NULL;
    file_info->location = strdup(file_path); // TODO: real path (?)

    struct stat file_info_stat;
    if (stat(file_path, &file_info_stat) == 0) {
        GDateTime* gdt = g_date_time_new_from_unix_utc(file_info_stat.st_mtime);
        if (gdt) {
            auto_gchar gchar* date_time_str = g_date_time_format(gdt, "%Y-%m-%dT%H:%M:%S");
            file_info->date = strdup(date_time_str);
            g_date_time_unref(gdt);
        }
        file_info->size = file_info_stat.st_size;
    }

    cons_show("File info for %s\n  Name: %s\n  Size: %llu\n  Date: %s", file_path, file_info->name, file_info->size, file_info->date);

    return file_info;
}

static prof_jingle_session_t*
_get_session_by_id(const char* session_id)
{
    prof_jingle_session_t* session = g_hash_table_lookup(jingle_sessions, session_id);
    if (!session) {
        cons_show("Jingle: unable to find a session.");
        return NULL;
    }
    return session;
}

static char*
_get_child_text(xmpp_stanza_t* const stanza, char* child_name)
{
    xmpp_stanza_t* child = xmpp_stanza_get_child_by_name(stanza, child_name);
    if (!child) {
        return NULL;
    }
    return xmpp_stanza_get_text(child);
}

static void
_add_child_with_text(xmpp_stanza_t* parent, const char* child_name, const char* child_text)
{
    if (!child_text) {
        return;
    }
    xmpp_ctx_t* ctx = connection_get_ctx();
    xmpp_stanza_t* child_stanza = xmpp_stanza_new(ctx);
    xmpp_stanza_set_name(child_stanza, child_name);

    xmpp_stanza_t* txt = xmpp_stanza_new(ctx);
    xmpp_stanza_set_text(txt, child_text);
    xmpp_stanza_add_child_ex(child_stanza, txt, 0);

    xmpp_stanza_add_child_ex(parent, child_stanza, 0);
}

static xmpp_stanza_t*
_xmpp_jingle_new(const char* action, const char* sid)
{
    xmpp_ctx_t* ctx = connection_get_ctx();
    xmpp_stanza_t* jingle = xmpp_stanza_new(ctx);
    xmpp_stanza_set_name(jingle, "jingle");
    xmpp_stanza_set_ns(jingle, STANZA_NS_JINGLE);
    xmpp_stanza_set_attribute(jingle, "sid", sid);
    xmpp_stanza_set_attribute(jingle, "action", action);

    return jingle;
}

prof_jingle_creator_t
_parse_content_creator(const char* creator_raw)
{
    if (!creator_raw) {
        cons_show("Jingle: malformed content, no creator provided.");
        return PROF_JINGLE_CREATOR_UNKNOWN;
    }

    if (strcmp(creator_raw, "initiator") == 0) {
        return PROF_JINGLE_CREATOR_INITIATOR;
    } else if (strcmp(creator_raw, "responder") == 0) {
        return PROF_JINGLE_CREATOR_RESPONDER;
    } else {
        return PROF_JINGLE_CREATOR_UNKNOWN;
    }
}

prof_jingle_senders_t
_parse_content_senders(const char* senders_raw)
{
    if (!senders_raw) {
        cons_show("Jingle: malformed content, no senders provided.");
        return PROF_JINGLE_SENDERS_UNKNOWN;
    }

    if (strcmp(senders_raw, "both") == 0) {
        return PROF_JINGLE_SENDERS_BOTH;
    } else if (strcmp(senders_raw, "initiator") == 0) {
        return PROF_JINGLE_SENDERS_INITIATOR;
    } else if (strcmp(senders_raw, "responder") == 0) {
        return PROF_JINGLE_SENDERS_RESPONDER;
    } else if (strcmp(senders_raw, "none") == 0) {
        return PROF_JINGLE_SENDERS_NONE;
    } else {
        cons_show("Jingle: malformed content, invalid senders provided.");
        return PROF_JINGLE_SENDERS_UNKNOWN;
    }
}

static const char*
_stringify_senders(prof_jingle_senders_t senders)
{
    switch (senders) {
    case PROF_JINGLE_SENDERS_BOTH:
        return "both";
    case PROF_JINGLE_SENDERS_INITIATOR:
        return "initiator";
    case PROF_JINGLE_SENDERS_RESPONDER:
        return "responder";
    case PROF_JINGLE_SENDERS_NONE:
        return "none";
    case PROF_JINGLE_SENDERS_UNKNOWN:
    default:
        return "unknown";
    }
}

static const char*
_stringify_creator(prof_jingle_creator_t creator)
{
    switch (creator) {
    case PROF_JINGLE_CREATOR_INITIATOR:
        return "initiator";
    case PROF_JINGLE_CREATOR_RESPONDER:
        return "responder";
    case PROF_JINGLE_CREATOR_UNKNOWN:
    default:
        return "unknown";
    }
}

static char*
_uint_to_str(uint value)
{
    char* str = NULL;
    int num_chars = snprintf(NULL, 0, "%u", value);

    if (num_chars <= 0) {
        return NULL;
    }

    str = (char*)malloc(num_chars + 1);
    snprintf(str, num_chars + 1, "%u", value);

    return str;
}

static char*
_off_t_to_string(off_t value)
{
    auto_gchar gchar* tmp = g_strdup_printf("%llu", (long long)value);
    if (!tmp) {
        log_warning("[Jingle] Couldn't translate off_t to string.");
        return NULL;
    }
    return strdup(tmp);
}

static gboolean
_string_to_off_t(const char* str, off_t* result)
{
    errno = 0;

    char* endptr = NULL;
    off_t offset = strtoll(str, &endptr, 10);

    if ((errno == ERANGE && (offset == LLONG_MAX || offset == LLONG_MIN)) || (errno != 0 && offset == 0)) {
        log_warning("[Jingle] Couldn't translate string \"%s\" to off_t.", str);
        return FALSE;
    }

    if (endptr == str) {
        log_warning("[Jingle] Couldn't translate string \"%s\" to off_t. No digits were found.", str);
        return FALSE;
    }

    *result = offset;

    return TRUE;
}

static const char*
_jingle_transport_type_to_ns(prof_jingle_transport_type_t transport_type)
{
    switch (transport_type) {
    case PROF_JINGLE_TRANSPORT_TYPE_INBANDBYTESTREAM:
        return STANZA_NS_JINGLE_TRANSPORTS_IBB;
    case PROF_JINGLE_TRANSPORT_TYPE_SOCKS5:
        return STANZA_NS_JINGLE_TRANSPORTS_S5B;
    default:
        return NULL;
    }
}

static const char*
_jingle_description_type_to_ns(prof_jingle_description_type_t description_type)
{
    switch (description_type) {
    case PROF_JINGLE_DESCRIPTION_TYPE_FILETRANSFER:
        return STANZA_NS_JINGLE_FT5;
    case PROF_JINGLE_DESCRIPTION_TYPE_RTP:
        return STANZA_NS_JINGLE_RTP;
    default:
        return NULL;
    }
}

// Cleanup functions

// TODO
static void
_jingle_transport_candidates_destroy(void** transport_candidates)
{
    if (!transport_candidates) {
        return;
    }
    return;
}

static void
_jingle_session_destroy(prof_jingle_session_t* session)
{

    if (!session) {
        return;
    }

    free(session->jingle_sid);
    free(session->recipient_jid);
    free(session->initiator);
    g_hash_table_destroy(session->content_table);
    free(session);
}

static void
_jingle_content_destroy(prof_jingle_content_t* content)
{
    if (!content) {
        return;
    }

    free(content->name);
    _jingle_transport_destroy(content->transport);
    _jingle_description_destroy(content->description);
    free(content);
}

static void
_jingle_transport_destroy(prof_jingle_transport_t* transport)
{
    if (!transport) {
        return;
    }

    if (transport->destroy_function) {
        transport->destroy_function(transport->sid);
    }

    _jingle_transport_candidates_destroy(transport->candidates);
    free(transport->sid);
    free(transport);
}

static void
_jingle_description_destroy(prof_jingle_description_t* description)
{
    if (!description) {
        return;
    }

    if (description->type == PROF_JINGLE_DESCRIPTION_TYPE_FILETRANSFER) {
        _jingle_file_info_destroy((prof_jingle_file_info_t*)description->description);
    }
}

static void
_jingle_file_info_destroy(prof_jingle_file_info_t* file_info)
{
    if (!file_info) {
        return;
    }

    free(file_info->location);
    free(file_info->name);
    free(file_info->type);
    free(file_info->date);
    free(file_info->hash);
    free(file_info);
}