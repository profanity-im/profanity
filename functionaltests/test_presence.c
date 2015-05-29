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
presence_away(void **state)
{
    prof_connect("stabber@localhost", "password");

    prof_input("/away");

    assert_true(stbbr_received(
    "<presence id=\"*\">"
        "<show>away</show>"
        "<c hash=\"sha-1\" xmlns=\"http://jabber.org/protocol/caps\" ver=\"*\" node=\"http://www.profanity.im\"/>"
    "</presence>"
    ));

    assert_true(prof_output_exact("Status set to away (priority 0)"));
}

void
presence_away_with_message(void **state)
{
    prof_connect("stabber@localhost", "password");

    prof_input("/away \"I'm not here for a bit\"");

    assert_true(stbbr_received(
    "<presence id=\"*\">"
        "<show>away</show>"
        "<status>I'm not here for a bit</status>"
        "<c hash=\"sha-1\" xmlns=\"http://jabber.org/protocol/caps\" ver=\"*\" node=\"http://www.profanity.im\"/>"
    "</presence>"
    ));

    assert_true(prof_output_exact("Status set to away (priority 0), \"I'm not here for a bit\"."));
}
