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
disconnect_ends_session(void **state)
{
    prof_connect();

    prof_input("/disconnect");
    assert_true(prof_output_exact("stabber@localhost logged out successfully."));

    prof_input("/roster");
    assert_true(prof_output_exact("You are not currently connected."));
}
