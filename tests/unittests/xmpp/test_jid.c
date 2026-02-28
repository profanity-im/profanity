#include "prof_cmocka.h"
#include <stdlib.h>

#include "xmpp/jid.h"

void
jid_create__returns__null_from_null(void** state)
{
    Jid* result = jid_create(NULL);
    assert_null(result);
}

void
jid_create__returns__null_from_empty_string(void** state)
{
    Jid* result = jid_create("");
    assert_null(result);
}

void
jid_create__returns__full_from_full(void** state)
{
    Jid* result = jid_create("myuser@mydomain/laptop");
    assert_string_equal("myuser@mydomain/laptop", result->fulljid);
    jid_destroy(result);
}

void
jid_create__returns__bare_from_full(void** state)
{
    Jid* result = jid_create("myuser@mydomain/laptop");
    assert_string_equal("myuser@mydomain", result->barejid);
    jid_destroy(result);
}

void
jid_create__returns__resourcepart_from_full(void** state)
{
    Jid* result = jid_create("myuser@mydomain/laptop");
    assert_string_equal("laptop", result->resourcepart);
    jid_destroy(result);
}

void
jid_create__returns__localpart_from_full(void** state)
{
    Jid* result = jid_create("myuser@mydomain/laptop");
    assert_string_equal("myuser", result->localpart);
    jid_destroy(result);
}

void
jid_create__returns__domainpart_from_full(void** state)
{
    Jid* result = jid_create("myuser@mydomain/laptop");
    assert_string_equal("mydomain", result->domainpart);
    jid_destroy(result);
}

void
jid_create__returns__full_from_full_nolocal(void** state)
{
    Jid* result = jid_create("mydomain/laptop");
    assert_string_equal("mydomain/laptop", result->fulljid);
    jid_destroy(result);
}

void
jid_create__returns__bare_from_full_nolocal(void** state)
{
    Jid* result = jid_create("mydomain/laptop");
    assert_string_equal("mydomain", result->barejid);
    jid_destroy(result);
}

void
jid_create__returns__resourcepart_from_full_nolocal(void** state)
{
    Jid* result = jid_create("mydomain/laptop");
    assert_string_equal("laptop", result->resourcepart);
    jid_destroy(result);
}

void
jid_create__returns__domainpart_from_full_nolocal(void** state)
{
    Jid* result = jid_create("mydomain/laptop");
    assert_string_equal("mydomain", result->domainpart);
    jid_destroy(result);
}

void
jid_create__returns__null_localpart_from_full_nolocal(void** state)
{
    Jid* result = jid_create("mydomain/laptop");
    assert_null(result->localpart);
    jid_destroy(result);
}

void
jid_create__returns__null_full_from_bare(void** state)
{
    Jid* result = jid_create("myuser@mydomain");
    assert_null(result->fulljid);
    jid_destroy(result);
}

void
jid_create__returns__null_resource_from_bare(void** state)
{
    Jid* result = jid_create("myuser@mydomain");
    assert_null(result->resourcepart);
    jid_destroy(result);
}

void
jid_create__returns__bare_from_bare(void** state)
{
    Jid* result = jid_create("myuser@mydomain");
    assert_string_equal("myuser@mydomain", result->barejid);
    jid_destroy(result);
}

void
jid_create__returns__localpart_from_bare(void** state)
{
    Jid* result = jid_create("myuser@mydomain");
    assert_string_equal("myuser", result->localpart);
    jid_destroy(result);
}

void
jid_create__returns__domainpart_from_bare(void** state)
{
    Jid* result = jid_create("myuser@mydomain");
    assert_string_equal("mydomain", result->domainpart);
    jid_destroy(result);
}

void
jid_create_from_bare_and_resource__returns__room(void** state)
{
    Jid* result = jid_create_from_bare_and_resource("room@conference.domain.org", "myname");

    assert_string_equal("room@conference.domain.org", result->barejid);
    jid_destroy(result);
}

void
jid_create_from_bare_and_resource__returns__nick(void** state)
{
    Jid* result = jid_create_from_bare_and_resource("room@conference.domain.org", "myname");

    assert_string_equal("myname", result->resourcepart);
    jid_destroy(result);
}

void
jid_create__returns__correct_parts_with_slash_in_resource(void** state)
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
jid_create__returns__correct_parts_with_at_in_resource(void** state)
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
jid_create__returns__correct_parts_with_at_and_slash_in_resource(void** state)
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
jid_create__returns__correct_parts_with_trailing_slash(void** state)
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
jid_fulljid_or_barejid__returns__fulljid_when_exists(void** state)
{
    Jid* jid = jid_create("localpart@domainpart/resourcepart");

    const gchar* result = jid_fulljid_or_barejid(jid);

    assert_string_equal("localpart@domainpart/resourcepart", result);

    jid_destroy(jid);
}

void
jid_fulljid_or_barejid__returns__barejid_when_fulljid_not_exists(void** state)
{
    Jid* jid = jid_create("localpart@domainpart");

    const gchar* result = jid_fulljid_or_barejid(jid);

    assert_string_equal("localpart@domainpart", result);

    jid_destroy(jid);
}
