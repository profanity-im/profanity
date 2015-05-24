#include <glib.h>
#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>
#include <stdlib.h>
#include <string.h>

#include <stabber.h>

#include "proftest.h"
#include "xmpp/xmpp.h"
#include "ui/stub_ui.h"
#include "ui/window.h"
#include "command/command.h"

void
connect_jid(void **state)
{
    will_return(ui_ask_password, strdup("password"));
    expect_cons_show("Connecting as stabber@localhost");

    cmd_process_input(strdup("/connect stabber@localhost port 5230"));
    prof_process_xmpp(20);

    jabber_conn_status_t status = jabber_get_connection_status();
    assert_true(status == JABBER_CONNECTED);
}

void
connect_jid_requests_roster(void **state)
{
    will_return(ui_ask_password, strdup("password"));
    expect_any_cons_show();

    cmd_process_input(strdup("/connect stabber@localhost port 5230"));
    prof_process_xmpp(20);

    assert_true(stbbr_verify(
        "<iq id=\"*\" type=\"get\"><query xmlns=\"jabber:iq:roster\"/></iq>"
    ));
}

void
connect_jid_sends_presence_after_receiving_roster(void **state)
{
    will_return(ui_ask_password, strdup("password"));
    expect_any_cons_show();

    stbbr_for("roster",
        "<iq id=\"roster\" type=\"result\" to=\"stabber@localhost/profanity\">"
            "<query xmlns=\"jabber:iq:roster\" ver=\"362\">"
                "<item jid=\"buddy1@localhost\" subscription=\"both\" name=\"Buddy1\"/>"
                "<item jid=\"buddy2@localhost\" subscription=\"both\" name=\"Buddy2\"/>"
            "</query>"
        "</iq>"
    );

    cmd_process_input(strdup("/connect stabber@localhost port 5230"));
    prof_process_xmpp(20);

    assert_true(stbbr_verify(
        "<presence id=\"*\">"
            "<c hash=\"sha-1\" xmlns=\"http://jabber.org/protocol/caps\" ver=\"*\" node=\"http://www.profanity.im\"/>"
        "</presence>"
    ));
}

void
connect_jid_requests_bookmarks(void **state)
{
    will_return(ui_ask_password, strdup("password"));
    expect_any_cons_show();

    cmd_process_input(strdup("/connect stabber@localhost port 5230"));
    prof_process_xmpp(20);

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
    will_return(ui_ask_password, strdup("badpassword"));

    expect_cons_show("Connecting as stabber@localhost");
    expect_cons_show_error("Login failed.");

    cmd_process_input(strdup("/connect stabber@localhost port 5230"));
    prof_process_xmpp(20);

    jabber_conn_status_t status = jabber_get_connection_status();
    assert_true(status == JABBER_DISCONNECTED);
}

void
sends_rooms_iq(void **state)
{
    will_return(ui_ask_password, strdup("password"));

    expect_any_cons_show();

    cmd_process_input(strdup("/connect stabber@localhost port 5230"));
    prof_process_xmpp(20);

    stbbr_for("confreq",
        "<iq id=\"confreq\" type=\"result\" to=\"stabber@localhost/profanity\" from=\"conference.localhost\">"
            "<query xmlns=\"http://jabber.org/protocol/disco#items\">"
                "<item jid=\"chatroom@conference.localhost\" name=\"A chat room\"/>"
                "<item jid=\"hangout@conference.localhost\" name=\"Another chat room\"/>"
            "</query>"
        "</iq>"
    );

    cmd_process_input(strdup("/rooms"));
    prof_process_xmpp(20);

    assert_true(stbbr_verify_last(
        "<iq id=\"confreq\" to=\"conference.localhost\" type=\"get\">"
            "<query xmlns=\"http://jabber.org/protocol/disco#items\"/>"
        "</iq>"
    ));
}

void
multiple_pings(void **state)
{
    will_return(ui_ask_password, strdup("password"));

    expect_any_cons_show();

    cmd_process_input(strdup("/connect stabber@localhost port 5230"));
    prof_process_xmpp(20);

    expect_cons_show("Pinged server...");
    expect_any_cons_show();
    expect_cons_show("Pinged server...");
    expect_any_cons_show();

    stbbr_for("prof_ping_1",
        "<iq id=\"prof_ping_1\" type=\"result\" to=\"stabber@localhost/profanity\"/>"
    );
    stbbr_for("prof_ping_2",
        "<iq id=\"prof_ping_2\" type=\"result\" to=\"stabber@localhost/profanity\"/>"
    );

    cmd_process_input(strdup("/ping"));
    prof_process_xmpp(20);
    cmd_process_input(strdup("/ping"));
    prof_process_xmpp(20);

    assert_true(stbbr_verify(
        "<iq id=\"prof_ping_1\" type=\"get\">"
            "<ping xmlns=\"urn:xmpp:ping\"/>"
        "</iq>"
    ));
    assert_true(stbbr_verify(
        "<iq id=\"prof_ping_2\" type=\"get\">"
            "<ping xmlns=\"urn:xmpp:ping\"/>"
        "</iq>"
    ));
}

void
responds_to_ping(void **state)
{
    will_return(ui_ask_password, strdup("password"));

    expect_any_cons_show();

    cmd_process_input(strdup("/connect stabber@localhost port 5230"));
    prof_process_xmpp(20);

    stbbr_send(
        "<iq id=\"ping1\" type=\"get\" to=\"stabber@localhost\" from=\"localhost\">"
            "<ping xmlns=\"urn:xmpp:ping\"/>"
        "</iq>");

    prof_process_xmpp(20);

    assert_true(stbbr_verify(
        "<iq id=\"ping1\" type=\"result\" from=\"stabber@localhost\" to=\"localhost\"/>"
    ));
}
