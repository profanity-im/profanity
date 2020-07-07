#include <glib.h>
#include <stdarg.h>
#include <string.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>
#include <stdlib.h>

#include "xmpp/contact.h"

void
contact_in_group(void** state)
{
    GSList* groups = NULL;
    groups = g_slist_append(groups, strdup("somegroup"));
    PContact contact = p_contact_new("bob@server.com", "bob", groups, "both",
                                     "is offline", FALSE);

    gboolean result = p_contact_in_group(contact, "somegroup");

    assert_true(result);

    p_contact_free(contact);
    //    g_slist_free(groups);
}

void
contact_not_in_group(void** state)
{
    GSList* groups = NULL;
    groups = g_slist_append(groups, strdup("somegroup"));
    PContact contact = p_contact_new("bob@server.com", "bob", groups, "both",
                                     "is offline", FALSE);

    gboolean result = p_contact_in_group(contact, "othergroup");

    assert_false(result);

    p_contact_free(contact);
    //    g_slist_free(groups);
}

void
contact_name_when_name_exists(void** state)
{
    PContact contact = p_contact_new("bob@server.com", "bob", NULL, "both",
                                     "is offline", FALSE);

    const char* name = p_contact_name_or_jid(contact);

    assert_string_equal("bob", name);

    p_contact_free(contact);
}

void
contact_jid_when_name_not_exists(void** state)
{
    PContact contact = p_contact_new("bob@server.com", NULL, NULL, "both",
                                     "is offline", FALSE);

    const char* jid = p_contact_name_or_jid(contact);

    assert_string_equal("bob@server.com", jid);

    p_contact_free(contact);
}

void
contact_string_when_name_exists(void** state)
{
    PContact contact = p_contact_new("bob@server.com", "bob", NULL, "both",
                                     "is offline", FALSE);

    char* str = p_contact_create_display_string(contact, "laptop");

    assert_string_equal("bob (laptop)", str);

    p_contact_free(contact);
    free(str);
}

void
contact_string_when_name_not_exists(void** state)
{
    PContact contact = p_contact_new("bob@server.com", NULL, NULL, "both",
                                     "is offline", FALSE);

    char* str = p_contact_create_display_string(contact, "laptop");

    assert_string_equal("bob@server.com (laptop)", str);

    p_contact_free(contact);
    free(str);
}

void
contact_string_when_default_resource(void** state)
{
    PContact contact = p_contact_new("bob@server.com", "bob", NULL, "both",
                                     "is offline", FALSE);

    char* str = p_contact_create_display_string(contact, "__prof_default");

    assert_string_equal("bob", str);

    p_contact_free(contact);
    free(str);
}

void
contact_presence_offline(void** state)
{
    PContact contact = p_contact_new("bob@server.com", "bob", NULL, "both",
                                     "is offline", FALSE);

    const char* presence = p_contact_presence(contact);

    assert_string_equal("offline", presence);

    p_contact_free(contact);
}

void
contact_presence_uses_highest_priority(void** state)
{
    PContact contact = p_contact_new("bob@server.com", "bob", NULL, "both",
                                     "is offline", FALSE);

    Resource* resource10 = resource_new("resource10", RESOURCE_ONLINE, NULL, 10);
    Resource* resource20 = resource_new("resource20", RESOURCE_CHAT, NULL, 20);
    Resource* resource30 = resource_new("resource30", RESOURCE_AWAY, NULL, 30);
    Resource* resource1 = resource_new("resource1", RESOURCE_XA, NULL, 1);
    Resource* resource2 = resource_new("resource2", RESOURCE_DND, NULL, 2);
    p_contact_set_presence(contact, resource10);
    p_contact_set_presence(contact, resource20);
    p_contact_set_presence(contact, resource30);
    p_contact_set_presence(contact, resource1);
    p_contact_set_presence(contact, resource2);

    const char* presence = p_contact_presence(contact);

    assert_string_equal("away", presence);

    p_contact_free(contact);
}

void
contact_presence_chat_when_same_prioroty(void** state)
{
    PContact contact = p_contact_new("bob@server.com", "bob", NULL, "both",
                                     "is offline", FALSE);

    Resource* resource_online = resource_new("resource_online", RESOURCE_ONLINE, NULL, 10);
    Resource* resource_chat = resource_new("resource_chat", RESOURCE_CHAT, NULL, 10);
    Resource* resource_away = resource_new("resource_away", RESOURCE_AWAY, NULL, 10);
    Resource* resource_xa = resource_new("resource_xa", RESOURCE_XA, NULL, 10);
    Resource* resource_dnd = resource_new("resource_dnd", RESOURCE_DND, NULL, 10);
    p_contact_set_presence(contact, resource_online);
    p_contact_set_presence(contact, resource_chat);
    p_contact_set_presence(contact, resource_away);
    p_contact_set_presence(contact, resource_xa);
    p_contact_set_presence(contact, resource_dnd);

    const char* presence = p_contact_presence(contact);

    assert_string_equal("chat", presence);

    p_contact_free(contact);
}

