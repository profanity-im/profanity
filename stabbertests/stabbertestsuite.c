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
//        unit_test_setup_teardown(show_presence_updates,
//            init_prof_test,
//            close_prof_test),
//        unit_test_setup_teardown(sends_rooms_iq,
//            init_prof_test,
//            close_prof_test),
//        unit_test_setup_teardown(multiple_pings,
//            init_prof_test,
//            close_prof_test),
//        unit_test_setup_teardown(responds_to_ping,
//            init_prof_test,
//            close_prof_test),
    };

    return run_tests(all_tests);
}
