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
ping_multiple(void **state)
{
    stbbr_for("prof_ping_1",
        "<iq id=\"prof_ping_1\" type=\"result\" to=\"stabber@localhost/profanity\"/>"
    );
    stbbr_for("prof_ping_2",
        "<iq id=\"prof_ping_2\" type=\"result\" to=\"stabber@localhost/profanity\"/>"
    );

    prof_connect("stabber@localhost", "password");

    prof_input("/ping");
    assert_true(stbbr_received(
        "<iq id=\"prof_ping_1\" type=\"get\">"
            "<ping xmlns=\"urn:xmpp:ping\"/>"
        "</iq>"
    ));
    assert_true(prof_output_exact("Ping response from server"));

    prof_input("/ping");
    assert_true(stbbr_received(
        "<iq id=\"prof_ping_2\" type=\"get\">"
            "<ping xmlns=\"urn:xmpp:ping\"/>"
        "</iq>"
    ));
    assert_true(prof_output_exact("Ping response from server"));
}

void
ping_responds(void **state)
{
    prof_connect("stabber@localhost", "password");

    assert_true(prof_output_exact("stabber@localhost logged in successfully"));

    stbbr_send(
        "<iq id=\"pingtest1\" type=\"get\" to=\"stabber@localhost/profanity\" from=\"localhost\">"
            "<ping xmlns=\"urn:xmpp:ping\"/>"
        "</iq>"
    );

    assert_true(stbbr_received(
        "<iq id=\"pingtest1\" type=\"result\" from=\"stabber@localhost/profanity\" to=\"localhost\"/>"
    ));
}
