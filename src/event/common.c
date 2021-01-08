/*
 * common.c
 * vim: expandtab:ts=4:sts=4:sw=4
 *
 * Copyright (C) 2019 - 2021 Michael Vetter <jubalh@iodoru.org>
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
 */

#include "config/tlscerts.h"
#include "ui/ui.h"
#include "xmpp/chat_session.h"
#include "xmpp/roster_list.h"
#include "xmpp/muc.h"
#include "xmpp/xmpp.h"
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
