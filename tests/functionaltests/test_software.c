#include <glib.h>
#include "prof_cmocka.h"
#include <stdlib.h>
#include <string.h>

#include <stabber.h>
#include <expect.h>

#include "proftest.h"

void
send_software_version_request(void** state)
{
    prof_connect();
    stbbr_send(
        "<presence to='stabber@localhost' from='buddy1@localhost/mobile'>"
        "<priority>10</priority>"
        "<status>I'm here</status>"
        "</presence>");
    assert_true(prof_output_exact("Buddy1 (mobile) is online, \"I'm here\""));
    prof_input("/software buddy1@localhost/mobile");

    assert_true(stbbr_received(
        "<iq id='*' to='buddy1@localhost/mobile' type='get'>"
        "<query xmlns='jabber:iq:version'/>"
        "</iq>"));
}

void
display_software_version_result(void** state)
{
    prof_connect();
    stbbr_send(
        "<presence to='stabber@localhost' from='buddy1@localhost/mobile'>"
        "<priority>10</priority>"
        "<status>I'm here</status>"
        "</presence>");
    assert_true(prof_output_exact("Buddy1 (mobile) is online, \"I'm here\""));

    stbbr_for_query("jabber:iq:version",
                    "<iq id='*' type='result' lang='en' to='stabber@localhost/profanity' from='buddy1@localhost/mobile'>"
                    "<query xmlns='jabber:iq:version'>"
                    "<name>Profanity</name>"
                    "<version>0.4.7dev.master.2cb2f83</version>"
                    "</query>"
                    "</iq>");
    prof_input("/software buddy1@localhost/mobile");

    //    assert_true(prof_output_exact("buddy1@localhost/mobile:"));
    //    assert_true(prof_output_exact("Name    : Profanity"));
    assert_true(prof_output_exact("Version : 0.4.7dev.master.2cb2f83"));
}

void
shows_message_when_software_version_error(void** state)
{
    prof_connect();
    stbbr_send(
        "<presence to='stabber@localhost' from='buddy1@localhost/mobile'>"
        "<priority>10</priority>"
        "<status>I'm here</status>"
        "</presence>");
    assert_true(prof_output_exact("Buddy1 (mobile) is online, \"I'm here\""));

    stbbr_for_query("jabber:iq:version",
                    "<iq id='*' lang='en' type='error' to='stabber@localhost/profanity' from='buddy1@localhost/laptop'>"
                    "<query xmlns='jabber:iq:version'/>"
                    "<error code='503' type='cancel'>"
                    "<service-unavailable xmlns='urn:ietf:params:xml:ns:xmpp-stanzas'/>"
                    "</error>"
                    "</iq>");
    prof_input("/software buddy1@localhost/laptop");

    assert_true(prof_output_exact("Could not get software version: service-unavailable"));
}

// Typical use case for gateways that don't support resources
void
display_software_version_result_when_from_domainpart(void** state)
{
    prof_connect();
    stbbr_send(
        "<presence to='stabber@localhost' from='buddy1@localhost'>"
        "<priority>10</priority>"
        "<status>I'm here</status>"
        "</presence>");
    assert_true(prof_output_exact("Buddy1 is online, \"I'm here\""));

    stbbr_for_query("jabber:iq:version",
                    "<iq id='*' type='result' lang='en' to='stabber@localhost/profanity' from='localhost'>"
                    "<query xmlns='jabber:iq:version'>"
                    "<name>Some Gateway</name>"
                    "<version>1.0</version>"
                    "</query>"
                    "</iq>");
    prof_input("/software buddy1@localhost/__prof_default");

    //    assert_true(prof_output_exact("buddy1@localhost/__prof_default:"));
    //    assert_true(prof_output_exact("Name    : Some Gateway"));
    assert_true(prof_output_exact("Version : 1.0"));
}

void
show_message_in_chat_window_when_no_resource(void** state)
{
    prof_connect();
    stbbr_send(
        "<presence to='stabber@localhost' from='buddy1@localhost/mobile'>"
        "<priority>10</priority>"
        "<status>I'm here</status>"
        "</presence>");
    assert_true(prof_output_exact("Buddy1 (mobile) is online, \"I'm here\""));

    prof_input("/msg Buddy1");
    prof_input("/software");

    assert_true(prof_output_exact("Unknown resource for /software command."));
}

void
display_software_version_result_in_chat(void** state)
{
    prof_connect();
    stbbr_send(
        "<presence to='stabber@localhost' from='buddy1@localhost/mobile'>"
        "<priority>10</priority>"
        "<status>I'm here</status>"
        "</presence>");
    assert_true(prof_output_exact("Buddy1 (mobile) is online, \"I'm here\""));
    prof_input("/msg Buddy1");

    stbbr_send(
        "<message id='message1' to='stabber@localhost' from='buddy1@localhost/mobile' type='chat'>"
        "<body>Here's a message</body>"
        "</message>");
    assert_true(prof_output_exact("Here's a message"));

    stbbr_for_query("jabber:iq:version",
                    "<iq id='*' type='result' lang='en' to='stabber@localhost/profanity' from='buddy1@localhost/mobile'>"
                    "<query xmlns='jabber:iq:version'>"
                    "<name>Profanity</name>"
                    "<version>0.4.7dev.master.2cb2f83</version>"
                    "</query>"
                    "</iq>");

    prof_input("/software");

    //    assert_true(prof_output_exact("buddy1@localhost/mobile:"));
    //    assert_true(prof_output_exact("Name    : Profanity"));
    assert_true(prof_output_exact("Version : 0.4.7dev.master.2cb2f83"));
}
