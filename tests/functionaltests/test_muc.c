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
        "<presence id='*' to='testroom@conference.localhost/stabber'>"
            "<x xmlns='http://jabber.org/protocol/muc'/>"
            "<c hash='sha-1' xmlns='http://jabber.org/protocol/caps' ver='*' node='http://www.profanity.im'/>"
        "</presence>"
    ));
}

void
sends_room_join_with_nick(void **state)
{
    prof_connect();

    prof_input("/join testroom@conference.localhost nick testnick");

    assert_true(stbbr_last_received(
        "<presence id='*' to='testroom@conference.localhost/testnick'>"
            "<x xmlns='http://jabber.org/protocol/muc'/>"
            "<c hash='sha-1' xmlns='http://jabber.org/protocol/caps' ver='*' node='http://www.profanity.im'/>"
        "</presence>"
    ));
}

void
sends_room_join_with_password(void **state)
{
    prof_connect();

    prof_input("/join testroom@conference.localhost password testpassword");

    assert_true(stbbr_last_received(
        "<presence id='*' to='testroom@conference.localhost/stabber'>"
            "<x xmlns='http://jabber.org/protocol/muc'>"
                "<password>testpassword</password>"
            "</x>"
            "<c hash='sha-1' xmlns='http://jabber.org/protocol/caps' ver='*' node='http://www.profanity.im'/>"
        "</presence>"
    ));
}

void
sends_room_join_with_nick_and_password(void **state)
{
    prof_connect();

    prof_input("/join testroom@conference.localhost nick testnick password testpassword");

    assert_true(stbbr_last_received(
        "<presence id='*' to='testroom@conference.localhost/testnick'>"
            "<x xmlns='http://jabber.org/protocol/muc'>"
                "<password>testpassword</password>"
            "</x>"
            "<c hash='sha-1' xmlns='http://jabber.org/protocol/caps' ver='*' node='http://www.profanity.im'/>"
        "</presence>"
    ));
}

void
shows_role_and_affiliation_on_join(void **state)
{
    prof_connect();

    stbbr_for_id("prof_join_4",
        "<presence id='prof_join_4' lang='en' to='stabber@localhost/profanity' from='testroom@conference.localhost/stabber'>"
            "<c hash='sha-1' xmlns='http://jabber.org/protocol/caps' node='http://www.profanity.im' ver='*'/>"
            "<x xmlns='http://jabber.org/protocol/muc#user'>"
                "<item role='participant' jid='stabber@localhost/profanity' affiliation='none'/>"
            "</x>"
            "<status code='110'/>"
        "</presence>"
    );

    prof_input("/join testroom@conference.localhost");

    assert_true(prof_output_exact("-> You have joined the room as stabber, role: participant, affiliation: none"));
}

void
shows_subject_on_join(void **state)
{
    prof_connect();

    stbbr_for_id("prof_join_4",
        "<presence id='prof_join_4' lang='en' to='stabber@localhost/profanity' from='testroom@conference.localhost/stabber'>"
            "<c hash='sha-1' xmlns='http://jabber.org/protocol/caps' node='http://www.profanity.im' ver='*'/>"
            "<x xmlns='http://jabber.org/protocol/muc#user'>"
                "<item role='participant' jid='stabber@localhost/profanity' affiliation='none'/>"
            "</x>"
            "<status code='110'/>"
        "</presence>"
    );

    prof_input("/join testroom@conference.localhost");
    assert_true(prof_output_exact("-> You have joined the room as stabber, role: participant, affiliation: none"));

    stbbr_send(
        "<message type='groupchat' to='stabber@localhost/profanity' from='testroom@conference.localhost'>"
            "<subject>Test room subject</subject>"
            "<body>anothernick has set the subject to: Test room subject</body>"
        "</message>"
    );

    assert_true(prof_output_regex("Room subject: .+Test room subject"));
}

