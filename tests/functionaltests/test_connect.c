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
connect_jid_requests_roster(void **state)
{
    prof_connect();

    assert_true(stbbr_received(
        "<iq id='*' type='get'><query xmlns='jabber:iq:roster'/></iq>"
    ));
}

void
connect_jid_sends_presence_after_receiving_roster(void **state)
{
    prof_connect();

    assert_true(stbbr_received(
        "<presence id='*'>"
            "<c hash='sha-1' xmlns='http://jabber.org/protocol/caps' ver='*' node='http://profanity-im.github.io'/>"
        "</presence>"
    ));
}

void
connect_jid_requests_bookmarks(void **state)
{
    prof_connect();

    assert_true(stbbr_received(
        "<iq id='*' type='get'>"
            "<query xmlns='jabber:iq:private'>"
                "<storage xmlns='storage:bookmarks'/>"
            "</query>"
        "</iq>"
    ));
}

void
connect_bad_password(void **state)
{
    prof_input("/connect stabber@localhost server 127.0.0.1 port 5230 tls allow");
    prof_input("badpassword");

    assert_true(prof_output_exact("Connection closed."));
}

void
connect_shows_presence_updates(void **state)
{
    prof_connect();

    stbbr_send(
        "<presence to='stabber@localhost' from='buddy1@localhost/mobile'>"
            "<show>dnd</show>"
            "<status>busy!</status>"
        "</presence>"
    );
    assert_true(prof_output_exact("Buddy1 (mobile) is dnd, \"busy!\""));

    stbbr_send(
        "<presence to='stabber@localhost' from='buddy1@localhost/laptop'>"
            "<show>chat</show>"
            "<status>Talk to me!</status>"
        "</presence>"
    );
    assert_true(prof_output_exact("Buddy1 (laptop) is chat, \"Talk to me!\""));

    stbbr_send(
        "<presence to='stabber@localhost' from='buddy2@localhost/work'>"
            "<show>away</show>"
            "<status>Out of office</status>"
        "</presence>"
    );
    assert_true(prof_output_exact("Buddy2 (work) is away, \"Out of office\""));

    stbbr_send(
        "<presence to='stabber@localhost' from='buddy1@localhost/mobile'>"
            "<show>xa</show>"
            "<status>Gone :(</status>"
        "</presence>"
    );
    assert_true(prof_output_exact("Buddy1 (mobile) is xa, \"Gone :(\""));
}
