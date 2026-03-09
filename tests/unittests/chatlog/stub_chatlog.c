/*
 * mock_log.c
 *
 * Copyright (C) 2012 - 2019 James Booth <boothj5@gmail.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later WITH OpenSSL-exception
 */

#include <glib.h>
#include "prof_cmocka.h"

#include <xmpp/xmpp.h>

void
chatlog_init(void)
{
}

void
chat_log_msg_out(const char* const barejid, const char* const msg, const char* const resource)
{
}
void
chat_log_otr_msg_out(const char* const barejid, const char* const msg, const char* const resource)
{
}
void
chat_log_pgp_msg_out(const char* const barejid, const char* const msg, const char* const resource)
{
}
void
chat_log_omemo_msg_out(const char* const barejid, const char* const msg, const char* const resource)
{
}

void
chat_log_msg_in(ProfMessage* message)
{
}
void
chat_log_otr_msg_in(ProfMessage* message)
{
}
void
chat_log_pgp_msg_in(ProfMessage* message)
{
}
void
chat_log_omemo_msg_in(ProfMessage* message)
{
}

void
groupchat_log_init(void)
{
}
void
groupchat_log_msg_in(const gchar* const room, const gchar* const nick, const gchar* const msg)
{
}
void
groupchat_log_msg_out(const gchar* const room, const gchar* const msg)
{
}
void
groupchat_log_omemo_msg_in(const gchar* const room, const gchar* const nick, const gchar* const msg)
{
}
void
groupchat_log_omemo_msg_out(const gchar* const room, const gchar* const msg)
{
}
