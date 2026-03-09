/*
 * test_ping.c
 *
 * Copyright (C) 2015 - 2017 James Booth <boothj5@gmail.com>
 * Copyright (C) 2019 - 2026 Michael Vetter <jubalh@iodoru.org>
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
ping_server(void** state)
{
    stbbr_for_query("http://jabber.org/protocol/disco#info",
                    "<iq id='*' to='stabber@localhost/profanity' type='result' from='localhost'>"
                    "<query xmlns='http://jabber.org/protocol/disco#info'>"
                    "<identity category='server' type='im' name='Prosody'/>"
                    "<feature var='urn:xmpp:ping'/>"
                    "</query>"
                    "</iq>");

    stbbr_for_xmlns("urn:xmpp:ping",
                    "<iq id='*' type='result' to='stabber@localhost/profanity'/>");

    prof_connect();

    prof_input("/ping");
    assert_true(stbbr_received(
        "<iq id='*' type='get'>"
        "<ping xmlns='urn:xmpp:ping'/>"
        "</iq>"));
    assert_true(prof_output_exact("Ping response from server"));

    prof_input("/ping");
    assert_true(stbbr_received(
        "<iq id='*' type='get'>"
        "<ping xmlns='urn:xmpp:ping'/>"
        "</iq>"));
    assert_true(prof_output_exact("Ping response from server"));
}

void
ping_server_not_supported(void** state)
{
    stbbr_for_query("http://jabber.org/protocol/disco#info",
                    "<iq id='*' to='stabber@localhost/profanity' type='result' from='localhost'>"
                    "<query xmlns='http://jabber.org/protocol/disco#info'>"
                    "<identity category='server' type='im' name='Stabber'/>"
                    "</query>"
                    "</iq>");

    prof_connect();

    prof_input("/ping");
    assert_true(prof_output_exact("Server does not support ping requests (urn:xmpp:ping)."));
}

void
ping_responds_to_server_request(void** state)
{
    prof_connect();

    stbbr_send(
        "<iq id='pingtest1' type='get' to='stabber@localhost/profanity' from='localhost'>"
        "<ping xmlns='urn:xmpp:ping'/>"
        "</iq>");

    assert_true(stbbr_received(
        "<iq id='pingtest1' to='localhost' from='stabber@localhost/profanity' type='result'/>"));
}

void
ping_jid(void** state)
{
    stbbr_for_id("*",
                 "<iq id='*' to='stabber@localhost/profanity' type='result' from='buddy1@localhost/mobile'>"
                 "<query xmlns='http://jabber.org/protocol/disco#info' node='http://profanity-im.github.io#LpT2xs3nun7jC2sq4gg3WRDQFZ4='>"
                 "<identity category='client' type='console' name='Profanity0.6.0'/>"
                 "<feature var='urn:xmpp:ping'/>"
                 "<feature var='http://jabber.org/protocol/disco#info'/>"
                 "<feature var='http://jabber.org/protocol/caps'/>"
                 "</query>"
                 "</iq>");

    stbbr_for_xmlns("urn:xmpp:ping",
                    "<iq id='*' type='result' from='buddy1@localhost/mobile' to='stabber@localhost/profanity'/>");

    prof_connect();

    stbbr_send(
        "<presence to='stabber@localhost' from='buddy1@localhost/mobile'>"
        "<priority>10</priority>"
        "<status>I'm here</status>"
        "<c "
        "hash='sha-1' "
        "xmlns='http://jabber.org/protocol/caps' "
        "node='http://profanity-im.github.io' "
        "ver='LpT2xs3nun7jC2sq4gg3WRDQFZ4='"
        "/>"
        "</presence>");
    assert_true(prof_output_exact("++ Buddy1 (mobile) is online, \"I'm here\""));

    assert_true(stbbr_received(
        "<iq id='*' to='buddy1@localhost/mobile' type='get'>"
        "<query xmlns='http://jabber.org/protocol/disco#info' node='http://profanity-im.github.io#LpT2xs3nun7jC2sq4gg3WRDQFZ4='/>"
        "</iq>"));

    prof_input("/ping buddy1@localhost/mobile");

    assert_true(stbbr_received(
        "<iq id='*' type='get' to='buddy1@localhost/mobile'>"
        "<ping xmlns='urn:xmpp:ping'/>"
        "</iq>"));
    assert_true(prof_output_exact("Ping response from buddy1@localhost/mobile"));
}

void
ping_jid_not_supported(void** state)
{
    stbbr_for_id("*",
                 "<iq id='*' to='stabber@localhost/profanity' type='result' from='buddy1@localhost/mobile'>"
                 "<query xmlns='http://jabber.org/protocol/disco#info' node='http://profanity-im.github.io#LpT2xs3nun7jC2sq4gg3WRDQFZ4='>"
                 "<identity category='client' type='console' name='Profanity0.6.0'/>"
                 "<feature var='http://jabber.org/protocol/disco#info'/>"
                 "<feature var='http://jabber.org/protocol/caps'/>"
                 "</query>"
                 "</iq>");

    prof_connect();

    stbbr_send(
        "<presence to='stabber@localhost' from='buddy1@localhost/mobile'>"
        "<priority>10</priority>"
        "<status>I'm here</status>"
        "<c "
        "hash='sha-1' "
        "xmlns='http://jabber.org/protocol/caps' "
        "node='http://profanity-im.github.io' "
        "ver='LpT2xs3nun7jC2sq4gg3WRDQFZ4='"
        "/>"
        "</presence>");
    assert_true(prof_output_exact("++ Buddy1 (mobile) is online, \"I'm here\""));

    assert_true(stbbr_received(
        "<iq id='*' to='buddy1@localhost/mobile' type='get'>"
        "<query xmlns='http://jabber.org/protocol/disco#info' node='http://profanity-im.github.io#LpT2xs3nun7jC2sq4gg3WRDQFZ4='/>"
        "</iq>"));

    prof_input("/ping buddy1@localhost/mobile");
    assert_true(prof_output_exact("buddy1@localhost/mobile does not support ping requests."));
}
