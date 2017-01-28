/*
 * iq.h
 *
 * Copyright (C) 2012 - 2017 James Booth <boothj5@gmail.com>
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

#ifndef XMPP_IQ_H
#define XMPP_IQ_H

typedef int(*ProfIdCallback)(xmpp_stanza_t *const stanza, void *const userdata);
typedef void(*ProfIdFreeCallback)(void *userdata);

void iq_handlers_init(void);
void iq_send_stanza(xmpp_stanza_t *const stanza);
void iq_id_handler_add(const char *const id, ProfIdCallback func, ProfIdFreeCallback free_func, void *userdata);
void iq_disco_info_request_onconnect(gchar *jid);
void iq_disco_items_request_onconnect(gchar *jid);
void iq_send_caps_request(const char *const to, const char *const id, const char *const node, const char *const ver);
void iq_send_caps_request_for_jid(const char *const to, const char *const id, const char *const node,
    const char *const ver);
void iq_send_caps_request_legacy(const char *const to, const char *const id, const char *const node,
    const char *const ver);

#endif
