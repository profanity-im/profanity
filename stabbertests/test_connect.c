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

    sleep(1);

    assert_true(stbbr_verify(
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

    sleep(1);

    assert_true(stbbr_verify(
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

    sleep(1);

    assert_true(stbbr_verify(
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

//void
//show_presence_updates(void **state)
//{
//    will_return(ui_ask_password, strdup("password"));
//    expect_any_cons_show();
//
//    stbbr_for("roster",
//        "<iq id=\"roster\" type=\"result\" to=\"stabber@localhost/profanity\">"
//            "<query xmlns=\"jabber:iq:roster\" ver=\"362\">"
//                "<item jid=\"buddy1@localhost\" subscription=\"both\" name=\"Buddy1\"/>"
//                "<item jid=\"buddy2@localhost\" subscription=\"both\" name=\"Buddy2\"/>"
//            "</query>"
//        "</iq>"
//    );
//
//    cmd_process_input(strdup("/connect stabber@localhost port 5230"));
//    prof_process_xmpp(20);
//
//    stbbr_send(
//        "<presence to=\"stabber@localhost\" from=\"buddy1@localhost/mobile\">"
//            "<show>dnd</show>"
//            "<status>busy!</status>"
//        "</presence>"
//        "<presence to=\"stabber@localhost\" from=\"buddy1@localhost/laptop\">"
//            "<show>chat</show>"
//            "<status>Talk to me!</status>"
//        "</presence>"
//        "<presence to=\"stabber@localhost\" from=\"buddy2@localhost/work\">"
//            "<show>away</show>"
//            "<status>Out of office</status>"
//        "</presence>"
//    );
//
//    Resource *resource1 = resource_new("mobile", RESOURCE_DND, "busy!", 0);
//    expect_string(ui_contact_online, barejid, "buddy1@localhost");
//    expect_check(ui_contact_online, resource, (CheckParameterValue)resource_equal_check, resource1);
//    expect_value(ui_contact_online, last_activity, NULL);
//
//    Resource *resource2 = resource_new("laptop", RESOURCE_CHAT, "Talk to me!", 0);
//    expect_string(ui_contact_online, barejid, "buddy1@localhost");
//    expect_check(ui_contact_online, resource, (CheckParameterValue)resource_equal_check, resource2);
//    expect_value(ui_contact_online, last_activity, NULL);
//
//    Resource *resource3 = resource_new("work", RESOURCE_AWAY, "Out of office", 0);
//    expect_string(ui_contact_online, barejid, "buddy2@localhost");
//    expect_check(ui_contact_online, resource, (CheckParameterValue)resource_equal_check, resource3);
//    expect_value(ui_contact_online, last_activity, NULL);
//
//    prof_process_xmpp(20);
//}
//
//void
//sends_rooms_iq(void **state)
//{
//    will_return(ui_ask_password, strdup("password"));
//
//    expect_any_cons_show();
//
//    cmd_process_input(strdup("/connect stabber@localhost port 5230"));
//    prof_process_xmpp(20);
//
//    stbbr_for("confreq",
//        "<iq id=\"confreq\" type=\"result\" to=\"stabber@localhost/profanity\" from=\"conference.localhost\">"
//            "<query xmlns=\"http://jabber.org/protocol/disco#items\">"
//                "<item jid=\"chatroom@conference.localhost\" name=\"A chat room\"/>"
//                "<item jid=\"hangout@conference.localhost\" name=\"Another chat room\"/>"
//            "</query>"
//        "</iq>"
//    );
//
//    cmd_process_input(strdup("/rooms"));
//    prof_process_xmpp(20);
//
//    assert_true(stbbr_verify_last(
//        "<iq id=\"confreq\" to=\"conference.localhost\" type=\"get\">"
//            "<query xmlns=\"http://jabber.org/protocol/disco#items\"/>"
//        "</iq>"
//    ));
//}
//
//void
//multiple_pings(void **state)
//{
//    will_return(ui_ask_password, strdup("password"));
//
//    expect_any_cons_show();
//
//    cmd_process_input(strdup("/connect stabber@localhost port 5230"));
//    prof_process_xmpp(20);
//
//    expect_cons_show("Pinged server...");
//    expect_any_cons_show();
//    expect_cons_show("Pinged server...");
//    expect_any_cons_show();
//
//    stbbr_for("prof_ping_1",
//        "<iq id=\"prof_ping_1\" type=\"result\" to=\"stabber@localhost/profanity\"/>"
//    );
//    stbbr_for("prof_ping_2",
//        "<iq id=\"prof_ping_2\" type=\"result\" to=\"stabber@localhost/profanity\"/>"
//    );
//
//    cmd_process_input(strdup("/ping"));
//    prof_process_xmpp(20);
//    cmd_process_input(strdup("/ping"));
//    prof_process_xmpp(20);
//
//    assert_true(stbbr_verify(
//        "<iq id=\"prof_ping_1\" type=\"get\">"
//            "<ping xmlns=\"urn:xmpp:ping\"/>"
//        "</iq>"
//    ));
//    assert_true(stbbr_verify(
//        "<iq id=\"prof_ping_2\" type=\"get\">"
//            "<ping xmlns=\"urn:xmpp:ping\"/>"
//        "</iq>"
//    ));
//}
//
//void
//responds_to_ping(void **state)
//{
//    will_return(ui_ask_password, strdup("password"));
//
//    expect_any_cons_show();
//
//    cmd_process_input(strdup("/connect stabber@localhost port 5230"));
//    prof_process_xmpp(20);
//
//    stbbr_send(
//        "<iq id=\"ping1\" type=\"get\" to=\"stabber@localhost/profanity\" from=\"localhost\">"
//            "<ping xmlns=\"urn:xmpp:ping\"/>"
//        "</iq>"
//    );
//    prof_process_xmpp(20);
//
//    assert_true(stbbr_verify(
//        "<iq id=\"ping1\" type=\"result\" from=\"stabber@localhost/profanity\" to=\"localhost\"/>"
//    ));
//}
