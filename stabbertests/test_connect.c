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
    prof_input("/connect stabber@localhost port 5230");
    prof_input("password");

    assert_true(prof_output("Connecting as stabber@localhost"));
    assert_true(prof_output("stabber@localhost logged in successfully"));
}

void
connect_jid_requests_roster(void **state)
{
    prof_input("/connect stabber@localhost port 5230");
    prof_input("password");

    assert_true(stbbr_received(
        "<iq id=\"*\" type=\"get\"><query xmlns=\"jabber:iq:roster\"/></iq>"
    ));
}

void
connect_jid_sends_presence_after_receiving_roster(void **state)
{
    stbbr_for("roster",
        "<iq id=\"roster\" type=\"result\" to=\"stabber@localhost/profanity\">"
            "<query xmlns=\"jabber:iq:roster\" ver=\"362\">"
                "<item jid=\"buddy1@localhost\" subscription=\"both\" name=\"Buddy1\"/>"
                "<item jid=\"buddy2@localhost\" subscription=\"both\" name=\"Buddy2\"/>"
            "</query>"
        "</iq>"
    );

    prof_input("/connect stabber@localhost port 5230");
    prof_input("password");

    assert_true(stbbr_received(
        "<presence id=\"*\">"
            "<c hash=\"sha-1\" xmlns=\"http://jabber.org/protocol/caps\" ver=\"*\" node=\"http://www.profanity.im\"/>"
        "</presence>"
    ));
}

void
connect_jid_requests_bookmarks(void **state)
{
    prof_input("/connect stabber@localhost port 5230");
    prof_input("password");

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
    prof_input("/connect stabber@localhost port 5230");
    prof_input("badpassword");

    assert_true(prof_output("Login failed."));
}

void
show_presence_updates(void **state)
{
    stbbr_for("roster",
        "<iq id=\"roster\" type=\"result\" to=\"stabber@localhost/profanity\">"
            "<query xmlns=\"jabber:iq:roster\" ver=\"362\">"
                "<item jid=\"buddy1@localhost\" subscription=\"both\" name=\"Buddy1\"/>"
                "<item jid=\"buddy2@localhost\" subscription=\"both\" name=\"Buddy2\"/>"
            "</query>"
        "</iq>"
    );

    stbbr_for("prof_presence_1",
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

    prof_input("/connect stabber@localhost port 5230");
    prof_input("password");

    assert_true(prof_output("Buddy1 (mobile) is dnd"));
    assert_true(prof_output("Buddy1 (laptop) is chat"));
    assert_true(prof_output("Buddy2 (work) is away"));

    stbbr_send(
        "<presence to=\"stabber@localhost\" from=\"buddy1@localhost/mobile\">"
            "<show>xa</show>"
            "<status>Gone :(</status>"
        "</presence>"
    );

    assert_true(prof_output("Buddy1 (mobile) is xa"));
}

void
sends_rooms_iq(void **state)
{
    stbbr_for("confreq",
        "<iq id=\"confreq\" type=\"result\" to=\"stabber@localhost/profanity\" from=\"conference.localhost\">"
            "<query xmlns=\"http://jabber.org/protocol/disco#items\">"
                "<item jid=\"chatroom@conference.localhost\" name=\"A chat room\"/>"
                "<item jid=\"hangout@conference.localhost\" name=\"Another chat room\"/>"
            "</query>"
        "</iq>"
    );

    prof_input("/connect stabber@localhost port 5230");
    prof_input("password");
    prof_input("/rooms");

    assert_true(stbbr_last_received(
        "<iq id=\"confreq\" to=\"conference.localhost\" type=\"get\">"
            "<query xmlns=\"http://jabber.org/protocol/disco#items\"/>"
        "</iq>"
    ));
}

void
multiple_pings(void **state)
{
    stbbr_for("prof_ping_1",
        "<iq id=\"prof_ping_1\" type=\"result\" to=\"stabber@localhost/profanity\"/>"
    );
    stbbr_for("prof_ping_2",
        "<iq id=\"prof_ping_2\" type=\"result\" to=\"stabber@localhost/profanity\"/>"
    );

    prof_input("/connect stabber@localhost port 5230");
    prof_input("password");

    prof_input("/ping");
    assert_true(stbbr_received(
        "<iq id=\"prof_ping_1\" type=\"get\">"
            "<ping xmlns=\"urn:xmpp:ping\"/>"
        "</iq>"
    ));
    assert_true(prof_output("Ping response from server"));

    prof_input("/ping");
    assert_true(stbbr_received(
        "<iq id=\"prof_ping_2\" type=\"get\">"
            "<ping xmlns=\"urn:xmpp:ping\"/>"
        "</iq>"
    ));
    assert_true(prof_output("Ping response from server"));
}

void
responds_to_ping(void **state)
{
    prof_input("/connect stabber@localhost port 5230");
    prof_input("password");

    assert_true(prof_output("stabber@localhost logged in successfully"));

    stbbr_send(
        "<iq id=\"pingtest1\" type=\"get\" to=\"stabber@localhost/profanity\" from=\"localhost\">"
            "<ping xmlns=\"urn:xmpp:ping\"/>"
        "</iq>"
    );

    assert_true(stbbr_received(
        "<iq id=\"pingtest1\" type=\"result\" from=\"stabber@localhost/profanity\" to=\"localhost\"/>"
    ));
}
