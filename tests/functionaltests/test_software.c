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
send_software_version_request(void **state)
{
    prof_connect();
    stbbr_send(
        "<presence to=\"stabber@localhost\" from=\"buddy1@localhost/mobile\">"
            "<priority>10</priority>"
            "<status>I'm here</status>"
        "</presence>"
    );
    prof_output_exact("Buddy1 (mobile) is online, \"I'm here\"");
    prof_input("/software buddy1@localhost/mobile");

    stbbr_received(
        "<iq id=\"*\" to=\"buddy1@localhost/mobile\" type=\"get\">"
            "<query xmlns=\"jabber:iq:version\"/>"
        "</iq>"
    );
}

void
display_software_version_result(void **state)
{
    prof_connect();
    stbbr_send(
        "<presence to=\"stabber@localhost\" from=\"buddy1@localhost/mobile\">"
            "<priority>10</priority>"
            "<status>I'm here</status>"
        "</presence>"
    );
    prof_output_exact("Buddy1 (mobile) is online, \"I'm here\"");

    stbbr_for_query("jabber:iq:version",
        "<iq id=\"*\" type=\"result\" lang=\"en\" to=\"stabber@localhost/profanity\" from=\"buddy1@localhost/mobile\">"
            "<query xmlns=\"jabber:iq:version\">"
                "<name>Profanity</name>"
                "<version>0.4.7dev.master.2cb2f83</version>"
            "</query>"
        "</iq>"
    );
    prof_input("/software buddy1@localhost/mobile");

    prof_output_exact("buddy1@localhost/mobile:");
    prof_output_exact("Name    : Profanity");
    prof_output_exact("Version : 0.4.7dev.master.2cb2f83");
}
