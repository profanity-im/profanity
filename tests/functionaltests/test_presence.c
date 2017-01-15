#include <glib.h>
#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>
#include <stdlib.h>
#include <string.h>

#include <stabber.h>
#include <expect.h>

#include "proftest.h"

void
presence_online(void **state)
{
    prof_connect();

    prof_input("/online");

    assert_true(stbbr_received(
        "<presence id='prof_presence_3'>"
            "<c hash='sha-1' xmlns='http://jabber.org/protocol/caps' ver='*' node='http://www.profanity.im'/>"
        "</presence>"
    ));

    assert_true(prof_output_exact("Status set to online (priority 0)"));
}

void
presence_online_with_message(void **state)
{
    prof_connect();

    prof_input("/online \"Hi there\"");

    assert_true(stbbr_received(
        "<presence id='prof_presence_4'>"
            "<status>Hi there</status>"
            "<c hash='sha-1' xmlns='http://jabber.org/protocol/caps' ver='*' node='http://www.profanity.im'/>"
        "</presence>"
    ));

    assert_true(prof_output_exact("Status set to online (priority 0), \"Hi there\"."));
}

void
presence_away(void **state)
{
    prof_connect();

    prof_input("/away");

    assert_true(stbbr_received(
        "<presence id='prof_presence_4'>"
            "<show>away</show>"
            "<c hash='sha-1' xmlns='http://jabber.org/protocol/caps' ver='*' node='http://www.profanity.im'/>"
        "</presence>"
    ));

    assert_true(prof_output_exact("Status set to away (priority 0)"));
}

void
presence_away_with_message(void **state)
{
    prof_connect();

    prof_input("/away \"I'm not here for a bit\"");

    assert_true(stbbr_received(
        "<presence id='prof_presence_4'>"
            "<show>away</show>"
            "<status>I'm not here for a bit</status>"
            "<c hash='sha-1' xmlns='http://jabber.org/protocol/caps' ver='*' node='http://www.profanity.im'/>"
        "</presence>"
    ));

    assert_true(prof_output_exact("Status set to away (priority 0), \"I'm not here for a bit\"."));
}

void
presence_xa(void **state)
{
    prof_connect();

    prof_input("/xa");

    assert_true(stbbr_received(
        "<presence id='prof_presence_4'>"
            "<show>xa</show>"
            "<c hash='sha-1' xmlns='http://jabber.org/protocol/caps' ver='*' node='http://www.profanity.im'/>"
        "</presence>"
    ));

    assert_true(prof_output_exact("Status set to xa (priority 0)"));
}

void
presence_xa_with_message(void **state)
{
    prof_connect();

    prof_input("/xa \"Gone to the shops\"");

    assert_true(stbbr_received(
        "<presence id='prof_presence_4'>"
            "<show>xa</show>"
            "<status>Gone to the shops</status>"
            "<c hash='sha-1' xmlns='http://jabber.org/protocol/caps' ver='*' node='http://www.profanity.im'/>"
        "</presence>"
    ));

    assert_true(prof_output_exact("Status set to xa (priority 0), \"Gone to the shops\"."));
}

void
presence_dnd(void **state)
{
    prof_connect();

    prof_input("/dnd");

    assert_true(stbbr_received(
        "<presence id='prof_presence_4'>"
            "<show>dnd</show>"
            "<c hash='sha-1' xmlns='http://jabber.org/protocol/caps' ver='*' node='http://www.profanity.im'/>"
        "</presence>"
    ));

    assert_true(prof_output_exact("Status set to dnd (priority 0)"));
}

void
presence_dnd_with_message(void **state)
{
    prof_connect();

    prof_input("/dnd \"Working\"");

    assert_true(stbbr_received(
        "<presence id='prof_presence_4'>"
            "<show>dnd</show>"
            "<status>Working</status>"
            "<c hash='sha-1' xmlns='http://jabber.org/protocol/caps' ver='*' node='http://www.profanity.im'/>"
        "</presence>"
    ));

    assert_true(prof_output_exact("Status set to dnd (priority 0), \"Working\"."));
}