void
shows_history_message(void **state)
{
    prof_connect();

    stbbr_for_id("prof_join_4",
        "<presence id='prof_join_4' lang='en' to='stabber@localhost/profanity' from='testroom@conference.localhost/stabber'>"
            "<c hash='sha-1' xmlns='http://jabber.org/protocol/caps' node='http://www.profanity.im' ver='*'/>"
            "<x xmlns='http://jabber.org/protocol/muc#user'>"
                "<item role='participant' jid='stabber@localhost/profanity' affiliation='none'/>"
            "</x>"
            "<status code='110'/>"
        "</presence>"
    );

    prof_input("/join testroom@conference.localhost");
    assert_true(prof_output_exact("-> You have joined the room as stabber, role: participant, affiliation: none"));

    stbbr_send(
        "<message type='groupchat' to='stabber@localhost/profanity' from='testroom@conference.localhost/testoccupant'>"
            "<body>an old message</body>"
            "<delay xmlns='urn:xmpp:delay' stamp='2015-12-19T23:55:25Z' from='testroom@conference.localhost'/>"
            "<x xmlns='jabber:x:delay' stamp='20151219T23:55:25'/>"
        "</message>"
    );

    assert_true(prof_output_regex("testoccupant: an old message"));
}

void
shows_occupant_join(void **state)
{
    prof_connect();

    stbbr_for_id("prof_join_4",
        "<presence id='prof_join_4' lang='en' to='stabber@localhost/profanity' from='testroom@conference.localhost/stabber'>"
            "<c hash='sha-1' xmlns='http://jabber.org/protocol/caps' node='http://www.profanity.im' ver='*'/>"
            "<x xmlns='http://jabber.org/protocol/muc#user'>"
                "<item role='participant' jid='stabber@localhost/profanity' affiliation='none'/>"
            "</x>"
            "<status code='110'/>"
        "</presence>"
    );

    prof_input("/join testroom@conference.localhost");
    assert_true(prof_output_exact("-> You have joined the room as stabber, role: participant, affiliation: none"));

    stbbr_send(
        "<presence to='stabber@localhost/profanity' from='testroom@conference.localhost/testoccupant'>"
            "<x xmlns='http://jabber.org/protocol/muc#user'>"
                "<item role='participant' jid='someuser@someserver.org/work' affiliation='none'/>"
            "</x>"
        "</presence>"
    );

    assert_true(prof_output_exact("-> testoccupant has joined the room, role: participant, affiliation: none"));
}

void
shows_message(void **state)
{
    prof_connect();

    stbbr_for_id("prof_join_4",
        "<presence id='prof_join_4' lang='en' to='stabber@localhost/profanity' from='testroom@conference.localhost/stabber'>"
            "<c hash='sha-1' xmlns='http://jabber.org/protocol/caps' node='http://www.profanity.im' ver='*'/>"
            "<x xmlns='http://jabber.org/protocol/muc#user'>"
                "<item role='participant' jid='stabber@localhost/profanity' affiliation='none'/>"
            "</x>"
            "<status code='110'/>"
        "</presence>"
    );

    prof_input("/join testroom@conference.localhost");
    assert_true(prof_output_exact("-> You have joined the room as stabber, role: participant, affiliation: none"));

    stbbr_send(
        "<message type='groupchat' to='stabber@localhost/profanity' from='testroom@conference.localhost/testoccupant'>"
            "<body>a new message</body>"
        "</message>"
    );

    assert_true(prof_output_regex("testoccupant: .+a new message"));
}

