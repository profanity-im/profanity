/*
 * test_rooms.c
 *
 * Copyright (C) 2015 - 2018 James Booth <boothj5@gmail.com>
 * Copyright (C) 2026 - 2026 Michael Vetter <jubalh@iodoru.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
#include <glib.h>
#include "prof_cmocka.h"
#include <stdlib.h>
#include <string.h>

#include <stabber.h>

#include "proftest.h"

void
rooms_query(void** state)
{
    stbbr_for_query("http://jabber.org/protocol/disco#items",
                    "<iq id='*' type='result' to='stabber@localhost/profanity' from='conference.localhost'>"
                    "<query xmlns='http://jabber.org/protocol/disco#items'>"
                    "<item jid='chatroom@conference.localhost' name='A chat room'/>"
                    "<item jid='hangout@conference.localhost' name='Another chat room'/>"
                    "</query>"
                    "</iq>");

    prof_connect();

    prof_input("/rooms service conference.localhost");

    assert_true(prof_output_regex("Room list request sent: conference.localhost[\\s\\S]*chatroom@conference.localhost[\\s\\S]*hangout@conference.localhost"));

    assert_true(stbbr_last_received(
        "<iq id='*' to='conference.localhost' type='get'>"
        "<query xmlns='http://jabber.org/protocol/disco#items'/>"
        "</iq>"));
}
