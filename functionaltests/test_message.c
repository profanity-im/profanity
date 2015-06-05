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
message_send(void **state)
{
    prof_connect("stabber@localhost", "password");

    prof_input("/msg somejid@someserver.com Hi there");

    assert_true(stbbr_received(
        "<message id=\"*\" to=\"somejid@someserver.com\" type=\"chat\">"
            "<body>Hi there</body>"
        "</message>"
    ));

    assert_true(prof_output_regex("me: .+Hi there"));
}

void
message_receive(void **state)
{
    stbbr_for_id("roster",
        "<iq id=\"roster\" type=\"result\" to=\"stabber@localhost/profanity\">"
            "<query xmlns=\"jabber:iq:roster\" ver=\"362\">"
                "<item jid=\"buddy1@localhost\" subscription=\"both\" name=\"Buddy1\"/>"
                "<item jid=\"buddy2@localhost\" subscription=\"both\" name=\"Buddy2\"/>"
            "</query>"
        "</iq>"
    );

    prof_connect("stabber@localhost", "password");
    stbbr_wait_for("prof_presence_1");

    stbbr_send(
        "<message id=\"message1\" to=\"stabber@localhost\" from=\"someuser@chatserv.org/laptop\" type=\"chat\">"
            "<body>How are you?</body>"
        "</message>"
    );

    assert_true(prof_output_exact("<< incoming from someuser@chatserv.org/laptop (2)"));
}
