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
    stbbr_for_id("prof_disco_info_onconnect_2",
        "<iq id='prof_disco_info_onconnect_2' to='stabber@localhost/profanity' type='result' from='localhost'>"
            "<query xmlns='http://jabber.org/protocol/disco#info'>"
                "<identity category='server' type='im' name='Prosody'/>"
                "<feature var='urn:xmpp:ping'/>"
            "</query>"
        "</iq>"
    );

    stbbr_for_id("prof_ping_4",
        "<iq id='prof_ping_4' type='result' to='stabber@localhost/profanity'/>"
    );
    stbbr_for_id("prof_ping_5",
        "<iq id='prof_ping_5' type='result' to='stabber@localhost/profanity'/>"
    );

    prof_connect();

    prof_input("/ping");
    assert_true(stbbr_received(
        "<iq id='prof_ping_4' type='get'>"
            "<ping xmlns='urn:xmpp:ping'/>"
        "</iq>"
    ));
    assert_true(prof_output_exact("Ping response from server"));

    prof_input("/ping");
    assert_true(stbbr_received(
        "<iq id='prof_ping_5' type='get'>"
            "<ping xmlns='urn:xmpp:ping'/>"
        "</iq>"
    ));
    assert_true(prof_output_exact("Ping response from server"));
}

void
ping_not_supported(void **state)
{
    stbbr_for_id("prof_disco_info_onconnect_2",
        "<iq id='prof_disco_info_onconnect_2' to='stabber@localhost/profanity' type='result' from='localhost'>"
            "<query xmlns='http://jabber.org/protocol/disco#info'>"
                "<identity category='server' type='im' name='Stabber'/>"
            "</query>"
        "</iq>"
    );

    prof_connect();

    prof_input("/ping");
    assert_true(prof_output_exact("Server does not support ping requests."));
}

void
ping_responds(void **state)
{
    prof_connect();

    stbbr_send(
        "<iq id='pingtest1' type='get' to='stabber@localhost/profanity' from='localhost'>"
            "<ping xmlns='urn:xmpp:ping'/>"
        "</iq>"
    );

    assert_true(stbbr_received(
        "<iq id='pingtest1' type='result' from='stabber@localhost/profanity' to='localhost'/>"
    ));
}
