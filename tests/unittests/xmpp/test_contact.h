#ifndef TESTS_TEST_CONTACT_H
#define TESTS_TEST_CONTACT_H

void p_contact_in_group__is__true_when_in_group(void** state);
void p_contact_in_group__is__false_when_not_in_group(void** state);
void p_contact_name_or_jid__returns__name_when_exists(void** state);
void p_contact_name_or_jid__returns__jid_when_name_not_exists(void** state);
void p_contact_create_display_string__returns__name_and_resource_when_name_exists(void** state);
void p_contact_create_display_string__returns__jid_and_resource_when_name_not_exists(void** state);
void p_contact_create_display_string__returns__name_when_default_resource(void** state);
void p_contact_presence__returns__offline_when_no_resources(void** state);
void p_contact_presence__returns__highest_priority_presence(void** state);
void p_contact_presence__returns__chat_when_same_priority(void** state);
void p_contact_presence__returns__online_when_same_priority(void** state);
void p_contact_presence__returns__away_when_same_priority(void** state);
void p_contact_presence__returns__xa_when_same_priority(void** state);
void p_contact_presence__returns__dnd(void** state);
void p_contact_subscribed__is__true_when_to(void** state);
void p_contact_subscribed__is__true_when_both(void** state);
void p_contact_subscribed__is__false_when_from(void** state);
void p_contact_subscribed__is__false_when_no_subscription_value(void** state);
void p_contact_is_available__is__false_when_offline(void** state);
void p_contact_is_available__is__false_when_highest_priority_away(void** state);
void p_contact_is_available__is__false_when_highest_priority_xa(void** state);
void p_contact_is_available__is__false_when_highest_priority_dnd(void** state);
void p_contact_is_available__is__true_when_highest_priority_online(void** state);
void p_contact_is_available__is__true_when_highest_priority_chat(void** state);

#endif
