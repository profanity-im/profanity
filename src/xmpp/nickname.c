/*
 * nickname.c
 * vim: expandtab:ts=4:sts=4:sw=4
 *
 * Copyright (C) 2023 Bohdan Horbesko <bodqhrohro@gmail.com>
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

#include "xmpp/message.h"
#include "xmpp/roster_list.h"
#include "xmpp/stanza.h"
#include "ui/ui.h"

static int _pep_nick_handler(xmpp_stanza_t* const stanza, void* const userdata);

void
nickname_pep_subscribe(void)
{
    message_pubsub_event_handler_add(STANZA_NS_NICK, _pep_nick_handler, NULL, NULL);
}

static int
_pep_nick_handler(xmpp_stanza_t* const stanza, void* const userdata)
{
    const char* from = xmpp_stanza_get_attribute(stanza, STANZA_ATTR_FROM);
    if (from) {
        xmpp_stanza_t* event = xmpp_stanza_get_child_by_name_and_ns(stanza, STANZA_NAME_EVENT, STANZA_NS_PUBSUB_EVENT);
        if (event) {
            xmpp_stanza_t* items = xmpp_stanza_get_child_by_name(event, STANZA_NAME_ITEMS);
            if (items) {
                xmpp_stanza_t* item = xmpp_stanza_get_child_by_name(items, STANZA_NAME_ITEM);
                if (item) {
                    xmpp_stanza_t* nick = xmpp_stanza_get_child_by_name_and_ns(item, STANZA_NAME_NICK, STANZA_NS_NICK);
                    if (nick) {
                        auto_char char* text = xmpp_stanza_get_text(nick);

                        PContact contact = roster_get_contact(from);
                        if (contact) {
                            roster_change_name(contact, text);
                        }
                    }
                }
            }
        }
    }
    return 1;
}
