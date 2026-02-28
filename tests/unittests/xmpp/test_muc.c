#include "prof_cmocka.h"
#include <stdlib.h>

#include "xmpp/muc.h"

void prof_shutdown(void);

int
muc_before_test(void** state)
{
    muc_init();
    return 0;
}

int
muc_after_test(void** state)
{
    prof_shutdown();
    return 0;
}

void
muc_invites_add__updates__invites_list(void** state)
{
    char* room = "room@conf.server";
    muc_invites_add(room, NULL);

    gboolean invite_exists = muc_invites_contain(room);

    assert_true(invite_exists);
}

void
muc_invites_remove__updates__invites_list(void** state)
{
    char* room = "room@conf.server";
    muc_invites_add(room, NULL);
    muc_invites_remove(room);

    gboolean invite_exists = muc_invites_contain(room);

    assert_false(invite_exists);
}

void
muc_invites_count__returns__0_when_no_invites(void** state)
{
    int invite_count = muc_invites_count();

    assert_true(invite_count == 0);
}

void
muc_invites_count__returns__5_when_five_invites_added(void** state)
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
muc_active__is__false_when_not_joined(void** state)
{
    char* room = "room@server.org";

    gboolean room_is_active = muc_active(room);

    assert_false(room_is_active);
}

void
muc_active__is__true_when_joined(void** state)
{
    char* room = "room@server.org";
    char* nick = "bob";
    muc_join(room, nick, NULL, FALSE);

    gboolean room_is_active = muc_active(room);

    assert_true(room_is_active);
}