void
presence_chat(void **state)
{
    prof_connect();

    prof_input("/chat");

    assert_true(stbbr_received(
        "<presence id='prof_presence_4'>"
            "<show>chat</show>"
            "<c hash='sha-1' xmlns='http://jabber.org/protocol/caps' ver='*' node='http://www.profanity.im'/>"
        "</presence>"
    ));

    assert_true(prof_output_exact("Status set to chat (priority 0)"));
}

void
presence_chat_with_message(void **state)
{
    prof_connect();

    prof_input("/chat \"Free to talk\"");

    assert_true(stbbr_received(
        "<presence id='prof_presence_4'>"
            "<show>chat</show>"
            "<status>Free to talk</status>"
            "<c hash='sha-1' xmlns='http://jabber.org/protocol/caps' ver='*' node='http://www.profanity.im'/>"
        "</presence>"
    ));

    assert_true(prof_output_exact("Status set to chat (priority 0), \"Free to talk\"."));
}

void
presence_set_priority(void **state)
{
    prof_connect();

    prof_input("/priority 25");

    assert_true(stbbr_received(
        "<presence id='prof_presence_4'>"
            "<priority>25</priority>"
            "<c hash='sha-1' xmlns='http://jabber.org/protocol/caps' ver='*' node='http://www.profanity.im'/>"
        "</presence>"
    ));

    assert_true(prof_output_exact("Priority set to 25."));
}

void
presence_includes_priority(void **state)
{
    prof_connect();

    prof_input("/priority 25");
    assert_true(stbbr_received(
        "<presence id='prof_presence_4'>"
            "<priority>25</priority>"
            "<c hash='sha-1' xmlns='http://jabber.org/protocol/caps' ver='*' node='http://www.profanity.im'/>"
        "</presence>"
    ));
    assert_true(prof_output_exact("Priority set to 25."));

    prof_input("/chat \"Free to talk\"");
    assert_true(stbbr_received(
        "<presence id='prof_presence_5'>"
            "<priority>25</priority>"
            "<show>chat</show>"
            "<status>Free to talk</status>"
            "<c hash='sha-1' xmlns='http://jabber.org/protocol/caps' ver='*' node='http://www.profanity.im'/>"
        "</presence>"
    ));
    assert_true(prof_output_exact("Status set to chat (priority 25), \"Free to talk\"."));
}

void
presence_keeps_status(void **state)
{
    prof_connect();

    prof_input("/chat \"Free to talk\"");
    assert_true(stbbr_received(
        "<presence id='prof_presence_4'>"
            "<show>chat</show>"
            "<status>Free to talk</status>"
            "<c hash='sha-1' xmlns='http://jabber.org/protocol/caps' ver='*' node='http://www.profanity.im'/>"
        "</presence>"
    ));
    assert_true(prof_output_exact("Status set to chat (priority 0), \"Free to talk\"."));

    prof_input("/priority 25");
    assert_true(stbbr_received(
        "<presence id='prof_presence_5'>"
            "<show>chat</show>"
            "<status>Free to talk</status>"
            "<priority>25</priority>"
            "<c hash='sha-1' xmlns='http://jabber.org/protocol/caps' ver='*' node='http://www.profanity.im'/>"
        "</presence>"
    ));
    assert_true(prof_output_exact("Priority set to 25."));
}

void
presence_received(void **state)
{
    prof_connect();

    stbbr_send(
        "<presence to='stabber@localhost' from='buddy1@localhost/mobile'>"
            "<priority>10</priority>"
            "<status>I'm here</status>"
        "</presence>"
    );

    assert_true(prof_output_exact("Buddy1 (mobile) is online, \"I'm here\""));
}

// Typical use case for gateways that don't support resources
void
presence_missing_resource_defaults(void **state)
{
    prof_connect();

    stbbr_send(
        "<presence to='stabber@localhost' from='buddy1@localhost'>"
            "<priority>15</priority>"
            "<status>My status</status>"
        "</presence>"
    );

    assert_true(prof_output_exact("Buddy1 is online, \"My status\""));

    prof_input("/info Buddy1");

    assert_true(prof_output_exact("__prof_default (15), online"));
}
