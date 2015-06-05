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
connect_jid(void **state)
{
    prof_connect("stabber@localhost", "password");

    assert_true(prof_output_exact("Connecting as stabber@localhost"));
    assert_true(prof_output_regex("stabber@localhost logged in successfully, .+online.+ \\(priority 0\\)\\."));
}

void
connect_jid_requests_roster(void **state)
{
    prof_connect("stabber@localhost", "password");

    assert_true(stbbr_received(
        "<iq id=\"*\" type=\"get\"><query xmlns=\"jabber:iq:roster\"/></iq>"
    ));
}

void
connect_jid_sends_presence_after_receiving_roster(void **state)
{
    stbbr_for_query("jabber:iq:roster",
        "<iq type=\"result\" to=\"stabber@localhost/profanity\">"
            "<query xmlns=\"jabber:iq:roster\" ver=\"362\">"
                "<item jid=\"buddy1@localhost\" subscription=\"both\" name=\"Buddy1\"/>"
                "<item jid=\"buddy2@localhost\" subscription=\"both\" name=\"Buddy2\"/>"
            "</query>"
        "</iq>"
    );

    prof_connect("stabber@localhost", "password");

    assert_true(stbbr_received(
        "<presence id=\"*\">"
            "<c hash=\"sha-1\" xmlns=\"http://jabber.org/protocol/caps\" ver=\"*\" node=\"http://www.profanity.im\"/>"
        "</presence>"
    ));
}

void
connect_jid_requests_bookmarks(void **state)
{
    prof_connect("stabber@localhost", "password");

    assert_true(stbbr_received(
        "<iq id=\"*\" type=\"get\">"
            "<query xmlns=\"jabber:iq:private\">"
                "<storage xmlns=\"storage:bookmarks\"/>"
            "</query>"
        "</iq>"
    ));
}

void
connect_bad_password(void **state)
{
    prof_connect("stabber@localhost", "badpassword");

    assert_true(prof_output_exact("Login failed."));
}

void
connect_shows_presence_updates(void **state)
{
    stbbr_for_query("jabber:iq:roster",
        "<iq type=\"result\" to=\"stabber@localhost/profanity\">"
            "<query xmlns=\"jabber:iq:roster\" ver=\"362\">"
                "<item jid=\"buddy1@localhost\" subscription=\"both\" name=\"Buddy1\"/>"
                "<item jid=\"buddy2@localhost\" subscription=\"both\" name=\"Buddy2\"/>"
            "</query>"
        "</iq>"
    );

    stbbr_for_id("prof_presence_1",
        "<presence to=\"stabber@localhost\" from=\"buddy1@localhost/mobile\">"
            "<show>dnd</show>"
            "<status>busy!</status>"
        "</presence>"
        "<presence to=\"stabber@localhost\" from=\"buddy1@localhost/laptop\">"
            "<show>chat</show>"
            "<status>Talk to me!</status>"
        "</presence>"
        "<presence to=\"stabber@localhost\" from=\"buddy2@localhost/work\">"
            "<show>away</show>"
            "<status>Out of office</status>"
        "</presence>"
    );

    prof_connect("stabber@localhost", "password");

    assert_true(prof_output_exact("Buddy1 (mobile) is dnd, \"busy!\""));
    assert_true(prof_output_exact("Buddy1 (laptop) is chat, \"Talk to me!\""));
    assert_true(prof_output_exact("Buddy2 (work) is away, \"Out of office\""));

    stbbr_send(
        "<presence to=\"stabber@localhost\" from=\"buddy1@localhost/mobile\">"
            "<show>xa</show>"
            "<status>Gone :(</status>"
        "</presence>"
    );

    assert_true(prof_output_exact("Buddy1 (mobile) is xa, \"Gone :(\""));
}
