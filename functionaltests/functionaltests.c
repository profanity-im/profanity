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

int main(int argc, char* argv[]) {

    const UnitTest all_tests[] = {

        unit_test_setup_teardown(connect_jid,
            init_prof_test,
            close_prof_test),
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
    };

    return run_tests(all_tests);
}
