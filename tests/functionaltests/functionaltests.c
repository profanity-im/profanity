#include <stdarg.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>
#include <sys/stat.h>

#include "config.h"

#include "proftest.h"
#include "test_connect.h"
#include "test_ping.h"
#include "test_rooms.h"
#include "test_presence.h"
#include "test_message.h"
#include "test_carbons.h"
#include "test_chat_session.h"
#include "test_receipts.h"

#define PROF_FUNC_TEST(test) unit_test_setup_teardown(test, init_prof_test, close_prof_test)

int main(int argc, char* argv[]) {

    const UnitTest all_tests[] = {

        PROF_FUNC_TEST(connect_jid_requests_roster),
        PROF_FUNC_TEST(connect_jid_sends_presence_after_receiving_roster),
        PROF_FUNC_TEST(connect_jid_requests_bookmarks),
        PROF_FUNC_TEST(connect_bad_password),
        PROF_FUNC_TEST(connect_shows_presence_updates),

        PROF_FUNC_TEST(ping_multiple),
        PROF_FUNC_TEST(ping_responds),

        PROF_FUNC_TEST(rooms_query),

        PROF_FUNC_TEST(presence_away),
        PROF_FUNC_TEST(presence_away_with_message),
        PROF_FUNC_TEST(presence_online),
        PROF_FUNC_TEST(presence_online_with_message),
        PROF_FUNC_TEST(presence_xa),
        PROF_FUNC_TEST(presence_xa_with_message),
        PROF_FUNC_TEST(presence_dnd),
        PROF_FUNC_TEST(presence_dnd_with_message),
        PROF_FUNC_TEST(presence_chat),
        PROF_FUNC_TEST(presence_chat_with_message),
        PROF_FUNC_TEST(presence_set_priority),
        PROF_FUNC_TEST(presence_includes_priority),
        PROF_FUNC_TEST(presence_received),

        PROF_FUNC_TEST(message_send),
        PROF_FUNC_TEST(message_receive),

        PROF_FUNC_TEST(sends_message_to_barejid_when_contact_offline),
        PROF_FUNC_TEST(sends_message_to_barejid_when_contact_online),
        PROF_FUNC_TEST(sends_message_to_fulljid_when_received_from_fulljid),
        PROF_FUNC_TEST(sends_subsequent_messages_to_fulljid),
        PROF_FUNC_TEST(resets_to_barejid_after_presence_received),
        PROF_FUNC_TEST(new_session_when_message_received_from_different_fulljid),

        PROF_FUNC_TEST(send_enable_carbons),
        PROF_FUNC_TEST(connect_with_carbons_enabled),
        PROF_FUNC_TEST(send_disable_carbons),
        PROF_FUNC_TEST(receive_carbon),
        PROF_FUNC_TEST(receive_self_carbon),

        PROF_FUNC_TEST(send_receipt_request),
        PROF_FUNC_TEST(send_receipt_on_request),
    };

    return run_tests(all_tests);
}
