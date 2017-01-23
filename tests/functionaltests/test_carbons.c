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
send_enable_carbons(void **state)
{
    prof_connect();

    prof_input("/carbons on");

    assert_true(stbbr_received(
        "<iq id='*' type='set'><enable xmlns='urn:xmpp:carbons:2'/></iq>"
    ));
}

void
connect_with_carbons_enabled(void **state)
{
    prof_input("/carbons on");

    prof_connect();

    assert_true(stbbr_received(
        "<iq id='*' type='set'><enable xmlns='urn:xmpp:carbons:2'/></iq>"
    ));
}

void
send_disable_carbons(void **state)
{
    prof_input("/carbons on");

    prof_connect();

    prof_input("/carbons off");

    assert_true(stbbr_received(
        "<iq id='*' type='set'><disable xmlns='urn:xmpp:carbons:2'/></iq>"
    ));
}

void
receive_carbon(void **state)
{
    prof_input("/carbons on");

    prof_connect();
    assert_true(stbbr_received(
        "<iq id='*' type='set'><enable xmlns='urn:xmpp:carbons:2'/></iq>"
    ));

    stbbr_send(
        "<presence to='stabber@localhost' from='buddy1@localhost/mobile'>"
            "<priority>10</priority>"
            "<status>On my mobile</status>"
        "</presence>"
    );
    assert_true(prof_output_exact("Buddy1 (mobile) is online, \"On my mobile\""));
    prof_input("/msg Buddy1");
    assert_true(prof_output_exact("unencrypted"));

    stbbr_send(
        "<message type='chat' to='stabber@localhost/profanity' from='stabber@localhost'>"
            "<received xmlns='urn:xmpp:carbons:2'>"
                "<forwarded xmlns='urn:xmpp:forward:0'>"
                    "<message id='prof_msg_7' xmlns='jabber:client' type='chat' lang='en' to='stabber@localhost/profanity' from='buddy1@localhost/mobile'>"
                        "<body>test carbon from recipient</body>"
                    "</message>"
                "</forwarded>"
            "</received>"
        "</message>"
    );

    assert_true(prof_output_regex("Buddy1/mobile: .+test carbon from recipient"));
}

void
receive_self_carbon(void **state)
{
    prof_input("/carbons on");

    prof_connect();
    assert_true(stbbr_received(
        "<iq id='*' type='set'><enable xmlns='urn:xmpp:carbons:2'/></iq>"
    ));

    stbbr_send(
        "<presence to='stabber@localhost' from='buddy1@localhost/mobile'>"
            "<priority>10</priority>"
            "<status>On my mobile</status>"
        "</presence>"
    );
    assert_true(prof_output_exact("Buddy1 (mobile) is online, \"On my mobile\""));
    prof_input("/msg Buddy1");
    assert_true(prof_output_exact("unencrypted"));

    stbbr_send(
        "<message type='chat' to='stabber@localhost/profanity' from='stabber@localhost'>"
            "<sent xmlns='urn:xmpp:carbons:2'>"
                "<forwarded xmlns='urn:xmpp:forward:0'>"
                    "<message id='59' xmlns='jabber:client' type='chat' to='buddy1@localhost/mobile' lang='en' from='stabber@localhost/profanity'>"
                        "<body>self sent carbon</body>"
                    "</message>"
                "</forwarded>"
            "</sent>"
        "</message>"
    );

    assert_true(prof_output_regex("me: .+self sent carbon"));
}

void
receive_private_carbon(void **state)
{
    prof_input("/carbons on");

    prof_connect();
    assert_true(stbbr_received(
        "<iq id='*' type='set'><enable xmlns='urn:xmpp:carbons:2'/></iq>"
    ));

    stbbr_send(
        "<presence to='stabber@localhost' from='buddy1@localhost/mobile'>"
            "<priority>10</priority>"
            "<status>On my mobile</status>"
        "</presence>"
    );
    assert_true(prof_output_exact("Buddy1 (mobile) is online, \"On my mobile\""));
    prof_input("/msg Buddy1");
    assert_true(prof_output_exact("unencrypted"));

    stbbr_send(
        "<message type='chat' to='stabber@localhost/profanity' from='buddy1@localhost/mobile'>"
            "<body>Private carbon</body>"
            "<private xmlns='urn:xmpp:carbons:2'/>"
        "</message>"
    );

    assert_true(prof_output_regex("Buddy1/mobile: .+Private carbon"));
}
