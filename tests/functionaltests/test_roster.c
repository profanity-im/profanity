#include <glib.h>
#include "prof_cmocka.h"
#include <stdlib.h>
#include <string.h>

#include <stabber.h>
#include <expect.h>

#include "proftest.h"

void
sends_new_item(void** state)
{
    prof_connect();

    stbbr_for_query("jabber:iq:roster",
                    "<iq type='set' from='stabber@localhost'>"
                    "<query xmlns='jabber:iq:roster'>"
                    "<item jid='bob@localhost' subscription='none' name=''/>"
                    "</query>"
                    "</iq>");

    prof_input("/roster add bob@localhost");

    assert_true(stbbr_received(
        "<iq type='set' id='*'>"
        "<query xmlns='jabber:iq:roster'>"
        "<item jid='bob@localhost' name=''/>"
        "</query>"
        "</iq>"));

    assert_true(prof_output_exact("Roster item added: bob@localhost"));
}

void
sends_new_item_nick(void** state)
{
    prof_connect();

    stbbr_for_query("jabber:iq:roster",
                    "<iq type='set' from='stabber@localhost'>"
                    "<query xmlns='jabber:iq:roster'>"
                    "<item jid='bob@localhost' subscription='none' name='Bobby'/>"
                    "</query>"
                    "</iq>");

    prof_input("/roster add bob@localhost Bobby");

    assert_true(stbbr_received(
        "<iq type='set' id='*'>"
        "<query xmlns='jabber:iq:roster'>"
        "<item jid='bob@localhost' name='Bobby'/>"
        "</query>"
        "</iq>"));

    assert_true(prof_output_exact("Roster item added: bob@localhost (Bobby)"));
}

void
sends_remove_item(void** state)
{
    prof_connect_with_roster(
        "<item jid='buddy1@localhost' subscription='both'/>"
        "<item jid='buddy2@localhost' subscription='both'/>");

    stbbr_for_query("jabber:iq:roster",
                    "<iq id='*' type='set'>"
                    "<query xmlns='jabber:iq:roster'>"
                    "<item jid='buddy1@localhost' subscription='remove'/>"
                    "</query>"
                    "</iq>");

    prof_input("/roster remove buddy1@localhost");

    assert_true(stbbr_received(
        "<iq type='set' id='*'>"
        "<query xmlns='jabber:iq:roster'>"
        "<item jid='buddy1@localhost' subscription='remove'/>"
        "</query>"
        "</iq>"));

    assert_true(prof_output_exact("Roster item removed: buddy1@localhost"));
}

void
sends_remove_item_nick(void** state)
{
    prof_connect_with_roster(
        "<item jid='buddy1@localhost' name='Bobby' subscription='both'/>"
        "<item jid='buddy2@localhost' subscription='both'/>");

    stbbr_for_query("jabber:iq:roster",
                    "<iq id='*' type='set'>"
                    "<query xmlns='jabber:iq:roster'>"
                    "<item jid='buddy1@localhost' subscription='remove'/>"
                    "</query>"
                    "</iq>");

    prof_input("/roster remove Bobby");

    assert_true(stbbr_received(
        "<iq type='set' id='*'>"
        "<query xmlns='jabber:iq:roster'>"
        "<item jid='buddy1@localhost' subscription='remove'/>"
        "</query>"
        "</iq>"));

    assert_true(prof_output_exact("Roster item removed: buddy1@localhost"));
}

void
sends_nick_change(void** state)
{
    prof_connect_with_roster(
        "<item jid='buddy1@localhost' subscription='both'/>");

    prof_input("/roster nick buddy1@localhost Buddy1");

    assert_true(prof_output_exact("Nickname for buddy1@localhost set to: Buddy1."));

    assert_true(stbbr_received(
        "<iq type='set' id='*'>"
        "<query xmlns='jabber:iq:roster'>"
        "<item jid='buddy1@localhost' name='Buddy1'/>"
        "</query>"
        "</iq>"));
}
