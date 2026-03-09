/*
 * test_carbons.c
 *
 * Copyright (C) 2015 - 2017 James Booth <boothj5@gmail.com>
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
send_enable_carbons(void** state)
{
    prof_connect();

    prof_input("/carbons on");

    assert_true(stbbr_received(
        "<iq id='*' type='set'><enable xmlns='urn:xmpp:carbons:2'/></iq>"));
}

void
connect_with_carbons_enabled(void** state)
{
    prof_input("/carbons on");

    prof_connect();
    assert_true(stbbr_received(
        "<iq id='*' type='set'><enable xmlns='urn:xmpp:carbons:2'/></iq>"));
}

void
send_disable_carbons(void** state)
{
    prof_input("/carbons on");

    prof_connect();
    assert_true(stbbr_received(
        "<iq id='*' type='set'><enable xmlns='urn:xmpp:carbons:2'/></iq>"));

    prof_input("/carbons off");
    assert_true(stbbr_received(
        "<iq id='*' type='set'><disable xmlns='urn:xmpp:carbons:2'/></iq>"));
}

void
receive_carbon(void** state)
{
    prof_input("/carbons on");
    stbbr_for_xmlns("urn:xmpp:carbons:2", "<iq type='result'/>");

    prof_connect();
    assert_true(stbbr_received(
        "<iq id='*' type='set'><enable xmlns='urn:xmpp:carbons:2'/></iq>"));

    stbbr_send(
        "<presence to='stabber@localhost' from='buddy1@localhost/mobile'>"
        "<priority>10</priority>"
        "<status>On my mobile</status>"
        "</presence>");
    assert_true(prof_output_exact("Buddy1 (mobile) is online, \"On my mobile\""));
    prof_input("/msg Buddy1");
    assert_true(prof_output_exact("unencrypted"));

    stbbr_send(
        "<message type='chat' to='stabber@localhost/profanity' from='stabber@localhost'>"
        "<received xmlns='urn:xmpp:carbons:2'>"
        "<forwarded xmlns='urn:xmpp:forward:0'>"
        "<message id='*' xmlns='jabber:client' type='chat' lang='en' to='stabber@localhost/profanity' from='buddy1@localhost/mobile'>"
        "<body>test carbon from recipient</body>"
        "</message>"
        "</forwarded>"
        "</received>"
        "</message>");

    assert_true(prof_output_regex("Buddy1/mobile: test carbon from recipient"));
}

void
receive_self_carbon(void** state)
{
    prof_input("/carbons on");
    stbbr_for_xmlns("urn:xmpp:carbons:2", "<iq type='result'/>");

    prof_connect();
    assert_true(stbbr_received(
        "<iq id='*' type='set'><enable xmlns='urn:xmpp:carbons:2'/></iq>"));

    stbbr_send(
        "<presence to='stabber@localhost' from='buddy1@localhost/mobile'>"
        "<priority>10</priority>"
        "<status>On my mobile</status>"
        "</presence>");
    assert_true(prof_output_exact("Buddy1 (mobile) is online, \"On my mobile\""));
    prof_input("/msg Buddy1");
    assert_true(prof_output_exact("unencrypted"));

    stbbr_send(
        "<message type='chat' to='stabber@localhost/profanity' from='stabber@localhost'>"
        "<sent xmlns='urn:xmpp:carbons:2'>"
        "<forwarded xmlns='urn:xmpp:forward:0'>"
        "<message id='*' xmlns='jabber:client' type='chat' lang='en' to='buddy1@localhost/mobile' from='stabber@localhost/mobile'>"
        "<body>test carbon from sender</body>"
        "</message>"
        "</forwarded>"
        "</sent>"
        "</message>");

    assert_true(prof_output_regex("me: test carbon from sender"));
}

void
receive_private_carbon(void** state)
{
    prof_input("/carbons on");
    stbbr_for_xmlns("urn:xmpp:carbons:2", "<iq type='result'/>");

    prof_connect();
    assert_true(stbbr_received(
        "<iq id='*' type='set'><enable xmlns='urn:xmpp:carbons:2'/></iq>"));

    stbbr_send(
        "<presence to='stabber@localhost' from='buddy1@localhost/mobile'>"
        "<priority>10</priority>"
        "<status>On my mobile</status>"
        "</presence>");
    assert_true(prof_output_exact("Buddy1 (mobile) is online, \"On my mobile\""));
    prof_input("/msg Buddy1");
    assert_true(prof_output_exact("unencrypted"));

    stbbr_send(
        "<message type='chat' to='stabber@localhost/profanity' from='stabber@localhost'>"
        "<received xmlns='urn:xmpp:carbons:2'>"
        "<forwarded xmlns='urn:xmpp:forward:0'>"
        "<message id='*' xmlns='jabber:client' type='chat' lang='en' to='stabber@localhost/profanity' from='buddy1@localhost/mobile'>"
        "<body>Private carbon</body>"
        "<private xmlns='urn:xmpp:carbons:2'/>"
        "</message>"
        "</forwarded>"
        "</received>"
        "</message>");

    assert_true(prof_output_regex("Buddy1/mobile: Private carbon"));
}
