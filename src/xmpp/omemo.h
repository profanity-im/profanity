/*
 * omemo.h
 * vim: expandtab:ts=4:sts=4:sw=4
 *
 * Copyright (C) 2019 Paul Fariello <paul@fariello.eu>
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

#include <glib.h>

#include "xmpp/iq.h"

void omemo_devicelist_subscribe(void);
void omemo_devicelist_configure_and_request(void);
void omemo_devicelist_publish(GList* device_list);
void omemo_devicelist_request(const char* const jid);
void omemo_bundle_publish(gboolean first);
void omemo_bundle_request(const char* const jid, uint32_t device_id, ProfIqCallback func, ProfIqFreeCallback free_func, void* userdata);
int omemo_start_device_session_handle_bundle(xmpp_stanza_t* const stanza, void* const userdata);
char* omemo_receive_message(xmpp_stanza_t* const stanza, gboolean* trusted);
