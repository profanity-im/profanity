#include <cmocka.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdlib.h>

#include "xmpp/jid.h"

void
create_jid_from_null_returns_null(void** state)
{
    Jid* result = jid_create(NULL);
    assert_null(result);
}

void
create_jid_from_empty_string_returns_null(void** state)
{
    Jid* result = jid_create("");
    assert_null(result);
}

void
create_jid_from_full_returns_full(void** state)
{
    Jid* result = jid_create("myuser@mydomain/laptop");
    assert_string_equal("myuser@mydomain/laptop", result->fulljid);
    jid_destroy(result);
}

void
create_jid_from_full_returns_bare(void** state)
{
    Jid* result = jid_create("myuser@mydomain/laptop");
    assert_string_equal("myuser@mydomain", result->barejid);
    jid_destroy(result);
}

void
create_jid_from_full_returns_resourcepart(void** state)
{
    Jid* result = jid_create("myuser@mydomain/laptop");
    assert_string_equal("laptop", result->resourcepart);
    jid_destroy(result);
}

void
create_jid_from_full_returns_localpart(void** state)
{
    Jid* result = jid_create("myuser@mydomain/laptop");
    assert_string_equal("myuser", result->localpart);
    jid_destroy(result);
}

void
create_jid_from_full_returns_domainpart(void** state)
{
    Jid* result = jid_create("myuser@mydomain/laptop");
    assert_string_equal("mydomain", result->domainpart);
    jid_destroy(result);
}

void
create_jid_from_full_nolocal_returns_full(void** state)
{
    Jid* result = jid_create("mydomain/laptop");
    assert_string_equal("mydomain/laptop", result->fulljid);
    jid_destroy(result);
}

void
create_jid_from_full_nolocal_returns_bare(void** state)
{
    Jid* result = jid_create("mydomain/laptop");
    assert_string_equal("mydomain", result->barejid);
    jid_destroy(result);
}

void
create_jid_from_full_nolocal_returns_resourcepart(void** state)
{
    Jid* result = jid_create("mydomain/laptop");
    assert_string_equal("laptop", result->resourcepart);
    jid_destroy(result);
}

void
create_jid_from_full_nolocal_returns_domainpart(void** state)
{
    Jid* result = jid_create("mydomain/laptop");
    assert_string_equal("mydomain", result->domainpart);
    jid_destroy(result);
}

void
create_jid_from_full_nolocal_returns_null_localpart(void** state)
{
    Jid* result = jid_create("mydomain/laptop");
    assert_null(result->localpart);
    jid_destroy(result);
}

void
create_jid_from_bare_returns_null_full(void** state)
{
    Jid* result = jid_create("myuser@mydomain");
    assert_null(result->fulljid);
    jid_destroy(result);
}

void
create_jid_from_bare_returns_null_resource(void** state)
{
    Jid* result = jid_create("myuser@mydomain");
    assert_null(result->resourcepart);
    jid_destroy(result);
}

void
create_jid_from_bare_returns_bare(void** state)
{
    Jid* result = jid_create("myuser@mydomain");
    assert_string_equal("myuser@mydomain", result->barejid);
    jid_destroy(result);
}

void
create_jid_from_bare_returns_localpart(void** state)
{
    Jid* result = jid_create("myuser@mydomain");
    assert_string_equal("myuser", result->localpart);
    jid_destroy(result);
}

void
create_jid_from_bare_returns_domainpart(void** state)
{
    Jid* result = jid_create("myuser@mydomain");
    assert_string_equal("mydomain", result->domainpart);
    jid_destroy(result);
}

void
create_room_jid_returns_room(void** state)
{
    Jid* result = jid_create_from_bare_and_resource("room@conference.domain.org", "myname");

    assert_string_equal("room@conference.domain.org", result->barejid);
    jid_destroy(result);
}

void
create_room_jid_returns_nick(void** state)
{
    Jid* result = jid_create_from_bare_and_resource("room@conference.domain.org", "myname");

    assert_string_equal("myname", result->resourcepart);
    jid_destroy(result);
}

void
create_with_slash_in_resource(void** state)
{
    Jid* result = jid_create("room@conference.domain.org/my/nick");

    assert_string_equal("room", result->localpart);
    assert_string_equal("conference.domain.org", result->domainpart);
    assert_string_equal("my/nick", result->resourcepart);
    assert_string_equal("room@conference.domain.org", result->barejid);
    assert_string_equal("room@conference.domain.org/my/nick", result->fulljid);

    jid_destroy(result);
}

void
create_with_at_in_resource(void** state)
{
    Jid* result = jid_create("room@conference.domain.org/my@nick");

    assert_string_equal("room", result->localpart);
    assert_string_equal("conference.domain.org", result->domainpart);
    assert_string_equal("my@nick", result->resourcepart);
    assert_string_equal("room@conference.domain.org", result->barejid);
    assert_string_equal("room@conference.domain.org/my@nick", result->fulljid);

    jid_destroy(result);
}

void
create_with_at_and_slash_in_resource(void** state)
{
    Jid* result = jid_create("room@conference.domain.org/my@nick/something");

    assert_string_equal("room", result->localpart);
    assert_string_equal("conference.domain.org", result->domainpart);
    assert_string_equal("my@nick/something", result->resourcepart);
    assert_string_equal("room@conference.domain.org", result->barejid);
    assert_string_equal("room@conference.domain.org/my@nick/something", result->fulljid);

    jid_destroy(result);
}

void
create_full_with_trailing_slash(void** state)
{
    Jid* result = jid_create("room@conference.domain.org/nick/");

    assert_string_equal("room", result->localpart);
    assert_string_equal("conference.domain.org", result->domainpart);
    assert_string_equal("nick/", result->resourcepart);
    assert_string_equal("room@conference.domain.org", result->barejid);
    assert_string_equal("room@conference.domain.org/nick/", result->fulljid);

    jid_destroy(result);
}

void
returns_fulljid_when_exists(void** state)
{
    Jid* jid = jid_create("localpart@domainpart/resourcepart");

    char* result = jid_fulljid_or_barejid(jid);

    assert_string_equal("localpart@domainpart/resourcepart", result);

    jid_destroy(jid);
}

void
returns_barejid_when_fulljid_not_exists(void** state)
{
    Jid* jid = jid_create("localpart@domainpart");

    char* result = jid_fulljid_or_barejid(jid);

    assert_string_equal("localpart@domainpart", result);

    jid_destroy(jid);
}
