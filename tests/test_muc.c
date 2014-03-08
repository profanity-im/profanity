#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>
#include <stdlib.h>

#include "muc.h"

void muc_before_test(void **state)
{
    muc_init();
}

void muc_after_test(void **state)
{
    muc_close();
}

void test_muc_add_invite(void **state)
{
    char *room = "room@conf.server";
    muc_add_invite(room);

    gboolean invite_exists = muc_invites_include(room);

    assert_true(invite_exists);
}

void test_muc_remove_invite(void **state)
{
    char *room = "room@conf.server";
    muc_add_invite(room);
    muc_remove_invite(room);

    gboolean invite_exists = muc_invites_include(room);

    assert_false(invite_exists);
}

void test_muc_invite_count_0(void **state)
{
    int invite_count = muc_invite_count();

    assert_true(invite_count == 0);
}

void test_muc_invite_count_5(void **state)
{
    muc_add_invite("room1@conf.server");
    muc_add_invite("room2@conf.server");
    muc_add_invite("room3@conf.server");
    muc_add_invite("room4@conf.server");
    muc_add_invite("room5@conf.server");

    int invite_count = muc_invite_count();

    assert_true(invite_count == 5);
}

void test_muc_room_is_not_active(void **state)
{
    char *room = "room@server.org";

    gboolean room_is_active = muc_room_is_active(room);

    assert_false(room_is_active);
}

void test_muc_room_is_active(void **state)
{
    char *room = "room@server.org";
    char *nick = "bob";
    muc_join_room(room, nick);

    gboolean room_is_active = muc_room_is_active(room);

    assert_true(room_is_active);
}
