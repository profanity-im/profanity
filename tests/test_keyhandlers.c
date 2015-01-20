#include "ui/keyhandlers.h"
#include "ui/inputwin.h"
#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>
#include <stdlib.h>
#include <string.h>

#include <locale.h>

static char line[INP_WIN_MAX];

static int utf8_pos_to_col(char *str, int utf8_pos)
{
    int col = 0;

    int i = 0;
    for (i = 0; i<utf8_pos; i++) {
        col++;
        gchar *ch = g_utf8_offset_to_pointer(str, i);
        gunichar uni = g_utf8_get_char(ch);
        if (g_unichar_iswide(uni)) {
            col++;
        }
    }

    return col;
}

void append_non_wide_to_empty(void **state)
{
    setlocale(LC_ALL, "");
    line[0] = '\0';
    int line_utf8_pos = 0;
    int col = utf8_pos_to_col(line, line_utf8_pos);
    int pad_start = 0;

    key_printable(line, &line_utf8_pos, &col, &pad_start, 'a', 80);

    assert_string_equal("a", line);
    assert_int_equal(line_utf8_pos, 1);
    assert_int_equal(col, utf8_pos_to_col(line, line_utf8_pos));
    assert_int_equal(pad_start, 0);
}

void append_wide_to_empty(void **state)
{
    setlocale(LC_ALL, "");
    line[0] = '\0';
    int line_utf8_pos = 0;
    int col = utf8_pos_to_col(line, line_utf8_pos);
    int pad_start = 0;

    key_printable(line, &line_utf8_pos, &col, &pad_start, 0x56DB, 80);

    assert_string_equal("四", line);
    assert_int_equal(line_utf8_pos, 1);
    assert_int_equal(col, utf8_pos_to_col(line, line_utf8_pos));
    assert_int_equal(pad_start, 0);
}

void append_non_wide_to_non_wide(void **state)
{
    setlocale(LC_ALL, "");
    strncpy(line, "a", 1);
    line[1] = '\0';
    int line_utf8_pos = 1;
    int col = utf8_pos_to_col(line, line_utf8_pos);
    int pad_start = 0;

    key_printable(line, &line_utf8_pos, &col, &pad_start, 'b', 80);

    assert_string_equal("ab", line);
    assert_int_equal(line_utf8_pos, 2);
    assert_int_equal(col, utf8_pos_to_col(line, line_utf8_pos));
    assert_int_equal(pad_start, 0);
}

void append_wide_to_non_wide(void **state)
{
    setlocale(LC_ALL, "");
    strncpy(line, "a", 1);
    line[1] = '\0';
    int line_utf8_pos = 1;
    int col = utf8_pos_to_col(line, line_utf8_pos);
    int pad_start = 0;

    key_printable(line, &line_utf8_pos, &col, &pad_start, 0x56DB, 80);

    assert_string_equal("a四", line);
    assert_int_equal(line_utf8_pos, 2);
    assert_int_equal(col, utf8_pos_to_col(line, line_utf8_pos));
    assert_int_equal(pad_start, 0);
}

void append_non_wide_to_wide(void **state)
{
    setlocale(LC_ALL, "");
    g_utf8_strncpy(line, "四", 1);
    line[strlen(line)] = '\0';
    int line_utf8_pos = 1;
    int col = utf8_pos_to_col(line, line_utf8_pos);
    int pad_start = 0;

    key_printable(line, &line_utf8_pos, &col, &pad_start, 'b', 80);

    assert_string_equal("四b", line);
    assert_int_equal(line_utf8_pos, 2);
    assert_int_equal(col, utf8_pos_to_col(line, line_utf8_pos));
    assert_int_equal(pad_start, 0);
}

void append_wide_to_wide(void **state)
{
    setlocale(LC_ALL, "");
    g_utf8_strncpy(line, "四", 1);
    line[strlen(line)] = '\0';
    int line_utf8_pos = 1;
    int col = utf8_pos_to_col(line, line_utf8_pos);
    int pad_start = 0;

    key_printable(line, &line_utf8_pos, &col, &pad_start, 0x4E09, 80);

    assert_string_equal("四三", line);
    assert_int_equal(line_utf8_pos, 2);
    assert_int_equal(col, utf8_pos_to_col(line, line_utf8_pos));
    assert_int_equal(pad_start, 0);
}

