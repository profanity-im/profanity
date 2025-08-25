#include <glib.h>
#include "prof_cmocka.h"
#include <stdlib.h>
#include <string.h>

#include <stabber.h>
#include <expect.h>

#include "proftest.h"

void
does_not_send_receipt_request_to_barejid(void **state)
{
    prof_input("/receipts request on");

    prof_connect();

    prof_input("/msg somejid@someserver.com Hi there");

    assert_true(stbbr_received(
        "<message id='*' type='chat' to='somejid@someserver.com'>"
            "<body>Hi there</body>"
        "</message>"
    ));
}

void
send_receipt_request(void **state)
{
    prof_input("/receipts request on");

    prof_connect();

    stbbr_for_id("prof_caps_4",
        "<iq from='buddy1@localhost/laptop' to='stabber@localhost' id='prof_caps_4' type='result'>"
            "<query xmlns='http://jabber.org/protocol/disco#info' node='http://profanity-im.github.io#hAkb1xZdJV9BQpgGNw8zG5Xsals='>"
                "<identity category='client' name='Profanity 0.5.0' type='console'/>"
                "<feature var='urn:xmpp:receipts'/>"
            "</query>"
        "</iq>"
    );

    stbbr_send(
        "<presence to='stabber@localhost' from='buddy1@localhost/laptop'>"
            "<priority>15</priority>"
            "<status>My status</status>"
            "<c hash='sha-256' xmlns='http://jabber.org/protocol/caps' node='http://profanity-im.github.io' ver='hAkb1xZdJV9BQpgGNw8zG5Xsals='/>"
        "</presence>"
    );

    prof_output_exact("Buddy1 is online, \"My status\"");

    prof_input("/msg Buddy1");
    prof_input("/resource set laptop");
    prof_input("Hi there, where is my receipt?");

    assert_true(stbbr_received(
        "<message id='*' type='chat' to='buddy1@localhost/laptop'>"
            "<body>Hi there, where is my receipt?</body>"
            "<request xmlns='urn:xmpp:receipts'/>"
        "</message>"
    ));
}

void
send_receipt_on_request(void **state)
{
    prof_input("/receipts send on");

    prof_connect();

    stbbr_send(
        "<message id='msg12213' type='chat' to='stabber@localhost/profanity' from='someuser@server.org/profanity'>"
            "<body>Wants a receipt</body>"
            "<request xmlns='urn:xmpp:receipts'/>"
        "</message>"
    );

    assert_true(stbbr_received(
        "<message id='*' to='someuser@server.org/profanity'>"
            "<received id='msg12213' xmlns='urn:xmpp:receipts'/>"
        "</message>"
    ));
}
