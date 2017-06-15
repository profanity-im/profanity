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
#include "test_roster.h"
#include "test_software.h"
#include "test_muc.h"
#include "test_disconnect.h"

#define PROF_FUNC_TEST(test) unit_test_setup_teardown(test, init_prof_test, close_prof_test)

int main(int argc, char* argv[]) {

    const UnitTest all_tests[] = {

        PROF_FUNC_TEST(connect_jid_requests_roster),
        PROF_FUNC_TEST(connect_jid_sends_presence_after_receiving_roster),
        PROF_FUNC_TEST(connect_jid_requests_bookmarks),
        PROF_FUNC_TEST(connect_bad_password),
        PROF_FUNC_TEST(connect_shows_presence_updates),

        PROF_FUNC_TEST(ping_server),
        PROF_FUNC_TEST(ping_server_not_supported),
        PROF_FUNC_TEST(ping_responds_to_server_request),
        PROF_FUNC_TEST(ping_jid),
        PROF_FUNC_TEST(ping_jid_not_supported),

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
        PROF_FUNC_TEST(presence_keeps_status),
        PROF_FUNC_TEST(presence_received),
        PROF_FUNC_TEST(presence_missing_resource_defaults),

        PROF_FUNC_TEST(message_send),
        PROF_FUNC_TEST(message_receive_console),
        PROF_FUNC_TEST(message_receive_chatwin),

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
        PROF_FUNC_TEST(receive_private_carbon),

        PROF_FUNC_TEST(send_receipt_request),
        PROF_FUNC_TEST(send_receipt_on_request),
        PROF_FUNC_TEST(does_not_send_receipt_request_to_barejid),
        PROF_FUNC_TEST(sends_new_item),
        PROF_FUNC_TEST(sends_new_item_nick),
        PROF_FUNC_TEST(sends_remove_item),
        PROF_FUNC_TEST(sends_nick_change),

        PROF_FUNC_TEST(send_software_version_request),
        PROF_FUNC_TEST(display_software_version_result),
        PROF_FUNC_TEST(shows_message_when_software_version_error),
        PROF_FUNC_TEST(display_software_version_result_when_from_domainpart),
        PROF_FUNC_TEST(show_message_in_chat_window_when_no_resource),
        PROF_FUNC_TEST(display_software_version_result_in_chat),

        PROF_FUNC_TEST(sends_room_join),
        PROF_FUNC_TEST(sends_room_join_with_nick),
        PROF_FUNC_TEST(sends_room_join_with_password),
        PROF_FUNC_TEST(sends_room_join_with_nick_and_password),
        PROF_FUNC_TEST(shows_role_and_affiliation_on_join),
        PROF_FUNC_TEST(shows_subject_on_join),
        PROF_FUNC_TEST(shows_history_message),
        PROF_FUNC_TEST(shows_occupant_join),
        PROF_FUNC_TEST(shows_message),
        PROF_FUNC_TEST(shows_all_messages_in_console_when_window_not_focussed),
        PROF_FUNC_TEST(shows_first_message_in_console_when_window_not_focussed),
        PROF_FUNC_TEST(shows_no_message_in_console_when_window_not_focussed),

        PROF_FUNC_TEST(disconnect_ends_session),
    };

    return run_tests(all_tests);
}
