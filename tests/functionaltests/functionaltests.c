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

int main(int argc, char* argv[]) {

    const UnitTest all_tests[] = {

        unit_test_setup_teardown(connect_jid_requests_roster,
            init_prof_test,
            close_prof_test),
        unit_test_setup_teardown(connect_jid_sends_presence_after_receiving_roster,
            init_prof_test,
            close_prof_test),
        unit_test_setup_teardown(connect_jid_requests_bookmarks,
            init_prof_test,
            close_prof_test),
        unit_test_setup_teardown(connect_bad_password,
            init_prof_test,
            close_prof_test),
        unit_test_setup_teardown(connect_shows_presence_updates,
            init_prof_test,
            close_prof_test),

        unit_test_setup_teardown(ping_multiple,
            init_prof_test,
            close_prof_test),
        unit_test_setup_teardown(ping_responds,
            init_prof_test,
            close_prof_test),

        unit_test_setup_teardown(rooms_query,
            init_prof_test,
            close_prof_test),

        unit_test_setup_teardown(presence_away,
            init_prof_test,
            close_prof_test),
        unit_test_setup_teardown(presence_away_with_message,
            init_prof_test,
            close_prof_test),
        unit_test_setup_teardown(presence_online,
            init_prof_test,
            close_prof_test),
        unit_test_setup_teardown(presence_online_with_message,
            init_prof_test,
            close_prof_test),
        unit_test_setup_teardown(presence_xa,
            init_prof_test,
            close_prof_test),
        unit_test_setup_teardown(presence_xa_with_message,
            init_prof_test,
            close_prof_test),
        unit_test_setup_teardown(presence_dnd,
            init_prof_test,
            close_prof_test),
        unit_test_setup_teardown(presence_dnd_with_message,
            init_prof_test,
            close_prof_test),
        unit_test_setup_teardown(presence_chat,
            init_prof_test,
            close_prof_test),
        unit_test_setup_teardown(presence_chat_with_message,
            init_prof_test,
            close_prof_test),
        unit_test_setup_teardown(presence_set_priority,
            init_prof_test,
            close_prof_test),
        unit_test_setup_teardown(presence_includes_priority,
            init_prof_test,
            close_prof_test),
        unit_test_setup_teardown(presence_received,
            init_prof_test,
            close_prof_test),

        unit_test_setup_teardown(message_send,
            init_prof_test,
            close_prof_test),
        unit_test_setup_teardown(message_receive,
            init_prof_test,
            close_prof_test),

        unit_test_setup_teardown(sends_message_to_barejid_when_contact_offline,
            init_prof_test,
            close_prof_test),
        unit_test_setup_teardown(sends_message_to_barejid_when_contact_online,
            init_prof_test,
            close_prof_test),
        unit_test_setup_teardown(sends_message_to_fulljid_when_received_from_fulljid,
            init_prof_test,
            close_prof_test),
        unit_test_setup_teardown(sends_subsequent_messages_to_fulljid,
            init_prof_test,
            close_prof_test),
        unit_test_setup_teardown(resets_to_barejid_after_presence_received,
            init_prof_test,
            close_prof_test),
        unit_test_setup_teardown(new_session_when_message_received_from_different_fulljid,
            init_prof_test,
            close_prof_test),

        unit_test_setup_teardown(send_enable_carbons,
            init_prof_test,
            close_prof_test),
        unit_test_setup_teardown(connect_with_carbons_enabled,
            init_prof_test,
            close_prof_test),
        unit_test_setup_teardown(send_disable_carbons,
            init_prof_test,
            close_prof_test),
    };

    return run_tests(all_tests);
}