void
contact_presence_online_when_same_prioroty(void** state)
{
    PContact contact = p_contact_new("bob@server.com", "bob", NULL, "both",
                                     "is offline", FALSE);

    Resource* resource_online = resource_new("resource_online", RESOURCE_ONLINE, NULL, 10);
    Resource* resource_away = resource_new("resource_away", RESOURCE_AWAY, NULL, 10);
    Resource* resource_xa = resource_new("resource_xa", RESOURCE_XA, NULL, 10);
    Resource* resource_dnd = resource_new("resource_dnd", RESOURCE_DND, NULL, 10);
    p_contact_set_presence(contact, resource_online);
    p_contact_set_presence(contact, resource_away);
    p_contact_set_presence(contact, resource_xa);
    p_contact_set_presence(contact, resource_dnd);

    const char* presence = p_contact_presence(contact);

    assert_string_equal("online", presence);

    p_contact_free(contact);
}

void
contact_presence_away_when_same_prioroty(void** state)
{
    PContact contact = p_contact_new("bob@server.com", "bob", NULL, "both",
                                     "is offline", FALSE);

    Resource* resource_away = resource_new("resource_away", RESOURCE_AWAY, NULL, 10);
    Resource* resource_xa = resource_new("resource_xa", RESOURCE_XA, NULL, 10);
    Resource* resource_dnd = resource_new("resource_dnd", RESOURCE_DND, NULL, 10);
    p_contact_set_presence(contact, resource_away);
    p_contact_set_presence(contact, resource_xa);
    p_contact_set_presence(contact, resource_dnd);

    const char* presence = p_contact_presence(contact);

    assert_string_equal("away", presence);

    p_contact_free(contact);
}

void
contact_presence_xa_when_same_prioroty(void** state)
{
    PContact contact = p_contact_new("bob@server.com", "bob", NULL, "both",
                                     "is offline", FALSE);

    Resource* resource_xa = resource_new("resource_xa", RESOURCE_XA, NULL, 10);
    Resource* resource_dnd = resource_new("resource_dnd", RESOURCE_DND, NULL, 10);
    p_contact_set_presence(contact, resource_xa);
    p_contact_set_presence(contact, resource_dnd);

    const char* presence = p_contact_presence(contact);

    assert_string_equal("xa", presence);

    p_contact_free(contact);
}

void
contact_presence_dnd(void** state)
{
    PContact contact = p_contact_new("bob@server.com", "bob", NULL, "both",
                                     "is offline", FALSE);

    Resource* resource_dnd = resource_new("resource_dnd", RESOURCE_DND, NULL, 10);
    p_contact_set_presence(contact, resource_dnd);

    const char* presence = p_contact_presence(contact);

    assert_string_equal("dnd", presence);

    p_contact_free(contact);
}

void
contact_subscribed_when_to(void** state)
{
    PContact contact = p_contact_new("bob@server.com", "bob", NULL, "to",
                                     "is offline", FALSE);

    gboolean result = p_contact_subscribed(contact);

    assert_true(result);

    p_contact_free(contact);
}

void
contact_subscribed_when_both(void** state)
{
    PContact contact = p_contact_new("bob@server.com", "bob", NULL, "both",
                                     "is offline", FALSE);

    gboolean result = p_contact_subscribed(contact);

    assert_true(result);

    p_contact_free(contact);
}

void
contact_not_subscribed_when_from(void** state)
{
    PContact contact = p_contact_new("bob@server.com", "bob", NULL, "from",
                                     "is offline", FALSE);

    gboolean result = p_contact_subscribed(contact);

    assert_false(result);

    p_contact_free(contact);
}

void
contact_not_subscribed_when_no_subscription_value(void** state)
{
    PContact contact = p_contact_new("bob@server.com", "bob", NULL, NULL,
                                     "is offline", FALSE);

    gboolean result = p_contact_subscribed(contact);

    assert_false(result);

    p_contact_free(contact);
}

void
contact_not_available(void** state)
{
    PContact contact = p_contact_new("bob@server.com", "bob", NULL, NULL,
                                     "is offline", FALSE);

    gboolean result = p_contact_is_available(contact);

    assert_false(result);

    p_contact_free(contact);
}

