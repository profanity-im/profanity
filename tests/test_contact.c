#include <glib.h>
#include <stdarg.h>
#include <string.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>
#include <stdlib.h>

#include "contact.h"

void contact_in_group(void **state)
{
    GSList *groups = NULL;
    groups = g_slist_append(groups, strdup("somegroup"));
    PContact contact = p_contact_new("bob@server.com", "bob", groups, "both",
        "is offline", FALSE);

    gboolean result = p_contact_in_group(contact, "somegroup");

    assert_true(result);

    p_contact_free(contact);
    g_slist_free(groups);
}

void contact_not_in_group(void **state)
{
    GSList *groups = NULL;
    groups = g_slist_append(groups, strdup("somegroup"));
    PContact contact = p_contact_new("bob@server.com", "bob", groups, "both",
        "is offline", FALSE);

    gboolean result = p_contact_in_group(contact, "othergroup");

    assert_false(result);

    p_contact_free(contact);
    g_slist_free(groups);
}

void contact_name_when_name_exists(void **state)
{
    PContact contact = p_contact_new("bob@server.com", "bob", NULL, "both",
        "is offline", FALSE);

    const char *name = p_contact_name_or_jid(contact);

    assert_string_equal("bob", name);

    p_contact_free(contact);
}

void contact_jid_when_name_not_exists(void **state)
{
    PContact contact = p_contact_new("bob@server.com", NULL, NULL, "both",
        "is offline", FALSE);

    const char *jid = p_contact_name_or_jid(contact);

    assert_string_equal("bob@server.com", jid);

    p_contact_free(contact);
}

void contact_string_when_name_exists(void **state)
{
    PContact contact = p_contact_new("bob@server.com", "bob", NULL, "both",
        "is offline", FALSE);

    char *str = p_contact_create_display_string(contact, "laptop");

    assert_string_equal("bob (laptop)", str);

    p_contact_free(contact);
    free(str);
}

void contact_string_when_name_not_exists(void **state)
{
    PContact contact = p_contact_new("bob@server.com", NULL, NULL, "both",
        "is offline", FALSE);

    char *str = p_contact_create_display_string(contact, "laptop");

    assert_string_equal("bob@server.com (laptop)", str);

    p_contact_free(contact);
    free(str);
}

void contact_string_when_default_resource(void **state)
{
    PContact contact = p_contact_new("bob@server.com", "bob", NULL, "both",
        "is offline", FALSE);

    char *str = p_contact_create_display_string(contact, "__prof_default");

    assert_string_equal("bob", str);

    p_contact_free(contact);
    free(str);
}

void contact_presence_offline(void **state)
{
    PContact contact = p_contact_new("bob@server.com", "bob", NULL, "both",
        "is offline", FALSE);

    const char *presence = p_contact_presence(contact);

    assert_string_equal("offline", presence);

    p_contact_free(contact);

}
