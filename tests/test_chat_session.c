#include <stdio.h>
#include <head-unit.h>
#include "chat_session.h"

void setup(void)
{
    chat_session_init();
}

void adding_new_sets_state_to_active(void)
{
    chat_session_start("prof1@panesar");
    chat_state_t state = chat_session_get_state("prof1@panesar");

    assert_int_equals(ACTIVE, state);
}

void set_inactive(void)
{
    chat_session_start("prof2@panesar");
    chat_session_set_state("prof2@panesar", INACTIVE);
    chat_state_t state = chat_session_get_state("prof2@panesar");

    assert_int_equals(INACTIVE, state);
}

void set_gone(void)
{
    chat_session_start("prof3@panesar");
    chat_session_set_state("prof3@panesar", GONE);
    chat_state_t state = chat_session_get_state("prof3@panesar");

    assert_int_equals(GONE, state);
}

void set_composing(void)
{
    chat_session_start("prof4@panesar");
    chat_session_set_state("prof4@panesar", COMPOSING);
    chat_state_t state = chat_session_get_state("prof4@panesar");

    assert_int_equals(COMPOSING, state);
}

void set_paused(void)
{
    chat_session_start("prof5@panesar");
    chat_session_set_state("prof5@panesar", PAUSED);
    chat_state_t state = chat_session_get_state("prof5@panesar");

    assert_int_equals(PAUSED, state);
}

void end_session(void)
{
    chat_session_start(strdup("prof6@panesar"));
    chat_session_end("prof6@panesar");
    chat_state_t state = chat_session_get_state("prof5@panesat");

    assert_int_equals(SESSION_ERR, state);
}

void register_chat_session_tests(void)
{
    TEST_MODULE("chat_session_tests");
    SETUP(setup);
    TEST(adding_new_sets_state_to_active);
    TEST(set_inactive);
    TEST(set_gone);
    TEST(set_composing);
    TEST(set_paused);
    TEST(end_session);
}