void
contact_not_available_when_highest_priority_away(void** state)
{
    PContact contact = p_contact_new("bob@server.com", "bob", NULL, NULL,
                                     "is offline", FALSE);

    Resource* resource_online = resource_new("resource_online", RESOURCE_ONLINE, NULL, 10);
    Resource* resource_chat = resource_new("resource_chat", RESOURCE_CHAT, NULL, 10);
    Resource* resource_away = resource_new("resource_away", RESOURCE_AWAY, NULL, 20);
    Resource* resource_xa = resource_new("resource_xa", RESOURCE_XA, NULL, 10);
    Resource* resource_dnd = resource_new("resource_dnd", RESOURCE_DND, NULL, 10);
    p_contact_set_presence(contact, resource_online);
    p_contact_set_presence(contact, resource_chat);
    p_contact_set_presence(contact, resource_away);
    p_contact_set_presence(contact, resource_xa);
    p_contact_set_presence(contact, resource_dnd);

    gboolean result = p_contact_is_available(contact);

    assert_false(result);

    p_contact_free(contact);
}

void
contact_not_available_when_highest_priority_xa(void** state)
{
    PContact contact = p_contact_new("bob@server.com", "bob", NULL, NULL,
                                     "is offline", FALSE);

    Resource* resource_online = resource_new("resource_online", RESOURCE_ONLINE, NULL, 10);
    Resource* resource_chat = resource_new("resource_chat", RESOURCE_CHAT, NULL, 10);
    Resource* resource_away = resource_new("resource_away", RESOURCE_AWAY, NULL, 10);
    Resource* resource_xa = resource_new("resource_xa", RESOURCE_XA, NULL, 20);
    Resource* resource_dnd = resource_new("resource_dnd", RESOURCE_DND, NULL, 10);
    p_contact_set_presence(contact, resource_online);
    p_contact_set_presence(contact, resource_chat);
    p_contact_set_presence(contact, resource_away);
    p_contact_set_presence(contact, resource_xa);
    p_contact_set_presence(contact, resource_dnd);

    gboolean result = p_contact_is_available(contact);

    assert_false(result);

    p_contact_free(contact);
}

void
contact_not_available_when_highest_priority_dnd(void** state)
{
    PContact contact = p_contact_new("bob@server.com", "bob", NULL, NULL,
                                     "is offline", FALSE);

    Resource* resource_online = resource_new("resource_online", RESOURCE_ONLINE, NULL, 10);
    Resource* resource_chat = resource_new("resource_chat", RESOURCE_CHAT, NULL, 10);
    Resource* resource_away = resource_new("resource_away", RESOURCE_AWAY, NULL, 10);
    Resource* resource_xa = resource_new("resource_xa", RESOURCE_XA, NULL, 10);
    Resource* resource_dnd = resource_new("resource_dnd", RESOURCE_DND, NULL, 20);
    p_contact_set_presence(contact, resource_online);
    p_contact_set_presence(contact, resource_chat);
    p_contact_set_presence(contact, resource_away);
    p_contact_set_presence(contact, resource_xa);
    p_contact_set_presence(contact, resource_dnd);

    gboolean result = p_contact_is_available(contact);

    assert_false(result);

    p_contact_free(contact);
}

void
contact_available_when_highest_priority_online(void** state)
{
    PContact contact = p_contact_new("bob@server.com", "bob", NULL, NULL,
                                     "is offline", FALSE);

    Resource* resource_online = resource_new("resource_online", RESOURCE_ONLINE, NULL, 20);
    Resource* resource_chat = resource_new("resource_chat", RESOURCE_CHAT, NULL, 10);
    Resource* resource_away = resource_new("resource_away", RESOURCE_AWAY, NULL, 10);
    Resource* resource_xa = resource_new("resource_xa", RESOURCE_XA, NULL, 10);
    Resource* resource_dnd = resource_new("resource_dnd", RESOURCE_DND, NULL, 10);
    p_contact_set_presence(contact, resource_online);
    p_contact_set_presence(contact, resource_chat);
    p_contact_set_presence(contact, resource_away);
    p_contact_set_presence(contact, resource_xa);
    p_contact_set_presence(contact, resource_dnd);

    gboolean result = p_contact_is_available(contact);

    assert_true(result);

    p_contact_free(contact);
}

void
contact_available_when_highest_priority_chat(void** state)
{
    PContact contact = p_contact_new("bob@server.com", "bob", NULL, NULL,
                                     "is offline", FALSE);

    Resource* resource_online = resource_new("resource_online", RESOURCE_ONLINE, NULL, 10);
    Resource* resource_chat = resource_new("resource_chat", RESOURCE_CHAT, NULL, 20);
    Resource* resource_away = resource_new("resource_away", RESOURCE_AWAY, NULL, 10);
    Resource* resource_xa = resource_new("resource_xa", RESOURCE_XA, NULL, 10);
    Resource* resource_dnd = resource_new("resource_dnd", RESOURCE_DND, NULL, 10);
    p_contact_set_presence(contact, resource_online);
    p_contact_set_presence(contact, resource_chat);
    p_contact_set_presence(contact, resource_away);
    p_contact_set_presence(contact, resource_xa);
    p_contact_set_presence(contact, resource_dnd);

    gboolean result = p_contact_is_available(contact);

    assert_true(result);

    p_contact_free(contact);
}
