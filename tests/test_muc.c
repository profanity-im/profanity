#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>
#include <stdlib.h>

#include "muc.h"

void test_muc_add_invite(void **state)
{
    char *room = "room@conf.server";
    muc_init();
    muc_add_invite(room);

    gboolean invite_exists = muc_invites_include(room);

    assert_true(invite_exists);

    muc_close();
}

void test_muc_remove_invite(void **state)
{
    char *room = "room@conf.server";
    muc_init();
    muc_add_invite(room);
    muc_remove_invite(room);

    gboolean invite_exists = muc_invites_include(room);

    assert_false(invite_exists);

    muc_close();
}
