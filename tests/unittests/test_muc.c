#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>
#include <stdlib.h>

#include "xmpp/muc.h"

int
muc_before_test(void** state)
{
    muc_init();
    return 0;
}

int
muc_after_test(void** state)
{
    return 0;
}

void
test_muc_invites_add(void** state)
{
    char* room = "room@conf.server";
    muc_invites_add(room, NULL);

    gboolean invite_exists = muc_invites_contain(room);

    assert_true(invite_exists);
}

void
test_muc_remove_invite(void** state)
{
    char* room = "room@conf.server";
    muc_invites_add(room, NULL);
    muc_invites_remove(room);

    gboolean invite_exists = muc_invites_contain(room);

    assert_false(invite_exists);
}

void
test_muc_invites_count_0(void** state)
{
    int invite_count = muc_invites_count();

    assert_true(invite_count == 0);
}

void
test_muc_invites_count_5(void** state)
{
    muc_invites_add("room1@conf.server", NULL);
    muc_invites_add("room2@conf.server", NULL);
    muc_invites_add("room3@conf.server", NULL);
    muc_invites_add("room4@conf.server", NULL);
    muc_invites_add("room5@conf.server", NULL);

    int invite_count = muc_invites_count();

    assert_true(invite_count == 5);
}

void
test_muc_room_is_not_active(void** state)
{
    char* room = "room@server.org";

    gboolean room_is_active = muc_active(room);

    assert_false(room_is_active);
}

void
test_muc_active(void** state)
{
    char* room = "room@server.org";
    char* nick = "bob";
    muc_join(room, nick, NULL, FALSE);

    gboolean room_is_active = muc_active(room);

    assert_true(room_is_active);
}
