#include <stdlib.h>
#include <string.h>
#include <head-unit.h>
#include "jid.h"

void create_jid_from_null_returns_null(void)
{
    Jid *result = jid_create(NULL);
    assert_is_null(result);
}

void create_jid_from_empty_string_returns_null(void)
{
    Jid *result = jid_create("");
    assert_is_null(result);
}

void create_jid_from_full_returns_full(void)
{
    Jid *result = jid_create("myuser@mydomain/laptop");
    assert_string_equals("myuser@mydomain/laptop", result->fulljid);
}

void create_jid_from_full_returns_bare(void)
{
    Jid *result = jid_create("myuser@mydomain/laptop");
    assert_string_equals("myuser@mydomain", result->barejid);
}

void create_jid_from_full_returns_resourcepart(void)
{
    Jid *result = jid_create("myuser@mydomain/laptop");
    assert_string_equals("laptop", result->resourcepart);
}

void create_jid_from_full_returns_localpart(void)
{
    Jid *result = jid_create("myuser@mydomain/laptop");
    assert_string_equals("myuser", result->localpart);
}

void create_jid_from_full_returns_domainpart(void)
{
    Jid *result = jid_create("myuser@mydomain/laptop");
    assert_string_equals("mydomain", result->domainpart);
}

void create_jid_from_full_nolocal_returns_full(void)
{
    Jid *result = jid_create("mydomain/laptop");
    assert_string_equals("mydomain/laptop", result->fulljid);
}

void create_jid_from_full_nolocal_returns_bare(void)
{
    Jid *result = jid_create("mydomain/laptop");
    assert_string_equals("mydomain", result->barejid);
}

void create_jid_from_full_nolocal_returns_resourcepart(void)
{
    Jid *result = jid_create("mydomain/laptop");
    assert_string_equals("laptop", result->resourcepart);
}

void create_jid_from_full_nolocal_returns_domainpart(void)
{
    Jid *result = jid_create("mydomain/laptop");
    assert_string_equals("mydomain", result->domainpart);
}

void create_jid_from_full_nolocal_returns_null_localpart(void)
{
    Jid *result = jid_create("mydomain/laptop");
    assert_is_null(result->localpart);
}

void create_jid_from_bare_returns_null_full(void)
{
    Jid *result = jid_create("myuser@mydomain");
    assert_is_null(result->fulljid);
}

void create_jid_from_bare_returns_null_resource(void)
{
    Jid *result = jid_create("myuser@mydomain");
    assert_is_null(result->resourcepart);
}

void create_jid_from_bare_returns_bare(void)
{
    Jid *result = jid_create("myuser@mydomain");
    assert_string_equals("myuser@mydomain", result->barejid);
}

void create_jid_from_bare_returns_localpart(void)
{
    Jid *result = jid_create("myuser@mydomain");
    assert_string_equals("myuser", result->localpart);
}

void create_jid_from_bare_returns_domainpart(void)
{
    Jid *result = jid_create("myuser@mydomain");
    assert_string_equals("mydomain", result->domainpart);
}

void create_room_jid_returns_room(void)
{
    Jid *result = jid_create_from_bare_and_resource("room@conference.domain.org", "myname");

    assert_string_equals("room@conference.domain.org", result->barejid);
}

void create_room_jid_returns_nick(void)
{
    Jid *result = jid_create_from_bare_and_resource("room@conference.domain.org", "myname");

    assert_string_equals("myname", result->resourcepart);
}

void create_with_slash_in_resource(void)
{
    Jid *result = jid_create("room@conference.domain.org/my/nick");

    assert_string_equals("room", result->localpart);
    assert_string_equals("conference.domain.org", result->domainpart);
    assert_string_equals("my/nick", result->resourcepart);
    assert_string_equals("room@conference.domain.org", result->barejid);
    assert_string_equals("room@conference.domain.org/my/nick", result->fulljid);
}

void create_with_at_in_resource(void)
{
    Jid *result = jid_create("room@conference.domain.org/my@nick");

    assert_string_equals("room", result->localpart);
    assert_string_equals("conference.domain.org", result->domainpart);
    assert_string_equals("my@nick", result->resourcepart);
    assert_string_equals("room@conference.domain.org", result->barejid);
    assert_string_equals("room@conference.domain.org/my@nick", result->fulljid);
}

void create_with_at_and_slash_in_resource(void)
{
    Jid *result = jid_create("room@conference.domain.org/my@nick/something");

    assert_string_equals("room", result->localpart);
    assert_string_equals("conference.domain.org", result->domainpart);
    assert_string_equals("my@nick/something", result->resourcepart);
    assert_string_equals("room@conference.domain.org", result->barejid);
    assert_string_equals("room@conference.domain.org/my@nick/something", result->fulljid);
}

void create_full_with_trailing_slash(void)
{
    Jid *result = jid_create("room@conference.domain.org/nick/");

    assert_string_equals("room", result->localpart);
    assert_string_equals("conference.domain.org", result->domainpart);
    assert_string_equals("nick/", result->resourcepart);
    assert_string_equals("room@conference.domain.org", result->barejid);
    assert_string_equals("room@conference.domain.org/nick/", result->fulljid);
}

void register_jid_tests(void)
{
    TEST_MODULE("jid tests");
    TEST(create_jid_from_null_returns_null);
    TEST(create_jid_from_empty_string_returns_null);
    TEST(create_jid_from_full_returns_full);
    TEST(create_jid_from_full_returns_bare);
    TEST(create_jid_from_full_returns_resourcepart);
    TEST(create_jid_from_full_returns_localpart);
    TEST(create_jid_from_full_returns_domainpart);
    TEST(create_jid_from_full_nolocal_returns_full);
    TEST(create_jid_from_full_nolocal_returns_bare);
    TEST(create_jid_from_full_nolocal_returns_resourcepart);
    TEST(create_jid_from_full_nolocal_returns_domainpart);
    TEST(create_jid_from_full_nolocal_returns_null_localpart);
    TEST(create_jid_from_bare_returns_null_full);
    TEST(create_jid_from_bare_returns_null_resource);
    TEST(create_jid_from_bare_returns_bare);
    TEST(create_jid_from_bare_returns_localpart);
    TEST(create_jid_from_bare_returns_domainpart);
    TEST(create_room_jid_returns_room);
    TEST(create_room_jid_returns_nick);
    TEST(create_with_slash_in_resource);
    TEST(create_with_at_in_resource);
    TEST(create_with_at_and_slash_in_resource);
    TEST(create_full_with_trailing_slash);
}
