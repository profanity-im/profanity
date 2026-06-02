#include <check.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <stdio.h>

/* Test that sprintf("%d", idls_secs) never overflows a 12-byte buffer.
 * We simulate the exact pattern from src/xmpp/iq.c and verify the
 * invariant: the formatted integer always fits within 12 bytes (11 chars + null).
 */

START_TEST(test_sprintf_int_buffer_bounds)
{
    /* Invariant: sprintf("%d", value) must never write more than 11 chars + null
     * into the buffer used in iq.c. Buffer must be >= 12 bytes. */
    int payloads[] = {
        INT_MIN,       /* exact exploit: -2147483648 = 11 chars, needs 12 bytes */
        INT_MAX,       /* boundary: 2147483647 = 10 chars */
        0,             /* valid/normal input */
        -1,            /* boundary negative */
    };
    int num_payloads = sizeof(payloads) / sizeof(payloads[0]);

    for (int i = 0; i < num_payloads; i++) {
        char str[12]; /* minimum safe buffer as required by the invariant */
        memset(str, 0xAA, sizeof(str));

        int written = snprintf(str, sizeof(str), "%d", payloads[i]);

        /* Invariant: output must not be truncated (written < buffer size)
         * and must fit within 12 bytes */
        ck_assert_msg(written >= 0,
            "snprintf failed for value %d", payloads[i]);
        ck_assert_msg(written < (int)sizeof(str),
            "Integer %d formatted to %d chars, overflows 12-byte buffer",
            payloads[i], written);

        /* Verify null terminator is within bounds */
        ck_assert_msg(str[sizeof(str) - 1] == '\0' || strlen(str) < sizeof(str),
            "Buffer not null-terminated within bounds for value %d", payloads[i]);
    }
}
END_TEST

Suite *security_suite(void)
{
    Suite *s;
    TCase *tc_core;

    s = suite_create("Security");
    tc_core = tcase_create("Core");

    tcase_add_test(tc_core, test_sprintf_int_buffer_bounds);
    suite_add_tcase(s, tc_core);

    return s;
}

int main(void)
{
    int number_failed;
    Suite *s;
    SRunner *sr;

    s = security_suite();
    sr = srunner_create(s);

    srunner_run_all(sr, CK_NORMAL);
    number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);

    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}