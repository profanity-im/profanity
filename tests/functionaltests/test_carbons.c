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
        "<iq id=\"*\" type=\"set\"><enable xmlns=\"urn:xmpp:carbons:2\"/></iq>"
    ));
}

void
connect_with_carbons_enabled(void **state)
{
    prof_input("/carbons on");

    prof_connect();

    assert_true(stbbr_received(
        "<iq id=\"*\" type=\"set\"><enable xmlns=\"urn:xmpp:carbons:2\"/></iq>"
    ));
}

void
send_disable_carbons(void **state)
{
    prof_input("/carbons on");

    prof_connect();

    prof_input("/carbons off");

    assert_true(stbbr_received(
        "<iq id=\"*\" type=\"set\"><disable xmlns=\"urn:xmpp:carbons:2\"/></iq>"
    ));
}
