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
send_receipt_request(void **state)
{
    prof_input("/receipts request on");

    prof_connect();

    prof_input("/msg somejid@someserver.com Hi there");

    assert_true(stbbr_received(
        "<message id=\"*\" type=\"chat\" to=\"somejid@someserver.com\">"
            "<body>Hi there</body>"
            "<request xmlns=\"urn:xmpp:receipts\"/>"
        "</message>"
    ));
}
