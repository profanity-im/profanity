#include <head-unit.h>
#include "testsuite.h"

int main(void)
{
    register_history_tests();
    register_contact_list_tests();
    register_util_tests();
    run_suite();
    return 0;
}
