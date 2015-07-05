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