void
shows_all_messages_in_console_when_window_not_focussed(void **state)
{
    prof_connect();

    stbbr_for_id("prof_join_4",
        "<presence id='prof_join_4' lang='en' to='stabber@localhost/profanity' from='testroom@conference.localhost/stabber'>"
            "<c hash='sha-1' xmlns='http://jabber.org/protocol/caps' node='http://www.profanity.im' ver='*'/>"
            "<x xmlns='http://jabber.org/protocol/muc#user'>"
                "<item role='participant' jid='stabber@localhost/profanity' affiliation='none'/>"
            "</x>"
            "<status code='110'/>"
        "</presence>"
    );

    prof_input("/join testroom@conference.localhost");
    assert_true(prof_output_exact("-> You have joined the room as stabber, role: participant, affiliation: none"));

    prof_input("/win 1");
    assert_true(prof_output_exact("Profanity. Type /help for help information."));

    stbbr_send(
        "<message type='groupchat' to='stabber@localhost/profanity' from='testroom@conference.localhost/testoccupant'>"
            "<body>a new message</body>"
        "</message>"
    );

    assert_true(prof_output_exact("<< room message: testoccupant in testroom@conference.localhost (win 2)"));

    stbbr_send(
        "<message type='groupchat' to='stabber@localhost/profanity' from='testroom@conference.localhost/anotheroccupant'>"
            "<body>some other message</body>"
        "</message>"
    );

    assert_true(prof_output_exact("<< room message: anotheroccupant in testroom@conference.localhost (win 2)"));
}

void
shows_first_message_in_console_when_window_not_focussed(void **state)
{
    prof_connect();

    prof_input("/console muc first");
    assert_true(prof_output_exact("Console MUC messages set: first"));

    stbbr_for_id("prof_join_4",
        "<presence id='prof_join_4' lang='en' to='stabber@localhost/profanity' from='testroom@conference.localhost/stabber'>"
            "<c hash='sha-1' xmlns='http://jabber.org/protocol/caps' node='http://www.profanity.im' ver='*'/>"
            "<x xmlns='http://jabber.org/protocol/muc#user'>"
                "<item role='participant' jid='stabber@localhost/profanity' affiliation='none'/>"
            "</x>"
            "<status code='110'/>"
        "</presence>"
    );

    prof_input("/join testroom@conference.localhost");
    assert_true(prof_output_exact("-> You have joined the room as stabber, role: participant, affiliation: none"));

    prof_input("/win 1");
    assert_true(prof_output_exact("Profanity. Type /help for help information."));

    stbbr_send(
        "<message type='groupchat' to='stabber@localhost/profanity' from='testroom@conference.localhost/testoccupant'>"
            "<body>a new message</body>"
        "</message>"
    );

    assert_true(prof_output_exact("<< room message: testroom@conference.localhost (win 2)"));
    prof_input("/clear");
    prof_input("/about");
    assert_true(prof_output_exact("Type '/help' to show complete help."));

    stbbr_send(
        "<message type='groupchat' to='stabber@localhost/profanity' from='testroom@conference.localhost/anotheroccupant'>"
            "<body>some other message</body>"
        "</message>"
    );

    prof_timeout(2);
    assert_false(prof_output_exact("<< room message: testroom@conference.localhost (win 2)"));
    prof_timeout_reset();
}

void
shows_no_message_in_console_when_window_not_focussed(void **state)
{
    prof_connect();

    prof_input("/console muc none");
    assert_true(prof_output_exact("Console MUC messages set: none"));

    stbbr_for_id("prof_join_4",
        "<presence id='prof_join_4' lang='en' to='stabber@localhost/profanity' from='testroom@conference.localhost/stabber'>"
            "<c hash='sha-1' xmlns='http://jabber.org/protocol/caps' node='http://www.profanity.im' ver='*'/>"
            "<x xmlns='http://jabber.org/protocol/muc#user'>"
                "<item role='participant' jid='stabber@localhost/profanity' affiliation='none'/>"
            "</x>"
            "<status code='110'/>"
        "</presence>"
    );

    prof_input("/join testroom@conference.localhost");
    assert_true(prof_output_exact("-> You have joined the room as stabber, role: participant, affiliation: none"));

    prof_input("/win 1");
    assert_true(prof_output_exact("Profanity. Type /help for help information."));

    stbbr_send(
        "<message type='groupchat' to='stabber@localhost/profanity' from='testroom@conference.localhost/testoccupant'>"
            "<body>a new message</body>"
        "</message>"
    );

    prof_timeout(2);
    assert_false(prof_output_exact("testroom@conference.localhost (win 2)"));
    prof_timeout_reset();
}
