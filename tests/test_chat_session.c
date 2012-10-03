#include <stdio.h>
#include <head-unit.h>
#include "chat_session.h"

void can_init(void)
{
    chat_session_init();
    assert_true(1);
}

void adding_new_sets_state_to_active(void)
{
    chat_session_init();
    chat_session_new("prof1@panesar");
    ChatSession session = chat_session_get("prof1@panesar");

    assert_int_equals(ACTIVE, chat_session_get_state(session));
}

void register_chat_session_tests(void)
{
    TEST_MODULE("chat_session_tests");
    TEST(can_init);
    TEST(adding_new_sets_state_to_active);
}
