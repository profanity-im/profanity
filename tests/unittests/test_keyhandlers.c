#include "prof_cmocka.h"
#include <stdlib.h>
#include <string.h>

#include <locale.h>

#include "ui/keyhandlers.h"
#include "ui/inputwin.h"
#include "tests/helpers.h"

static char line[INP_WIN_MAX];

// append

void append_to_empty(void **state)
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

void append_to_single(void **state)
{
    setlocale(LC_ALL, "");
    g_utf8_strncpy(line, "a", 1);
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


void append_wide_to_single_non_wide(void **state)
{
    setlocale(LC_ALL, "");
    g_utf8_strncpy(line, "a", 1);
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

void append_non_wide_to_single_wide(void **state)
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

void append_wide_to_single_wide(void **state)
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

void append_non_wide_when_overrun(void **state)
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

void insert_single_non_wide_when_pad_scrolled(void **state)
{
    setlocale(LC_ALL, "");
    g_utf8_strncpy(line, "AAAAAAAAAAAAAAA", 15);
    line[strlen(line)] = '\0';
    int line_utf8_pos = 2;
    int col = utf8_pos_to_col(line, line_utf8_pos);
    int pad_start = 2;

    key_printable(line, &line_utf8_pos, &col, &pad_start, 'B', 12);

    assert_string_equal("AABAAAAAAAAAAAAA", line);
    assert_int_equal(line_utf8_pos, 3);
    assert_int_equal(col, utf8_pos_to_col(line, line_utf8_pos));
    assert_int_equal(pad_start, 2);
}

void insert_many_non_wide_when_pad_scrolled(void **state)
{
    setlocale(LC_ALL, "");
    g_utf8_strncpy(line, "AAAAAAAAAAAAAAA", 15);
    line[strlen(line)] = '\0';
    int line_utf8_pos = 2;
    int col = utf8_pos_to_col(line, line_utf8_pos);
    int pad_start = 2;

    key_printable(line, &line_utf8_pos, &col, &pad_start, 'B', 12);
    key_printable(line, &line_utf8_pos, &col, &pad_start, 'C', 12);
    key_printable(line, &line_utf8_pos, &col, &pad_start, 'D', 12);

    assert_string_equal("AABCDAAAAAAAAAAAAA", line);
    assert_int_equal(line_utf8_pos, 5);
    assert_int_equal(col, utf8_pos_to_col(line, line_utf8_pos));
    assert_int_equal(pad_start, 2);
}

void insert_single_non_wide_last_column(void **state)
{
    setlocale(LC_ALL, "");
    g_utf8_strncpy(line, "abcdefghijklmno", 15);
    line[strlen(line)] = '\0';
    int line_utf8_pos = 7;
    int col = utf8_pos_to_col(line, line_utf8_pos);
    int pad_start = 2;

    key_printable(line, &line_utf8_pos, &col, &pad_start, '1', 5);

    assert_string_equal("abcdefg1hijklmno", line);
    assert_int_equal(line_utf8_pos, 8);
    assert_int_equal(col, utf8_pos_to_col(line, line_utf8_pos));
    assert_int_equal(pad_start, 3);
}

void insert_many_non_wide_last_column(void **state)
{
    setlocale(LC_ALL, "");
    g_utf8_strncpy(line, "abcdefghijklmno", 15);
    line[strlen(line)] = '\0';
    int line_utf8_pos = 7;
    int col = utf8_pos_to_col(line, line_utf8_pos);
    int pad_start = 2;

    key_printable(line, &line_utf8_pos, &col, &pad_start, '1', 5);
    key_printable(line, &line_utf8_pos, &col, &pad_start, '2', 5);

    assert_string_equal("abcdefg12hijklmno", line);
    assert_int_equal(line_utf8_pos, 9);
    assert_int_equal(col, utf8_pos_to_col(line, line_utf8_pos));
    assert_int_equal(pad_start, 4);
}

void ctrl_left_when_no_input(void **state)
{
    setlocale(LC_ALL, "");
    line[0] = '\0';
    int line_utf8_pos = 0;
    int col = utf8_pos_to_col(line, line_utf8_pos);
    int pad_start = 0;

    key_ctrl_left(line, &line_utf8_pos, &col, &pad_start, 80);

    assert_int_equal(line_utf8_pos, 0);
    assert_int_equal(col, utf8_pos_to_col(line, line_utf8_pos));
    assert_int_equal(pad_start, 0);
}

void ctrl_left_when_at_start(void **state)
{
    setlocale(LC_ALL, "");
    g_utf8_strncpy(line, "abcd efghij klmn opqr", 21);
    line[strlen(line)] = '\0';
    int line_utf8_pos = 0;
    int col = utf8_pos_to_col(line, line_utf8_pos);
    int pad_start = 0;

    key_ctrl_left(line, &line_utf8_pos, &col, &pad_start, 80);

    assert_int_equal(line_utf8_pos, 0);
    assert_int_equal(col, utf8_pos_to_col(line, line_utf8_pos));
    assert_int_equal(pad_start, 0);
}

void ctrl_left_when_in_first_word(void **state)
{
    setlocale(LC_ALL, "");
    g_utf8_strncpy(line, "abcd efghij klmn opqr", 21);
    line[strlen(line)] = '\0';
    int line_utf8_pos = 2;
    int col = utf8_pos_to_col(line, line_utf8_pos);
    int pad_start = 0;

    key_ctrl_left(line, &line_utf8_pos, &col, &pad_start, 80);

    assert_int_equal(line_utf8_pos, 0);
    assert_int_equal(col, utf8_pos_to_col(line, line_utf8_pos));
    assert_int_equal(pad_start, 0);
}

void ctrl_left_when_in_first_space(void **state)
{
    setlocale(LC_ALL, "");
    g_utf8_strncpy(line, "abcd efghij klmn opqr", 21);
    line[strlen(line)] = '\0';
    int line_utf8_pos = 4;
    int col = utf8_pos_to_col(line, line_utf8_pos);
    int pad_start = 0;

    key_ctrl_left(line, &line_utf8_pos, &col, &pad_start, 80);

    assert_int_equal(line_utf8_pos, 0);
    assert_int_equal(col, utf8_pos_to_col(line, line_utf8_pos));
    assert_int_equal(pad_start, 0);
}

void ctrl_left_when_at_start_of_second_word(void **state)
{
    setlocale(LC_ALL, "");
    g_utf8_strncpy(line, "abcd efghij klmn opqr", 21);
    line[strlen(line)] = '\0';
    int line_utf8_pos = 5;
    int col = utf8_pos_to_col(line, line_utf8_pos);
    int pad_start = 0;

    key_ctrl_left(line, &line_utf8_pos, &col, &pad_start, 80);

    assert_int_equal(line_utf8_pos, 0);
    assert_int_equal(col, utf8_pos_to_col(line, line_utf8_pos));
    assert_int_equal(pad_start, 0);
}

void ctrl_left_when_in_second_word(void **state)
{
    setlocale(LC_ALL, "");
    g_utf8_strncpy(line, "abcd efghij klmn opqr", 21);
    line[strlen(line)] = '\0';
    int line_utf8_pos = 8;
    int col = utf8_pos_to_col(line, line_utf8_pos);
    int pad_start = 0;

    key_ctrl_left(line, &line_utf8_pos, &col, &pad_start, 80);

    assert_int_equal(line_utf8_pos, 5);
    assert_int_equal(col, utf8_pos_to_col(line, line_utf8_pos));
    assert_int_equal(pad_start, 0);
}

void ctrl_left_when_at_end_of_second_word(void **state)
{
    setlocale(LC_ALL, "");
    g_utf8_strncpy(line, "abcd efghij klmn opqr", 21);
    line[strlen(line)] = '\0';
    int line_utf8_pos = 10;
    int col = utf8_pos_to_col(line, line_utf8_pos);
    int pad_start = 0;

    key_ctrl_left(line, &line_utf8_pos, &col, &pad_start, 80);

    assert_int_equal(line_utf8_pos, 5);
    assert_int_equal(col, utf8_pos_to_col(line, line_utf8_pos));
    assert_int_equal(pad_start, 0);
}

void ctrl_left_when_in_second_space(void **state)
{
    setlocale(LC_ALL, "");
    g_utf8_strncpy(line, "abcd efghij klmn opqr", 21);
    line[strlen(line)] = '\0';
    int line_utf8_pos = 11;
    int col = utf8_pos_to_col(line, line_utf8_pos);
    int pad_start = 0;

    key_ctrl_left(line, &line_utf8_pos, &col, &pad_start, 80);

    assert_int_equal(line_utf8_pos, 5);
    assert_int_equal(col, utf8_pos_to_col(line, line_utf8_pos));
    assert_int_equal(pad_start, 0);
}

void ctrl_left_when_at_start_of_third_word(void **state)
{
    setlocale(LC_ALL, "");
    g_utf8_strncpy(line, "abcd efghij klmn opqr", 21);
    line[strlen(line)] = '\0';
    int line_utf8_pos = 12;
    int col = utf8_pos_to_col(line, line_utf8_pos);
    int pad_start = 0;

    key_ctrl_left(line, &line_utf8_pos, &col, &pad_start, 80);

    assert_int_equal(line_utf8_pos, 5);
    assert_int_equal(col, utf8_pos_to_col(line, line_utf8_pos));
    assert_int_equal(pad_start, 0);
}

void ctrl_left_when_in_third_word(void **state)
{
    setlocale(LC_ALL, "");
    g_utf8_strncpy(line, "abcd efghij klmn opqr", 21);
    line[strlen(line)] = '\0';
    int line_utf8_pos = 14;
    int col = utf8_pos_to_col(line, line_utf8_pos);
    int pad_start = 0;

    key_ctrl_left(line, &line_utf8_pos, &col, &pad_start, 80);

    assert_int_equal(line_utf8_pos, 12);
    assert_int_equal(col, utf8_pos_to_col(line, line_utf8_pos));
    assert_int_equal(pad_start, 0);
}

void ctrl_left_when_at_end_of_third_word(void **state)
{
    setlocale(LC_ALL, "");
    g_utf8_strncpy(line, "abcd efghij klmn opqr", 21);
    line[strlen(line)] = '\0';
    int line_utf8_pos = 15;
    int col = utf8_pos_to_col(line, line_utf8_pos);
    int pad_start = 0;

    key_ctrl_left(line, &line_utf8_pos, &col, &pad_start, 80);

    assert_int_equal(line_utf8_pos, 12);
    assert_int_equal(col, utf8_pos_to_col(line, line_utf8_pos));
    assert_int_equal(pad_start, 0);
}

void ctrl_left_when_in_third_space(void **state)
{
    setlocale(LC_ALL, "");
    g_utf8_strncpy(line, "abcd efghij klmn opqr", 21);
    line[strlen(line)] = '\0';
    int line_utf8_pos = 16;
    int col = utf8_pos_to_col(line, line_utf8_pos);
    int pad_start = 0;

    key_ctrl_left(line, &line_utf8_pos, &col, &pad_start, 80);

    assert_int_equal(line_utf8_pos, 12);
    assert_int_equal(col, utf8_pos_to_col(line, line_utf8_pos));
    assert_int_equal(pad_start, 0);
}

void ctrl_left_when_at_end(void **state)
{
    setlocale(LC_ALL, "");
    g_utf8_strncpy(line, "abcd efghij klmn opqr", 21);
    line[strlen(line)] = '\0';
    int line_utf8_pos = 20;
    int col = utf8_pos_to_col(line, line_utf8_pos);
    int pad_start = 0;

    key_ctrl_left(line, &line_utf8_pos, &col, &pad_start, 80);

    assert_int_equal(line_utf8_pos, 17);
    assert_int_equal(col, utf8_pos_to_col(line, line_utf8_pos));
    assert_int_equal(pad_start, 0);
}

void ctrl_left_when_in_only_whitespace(void **state)
{
    setlocale(LC_ALL, "");
    g_utf8_strncpy(line, "       ", 7);
    line[strlen(line)] = '\0';
    int line_utf8_pos = 5;
    int col = utf8_pos_to_col(line, line_utf8_pos);
    int pad_start = 0;

    key_ctrl_left(line, &line_utf8_pos, &col, &pad_start, 80);

    assert_int_equal(line_utf8_pos, 0);
    assert_int_equal(col, utf8_pos_to_col(line, line_utf8_pos));
    assert_int_equal(pad_start, 0);
}

void ctrl_left_when_start_whitespace_start_of_word(void **state)
{
    setlocale(LC_ALL, "");
    g_utf8_strncpy(line, "    hello", 9);
    line[strlen(line)] = '\0';
    int line_utf8_pos = 4;
    int col = utf8_pos_to_col(line, line_utf8_pos);
    int pad_start = 0;

    key_ctrl_left(line, &line_utf8_pos, &col, &pad_start, 80);

    assert_int_equal(line_utf8_pos, 0);
    assert_int_equal(col, utf8_pos_to_col(line, line_utf8_pos));
    assert_int_equal(pad_start, 0);
}

void ctrl_left_when_start_whitespace_middle_of_word(void **state)
{
    setlocale(LC_ALL, "");
    g_utf8_strncpy(line, "    hello", 9);
    line[strlen(line)] = '\0';
    int line_utf8_pos = 7;
    int col = utf8_pos_to_col(line, line_utf8_pos);
    int pad_start = 0;

    key_ctrl_left(line, &line_utf8_pos, &col, &pad_start, 80);

    assert_int_equal(line_utf8_pos, 4);
    assert_int_equal(col, utf8_pos_to_col(line, line_utf8_pos));
    assert_int_equal(pad_start, 0);
}

void ctrl_left_in_whitespace_between_words(void **state)
{
    setlocale(LC_ALL, "");
    g_utf8_strncpy(line, "hey    hello", 12);
    line[strlen(line)] = '\0';
    int line_utf8_pos = 5;
    int col = utf8_pos_to_col(line, line_utf8_pos);
    int pad_start = 0;

    key_ctrl_left(line, &line_utf8_pos, &col, &pad_start, 80);

    assert_int_equal(line_utf8_pos, 0);
    assert_int_equal(col, utf8_pos_to_col(line, line_utf8_pos));
    assert_int_equal(pad_start, 0);
}

void ctrl_left_in_whitespace_between_words_start_of_word(void **state)
{
    setlocale(LC_ALL, "");
    g_utf8_strncpy(line, "hey    hello", 12);
    line[strlen(line)] = '\0';
    int line_utf8_pos = 7;
    int col = utf8_pos_to_col(line, line_utf8_pos);
    int pad_start = 0;

    key_ctrl_left(line, &line_utf8_pos, &col, &pad_start, 80);

    assert_int_equal(line_utf8_pos, 0);
    assert_int_equal(col, utf8_pos_to_col(line, line_utf8_pos));
    assert_int_equal(pad_start, 0);
}

void ctrl_left_in_whitespace_between_words_middle_of_word(void **state)
{
    setlocale(LC_ALL, "");
    g_utf8_strncpy(line, "hey    hello", 12);
    line[strlen(line)] = '\0';
    int line_utf8_pos = 9;
    int col = utf8_pos_to_col(line, line_utf8_pos);
    int pad_start = 0;

    key_ctrl_left(line, &line_utf8_pos, &col, &pad_start, 80);

    assert_int_equal(line_utf8_pos, 7);
    assert_int_equal(col, utf8_pos_to_col(line, line_utf8_pos));
    assert_int_equal(pad_start, 0);
}

void ctrl_left_when_word_overrun_to_left(void **state)
{
    setlocale(LC_ALL, "");
    g_utf8_strncpy(line, "someword anotherword", 20);
    line[strlen(line)] = '\0';
    int line_utf8_pos = 18;
    int col = utf8_pos_to_col(line, line_utf8_pos);
    int pad_start = 14;

    key_ctrl_left(line, &line_utf8_pos, &col, &pad_start, 80);

    assert_int_equal(line_utf8_pos, 9);
    assert_int_equal(col, utf8_pos_to_col(line, line_utf8_pos));
    assert_int_equal(pad_start, 9);
}

void ctrl_right_when_no_input(void **state)
{
    setlocale(LC_ALL, "");
    line[0] = '\0';
    int line_utf8_pos = 0;
    int col = utf8_pos_to_col(line, line_utf8_pos);
    int pad_start = 0;

    key_ctrl_right(line, &line_utf8_pos, &col, &pad_start, 80);

    assert_int_equal(line_utf8_pos, 0);
    assert_int_equal(col, utf8_pos_to_col(line, line_utf8_pos));
    assert_int_equal(pad_start, 0);
}

void ctrl_right_when_at_end(void **state)
{
    setlocale(LC_ALL, "");
    g_utf8_strncpy(line, "someword anotherword", 20);
    line[strlen(line)] = '\0';
    int line_utf8_pos = 20;
    int col = utf8_pos_to_col(line, line_utf8_pos);
    int pad_start = 0;

    key_ctrl_right(line, &line_utf8_pos, &col, &pad_start, 80);

    assert_int_equal(line_utf8_pos, 20);
    assert_int_equal(col, utf8_pos_to_col(line, line_utf8_pos));
    assert_int_equal(pad_start, 0);
}

void ctrl_right_one_word_at_start(void **state)
{
    setlocale(LC_ALL, "");
    g_utf8_strncpy(line, "someword", 8);
    line[strlen(line)] = '\0';
    int line_utf8_pos = 0;
    int col = utf8_pos_to_col(line, line_utf8_pos);
    int pad_start = 0;

    key_ctrl_right(line, &line_utf8_pos, &col, &pad_start, 80);

    assert_int_equal(line_utf8_pos, 8);
    assert_int_equal(col, utf8_pos_to_col(line, line_utf8_pos));
    assert_int_equal(pad_start, 0);
}

void ctrl_right_one_word_in_middle(void **state)
{
    setlocale(LC_ALL, "");
    g_utf8_strncpy(line, "someword", 8);
    line[strlen(line)] = '\0';
    int line_utf8_pos = 3;
    int col = utf8_pos_to_col(line, line_utf8_pos);
    int pad_start = 0;

    key_ctrl_right(line, &line_utf8_pos, &col, &pad_start, 80);

    assert_int_equal(line_utf8_pos, 8);
    assert_int_equal(col, utf8_pos_to_col(line, line_utf8_pos));
    assert_int_equal(pad_start, 0);
}

void ctrl_right_one_word_at_end(void **state)
{
    setlocale(LC_ALL, "");
    g_utf8_strncpy(line, "someword", 8);
    line[strlen(line)] = '\0';
    int line_utf8_pos = 7;
    int col = utf8_pos_to_col(line, line_utf8_pos);
    int pad_start = 0;

    key_ctrl_right(line, &line_utf8_pos, &col, &pad_start, 80);

    assert_int_equal(line_utf8_pos, 8);
    assert_int_equal(col, utf8_pos_to_col(line, line_utf8_pos));
    assert_int_equal(pad_start, 0);
}

void ctrl_right_two_words_from_middle_first(void **state)
{
    setlocale(LC_ALL, "");
    g_utf8_strncpy(line, "someword anotherword", 20);
    line[strlen(line)] = '\0';
    int line_utf8_pos = 4;
    int col = utf8_pos_to_col(line, line_utf8_pos);
    int pad_start = 0;

    key_ctrl_right(line, &line_utf8_pos, &col, &pad_start, 80);

    assert_int_equal(line_utf8_pos, 8);
    assert_int_equal(col, utf8_pos_to_col(line, line_utf8_pos));
    assert_int_equal(pad_start, 0);
}

void ctrl_right_two_words_from_end_first(void **state)
{
    setlocale(LC_ALL, "");
    g_utf8_strncpy(line, "someword anotherword", 20);
    line[strlen(line)] = '\0';
    int line_utf8_pos = 7;
    int col = utf8_pos_to_col(line, line_utf8_pos);
    int pad_start = 0;

    key_ctrl_right(line, &line_utf8_pos, &col, &pad_start, 80);

    assert_int_equal(line_utf8_pos, 8);
    assert_int_equal(col, utf8_pos_to_col(line, line_utf8_pos));
    assert_int_equal(pad_start, 0);
}

void ctrl_right_two_words_from_space(void **state)
{
    setlocale(LC_ALL, "");
    g_utf8_strncpy(line, "someword anotherword", 20);
    line[strlen(line)] = '\0';
    int line_utf8_pos = 8;
    int col = utf8_pos_to_col(line, line_utf8_pos);
    int pad_start = 0;

    key_ctrl_right(line, &line_utf8_pos, &col, &pad_start, 80);

    assert_int_equal(line_utf8_pos, 20);
    assert_int_equal(col, utf8_pos_to_col(line, line_utf8_pos));
    assert_int_equal(pad_start, 0);
}

void ctrl_right_two_words_from_start_second(void **state)
{
    setlocale(LC_ALL, "");
    g_utf8_strncpy(line, "someword anotherword", 20);
    line[strlen(line)] = '\0';
    int line_utf8_pos = 9;
    int col = utf8_pos_to_col(line, line_utf8_pos);
    int pad_start = 0;

    key_ctrl_right(line, &line_utf8_pos, &col, &pad_start, 80);

    assert_int_equal(line_utf8_pos, 20);
    assert_int_equal(col, utf8_pos_to_col(line, line_utf8_pos));
    assert_int_equal(pad_start, 0);
}

void ctrl_right_one_word_leading_whitespace(void **state)
{
    setlocale(LC_ALL, "");
    g_utf8_strncpy(line, "       someword", 15);
    line[strlen(line)] = '\0';
    int line_utf8_pos = 3;
    int col = utf8_pos_to_col(line, line_utf8_pos);
    int pad_start = 0;

    key_ctrl_right(line, &line_utf8_pos, &col, &pad_start, 80);

    assert_int_equal(line_utf8_pos, 15);
    assert_int_equal(col, utf8_pos_to_col(line, line_utf8_pos));
    assert_int_equal(pad_start, 0);
}

void ctrl_right_two_words_in_whitespace(void **state)
{
    setlocale(LC_ALL, "");
    g_utf8_strncpy(line, "       someword        adfasdf", 30);
    line[strlen(line)] = '\0';
    int line_utf8_pos = 19;
    int col = utf8_pos_to_col(line, line_utf8_pos);
    int pad_start = 0;

    key_ctrl_right(line, &line_utf8_pos, &col, &pad_start, 80);

    assert_int_equal(line_utf8_pos, 30);
    assert_int_equal(col, utf8_pos_to_col(line, line_utf8_pos));
    assert_int_equal(pad_start, 0);
}

void ctrl_right_trailing_whitespace_from_middle(void **state)
{
    setlocale(LC_ALL, "");
    g_utf8_strncpy(line, "someword        ", 16);
    line[strlen(line)] = '\0';
    int line_utf8_pos = 3;
    int col = utf8_pos_to_col(line, line_utf8_pos);
    int pad_start = 0;

    key_ctrl_right(line, &line_utf8_pos, &col, &pad_start, 80);

    assert_int_equal(line_utf8_pos, 8);
    assert_int_equal(col, utf8_pos_to_col(line, line_utf8_pos));
    assert_int_equal(pad_start, 0);
}
