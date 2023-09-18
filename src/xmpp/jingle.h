/*
 * jingle.h
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

#ifndef XMPP_JINGLE_H
#define XMPP_JINGLE_H

void jingle_init(void);
void jingle_close(void);

gboolean handle_jingle_iq(xmpp_stanza_t* const stanza);
gboolean handle_jingle_message(xmpp_stanza_t* const stanza);

gboolean jingle_accept_session(const char* session_id);
gboolean jingle_reject_session(const char* session_id);
gboolean jingle_cancel_session(const char* session_id);
void jingle_send_files(char* recipient_fulljid, GList* files);

const char* jingle_find_unique_session_by_jid(char* jid);

typedef void (*ProfJingleTransportDestroyCallback)(void* transport_sid);

typedef enum {
    PROF_JINGLE_STATE_INITIATED,
    PROF_JINGLE_STATE_ACCEPTED,
    PROF_JINGLE_STATE_TRANSFER_IN_PROGRESS,
    PROF_JINGLE_STATE_TRANSFER_FINISHED
} prof_jingle_state_t;

typedef enum {
    PROF_JINGLE_TRANSPORT_TYPE_INBANDBYTESTREAM,
    PROF_JINGLE_TRANSPORT_TYPE_SOCKS5
} prof_jingle_transport_type_t;

typedef enum {
    PROF_JINGLE_DESCRIPTION_TYPE_FILETRANSFER,
    PROF_JINGLE_DESCRIPTION_TYPE_RTP
} prof_jingle_description_type_t;

typedef enum {
    PROF_JINGLE_CREATOR_INITIATOR,
    PROF_JINGLE_CREATOR_RESPONDER,
    PROF_JINGLE_CREATOR_UNKNOWN
} prof_jingle_creator_t;

typedef enum {
    PROF_JINGLE_SENDERS_BOTH,
    PROF_JINGLE_SENDERS_INITIATOR,
    PROF_JINGLE_SENDERS_RESPONDER,
    PROF_JINGLE_SENDERS_NONE,
    PROF_JINGLE_SENDERS_UNKNOWN
} prof_jingle_senders_t;

// Jingle Session Metadata
typedef struct
{
    char* jingle_sid;
    char* initiator;
    char* recipient_jid;
    prof_jingle_state_t state;
    GHashTable* content_table;
} prof_jingle_session_t;

typedef struct
{
    prof_jingle_description_type_t type;
    void* description;
} prof_jingle_description_t;

typedef struct
{
    char* sid;
    prof_jingle_transport_type_t type;
    void** candidates;
    uint blocksize;
    ProfJingleTransportDestroyCallback destroy_function;
} prof_jingle_transport_t;

typedef struct
{
    char* name;
    prof_jingle_creator_t creator;
    prof_jingle_senders_t senders;
    prof_jingle_description_t* description;
    prof_jingle_transport_t* transport;
    prof_jingle_state_t state;
} prof_jingle_content_t;

// XEP-0234: Jingle File Transfer (to be moved in a separate file)

// File Info
typedef struct
{
    char* name;
    char* type;
    char* date;
    char* hash;
    char* location;
    off_t size;
} prof_jingle_file_info_t;

const prof_jingle_content_t* get_content_by_transport_id(const char* transport_id);
void set_content_state_by_transport_id(const char* transport_id, prof_jingle_state_t state);

#endif