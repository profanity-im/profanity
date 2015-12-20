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
sends_room_join(void **state)
{
    prof_connect();

    prof_input("/join testroom@conference.localhost");

    assert_true(stbbr_last_received(
        "<presence id=\"*\" to=\"testroom@conference.localhost/stabber\">"
            "<x xmlns=\"http://jabber.org/protocol/muc\"/>"
            "<c hash=\"sha-1\" xmlns=\"http://jabber.org/protocol/caps\" ver=\"*\" node=\"http://www.profanity.im\"/>"
        "</presence>"
    ));
}

void
sends_room_join_with_default_muc_service(void **state)
{
    prof_connect();

    prof_input("/join testroom");

    assert_true(stbbr_last_received(
        "<presence id=\"*\" to=\"testroom@conference.localhost/stabber\">"
            "<x xmlns=\"http://jabber.org/protocol/muc\"/>"
            "<c hash=\"sha-1\" xmlns=\"http://jabber.org/protocol/caps\" ver=\"*\" node=\"http://www.profanity.im\"/>"
        "</presence>"
    ));
}

void
sends_room_join_with_nick(void **state)
{
    prof_connect();

    prof_input("/join testroom@conference.localhost nick testnick");

    assert_true(stbbr_last_received(
        "<presence id=\"*\" to=\"testroom@conference.localhost/testnick\">"
            "<x xmlns=\"http://jabber.org/protocol/muc\"/>"
            "<c hash=\"sha-1\" xmlns=\"http://jabber.org/protocol/caps\" ver=\"*\" node=\"http://www.profanity.im\"/>"
        "</presence>"
    ));
}

void
sends_room_join_with_password(void **state)
{
    prof_connect();

    prof_input("/join testroom@conference.localhost password testpassword");

    assert_true(stbbr_last_received(
        "<presence id=\"*\" to=\"testroom@conference.localhost/stabber\">"
            "<x xmlns=\"http://jabber.org/protocol/muc\">"
                "<password>testpassword</password>"
            "</x>"
            "<c hash=\"sha-1\" xmlns=\"http://jabber.org/protocol/caps\" ver=\"*\" node=\"http://www.profanity.im\"/>"
        "</presence>"
    ));
}

void
sends_room_join_with_nick_and_password(void **state)
{
    prof_connect();

    prof_input("/join testroom@conference.localhost nick testnick password testpassword");

    assert_true(stbbr_last_received(
        "<presence id=\"*\" to=\"testroom@conference.localhost/testnick\">"
            "<x xmlns=\"http://jabber.org/protocol/muc\">"
                "<password>testpassword</password>"
            "</x>"
            "<c hash=\"sha-1\" xmlns=\"http://jabber.org/protocol/caps\" ver=\"*\" node=\"http://www.profanity.im\"/>"
        "</presence>"
    ));
}

void
show_role_and_affiliation_on_join(void **state)
{
    prof_connect();

    stbbr_for_id("prof_join_2",
        "<presence id=\"prof_join_2\" lang=\"en\" to=\"stabber@localhost/profanity\" from=\"testroom@conference.localhost/stabber\">"
            "<c hash=\"sha-1\" xmlns=\"http://jabber.org/protocol/caps\" node=\"http://www.profanity.im\" ver=\"*\"/>"
            "<x xmlns=\"http://jabber.org/protocol/muc#user\">"
                "<item role=\"participant\" jid=\"stabber@localhost/profanity\" affiliation=\"none\"/>"
            "</x>"
            "<status code=\"110\"/>"
        "</presence>"
    );

    prof_input("/join testroom@conference.localhost");

    assert_true(prof_output_exact("-> You have joined the room as stabber, role: participant, affiliation: none"));
}

void
show_subject_on_join(void **state)
{
    prof_connect();

    stbbr_for_id("prof_join_2",
        "<presence id=\"prof_join_2\" lang=\"en\" to=\"stabber@localhost/profanity\" from=\"testroom@conference.localhost/stabber\">"
            "<c hash=\"sha-1\" xmlns=\"http://jabber.org/protocol/caps\" node=\"http://www.profanity.im\" ver=\"*\"/>"
            "<x xmlns=\"http://jabber.org/protocol/muc#user\">"
                "<item role=\"participant\" jid=\"stabber@localhost/profanity\" affiliation=\"none\"/>"
            "</x>"
            "<status code=\"110\"/>"
        "</presence>"
    );

    prof_input("/join testroom@conference.localhost");
    assert_true(prof_output_exact("-> You have joined the room as stabber, role: participant, affiliation: none"));

    stbbr_send(
        "<message type=\"groupchat\" to=\"stabber@localhost/profanity\" from=\"testroom@conference.localhost\">"
            "<subject>Test room subject</subject>"
            "<body>anothernick has set the subject to: Test room subject</body>"
        "</message>"
    );

    assert_true(prof_output_regex("Room subject: .+Test room subject"));
}
