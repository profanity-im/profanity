#include <head-unit.h>
#include "testsuite.h"

int main(void)
{
    register_prof_history_tests();
    register_contact_list_tests();
    register_common_tests();
    register_prof_autocomplete_tests();
    run_suite();
    return 0;
}
