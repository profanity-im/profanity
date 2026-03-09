/*
 * common.c
 * vim: expandtab:ts=4:sts=4:sw=4
 *
 * Copyright (C) 2019 - 2026 Michael Vetter <jubalh@iodoru.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later WITH OpenSSL-exception
 */

#include "config.h"

#include "config/tlscerts.h"
#include "ui/ui.h"
#include "xmpp/chat_session.h"
#include "xmpp/roster_list.h"
#include "xmpp/muc.h"
#include "xmpp/xmpp.h"
#include "xmpp/vcard_funcs.h"
#include "database.h"
#include "tools/bookmark_ignore.h"

#ifdef HAVE_LIBGPGME
#include "pgp/gpg.h"
#endif

#ifdef HAVE_OMEMO
#include "omemo/omemo.h"
#endif

static gint _success_connections_counter = 0;

void
ev_disconnect_cleanup(void)
{
    ui_disconnected();
    session_disconnect();
    roster_destroy();
    iq_autoping_timer_cancel();
    muc_invites_clear();
    muc_confserver_clear();
    chat_sessions_clear();
    tlscerts_clear_current();
#ifdef HAVE_LIBGPGME
    p_gpg_on_disconnect();
#endif
#ifdef HAVE_OMEMO
    omemo_on_disconnect();
#endif
    log_database_close();
    bookmark_ignore_on_disconnect();
    vcard_user_free();
}

gboolean
ev_was_connected_already(void)
{
    if (_success_connections_counter > 0)
        return TRUE;
    else
        return FALSE;
}

gboolean
ev_is_first_connect(void)
{
    if (_success_connections_counter == 1)
        return TRUE;
    else
        return FALSE;
}

void
ev_inc_connection_counter(void)
{
    _success_connections_counter++;
}

void
ev_reset_connection_counter(void)
{
    _success_connections_counter = 0;
}
