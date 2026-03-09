/*
 * chat_session.h
 * vim: expandtab:ts=4:sts=4:sw=4
 *
 * Copyright (C) 2012 - 2019 James Booth <boothj5@gmail.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later WITH OpenSSL-exception
 */

#ifndef XMPP_CHAT_SESSION_H
#define XMPP_CHAT_SESSION_H

#include <glib.h>

typedef struct chat_session_t
{
    char* barejid;
    char* resource;
    gboolean resource_override;
    gboolean send_states;

} ChatSession;

void chat_sessions_init(void);
void chat_sessions_clear(void);

void chat_session_resource_override(const char* const barejid, const char* const resource);
ChatSession* chat_session_get(const char* const barejid);

void chat_session_recipient_active(const char* const barejid, const char* const resource, gboolean send_states);
void chat_session_recipient_typing(const char* const barejid, const char* const resource);
void chat_session_recipient_paused(const char* const barejid, const char* const resource);
void chat_session_recipient_gone(const char* const barejid, const char* const resource);
void chat_session_recipient_inactive(const char* const barejid, const char* const resource);
char* chat_session_get_jid(const char* const barejid);
const char* chat_session_get_state(const char* const barejid);

void chat_session_remove(const char* const barejid);

#endif