void append_no_wide_when_overrun(void **state)
{
    setlocale(LC_ALL, "");
    g_utf8_strncpy(line, "0123456789四1234567", 18);
    line[strlen(line)] = '\0';
    int line_utf8_pos = 18;
    int col = utf8_pos_to_col(line, line_utf8_pos);
    int pad_start = 0;

    key_printable(line, &line_utf8_pos, &col, &pad_start, 'z', 20);
    key_printable(line, &line_utf8_pos, &col, &pad_start, 'z', 20);
    key_printable(line, &line_utf8_pos, &col, &pad_start, 'z', 20);

    assert_string_equal("0123456789四1234567zzz", line);
    assert_int_equal(line_utf8_pos, 21);
    assert_int_equal(col, utf8_pos_to_col(line, line_utf8_pos));
    assert_int_equal(pad_start, 3);
}

void append_wide_when_overrun(void **state)
{
    setlocale(LC_ALL, "");
    g_utf8_strncpy(line, "0123456789四1234567", 18);
    line[strlen(line)] = '\0';
    int line_utf8_pos = 18;
    int col = utf8_pos_to_col(line, line_utf8_pos);
    int pad_start = 0;

    key_printable(line, &line_utf8_pos, &col, &pad_start, 0x4E09, 20);
    key_printable(line, &line_utf8_pos, &col, &pad_start, 0x4E09, 20);
    key_printable(line, &line_utf8_pos, &col, &pad_start, 0x4E09, 20);

    assert_string_equal("0123456789四1234567三三三", line);
    assert_int_equal(line_utf8_pos, 21);
    assert_int_equal(col, utf8_pos_to_col(line, line_utf8_pos));
    assert_int_equal(pad_start, 6);
}

void insert_non_wide_to_non_wide(void **state)
{
    setlocale(LC_ALL, "");
    g_utf8_strncpy(line, "abcd", 4);
    line[strlen(line)] = '\0';
    int line_utf8_pos = 2;
    int col = utf8_pos_to_col(line, line_utf8_pos);
    int pad_start = 0;

    key_printable(line, &line_utf8_pos, &col, &pad_start, '0', 80);

    assert_string_equal("ab0cd", line);
    assert_int_equal(line_utf8_pos, 3);
    assert_int_equal(col, utf8_pos_to_col(line, line_utf8_pos));
    assert_int_equal(pad_start, 0);
}

void insert_wide_to_non_wide(void **state)
{
    setlocale(LC_ALL, "");
    g_utf8_strncpy(line, "abcd", 26);
    line[strlen(line)] = '\0';
    int line_utf8_pos = 2;
    int col = utf8_pos_to_col(line, line_utf8_pos);
    int pad_start = 0;

    key_printable(line, &line_utf8_pos, &col, &pad_start, 0x304C, 80);

    assert_string_equal("abがcd", line);
    assert_int_equal(line_utf8_pos, 3);
    assert_int_equal(col, utf8_pos_to_col(line, line_utf8_pos));
    assert_int_equal(pad_start, 0);
}

void insert_non_wide_to_wide(void **state)
{
    setlocale(LC_ALL, "");
    g_utf8_strncpy(line, "ひらなひ", 4);
    line[strlen(line)] = '\0';
    int line_utf8_pos = 2;
    int col = utf8_pos_to_col(line, line_utf8_pos);
    int pad_start = 0;

    key_printable(line, &line_utf8_pos, &col, &pad_start, '0', 80);

    assert_string_equal("ひら0なひ", line);
    assert_int_equal(line_utf8_pos, 3);
    assert_int_equal(col, utf8_pos_to_col(line, line_utf8_pos));
    assert_int_equal(pad_start, 0);
}

void insert_wide_to_wide(void **state)
{
    setlocale(LC_ALL, "");
    g_utf8_strncpy(line, "ひらなひ", 4);
    line[strlen(line)] = '\0';
    int line_utf8_pos = 2;
    int col = utf8_pos_to_col(line, line_utf8_pos);
    int pad_start = 0;

    key_printable(line, &line_utf8_pos, &col, &pad_start, 0x4E09, 80);

    assert_string_equal("ひら三なひ", line);
    assert_int_equal(line_utf8_pos, 3);
    assert_int_equal(col, utf8_pos_to_col(line, line_utf8_pos));
    assert_int_equal(pad_start, 0);
}