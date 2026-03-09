/*
 * session.h
 * vim: expandtab:ts=4:sts=4:sw=4
 *
 * Copyright (C) 2012 - 2019 James Booth <boothj5@gmail.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later WITH OpenSSL-exception
 */

#ifndef XMPP_SESSION_H
#define XMPP_SESSION_H

#include <glib.h>

void session_login_success(gboolean secured);
void session_login_failed(void);
void session_lost_connection(void);
void session_autoping_fail(void);

void session_init_activity(void);
void session_check_autoaway(void);

void session_reconnect(gchar* altdomain, unsigned short altport);

#endif
