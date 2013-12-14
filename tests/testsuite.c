#include <head-unit.h>
#include "testsuite.h"

int main(void)
{
    register_history_tests();
    register_roster_list_tests();
    register_common_tests();
    register_autocomplete_tests();
    register_parser_tests();
    register_jid_tests();
    run_suite();
    return 0;
}
