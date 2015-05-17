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
        unit_test_setup_teardown(connect_with_jid,
            init_prof_test,
            close_prof_test),
    };

    return run_tests(all_tests);
}
