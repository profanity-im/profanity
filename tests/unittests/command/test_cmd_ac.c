#include <stdlib.h>
#include <glib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include "prof_cmocka.h"
#include "command/cmd_ac.h"

static void
_create_test_file(const char* path)
{
    FILE* f = fopen(path, "w");
    if (f) {
        fprintf(f, "test");
        fclose(f);
    }
}

void
cmd_ac_complete_filepath__segfaults_when_empty(void** state)
{
    char* result = cmd_ac_complete_filepath("/sendfile ", "/sendfile", FALSE);

    free(result);
}

void
cmd_ac_complete_filepath__finds_files_in_current_dir(void** state)
{
    mkdir("test_dir", 0755);
    _create_test_file("test_dir/file1.txt");
    _create_test_file("test_dir/file2.txt");

    char* result = cmd_ac_complete_filepath("/sendfile test_dir/file", "/sendfile", FALSE);
    assert_non_null(result);
    assert_string_equal(result, "/sendfile test_dir/file1.txt");
    free(result);

    remove("test_dir/file1.txt");
    remove("test_dir/file2.txt");
    rmdir("test_dir");
}

void
cmd_ac_complete_filepath__finds_files_with_dot_slash(void** state)
{
    mkdir("test_dir", 0755);
    _create_test_file("test_dir/file1.txt");

    char* result = cmd_ac_complete_filepath("/sendfile ./test_dir/file", "/sendfile", FALSE);
    assert_non_null(result);
    assert_string_equal(result, "/sendfile ./test_dir/file1.txt");
    free(result);

    remove("test_dir/file1.txt");
    rmdir("test_dir");
}

void
cmd_ac_complete_filepath__cycles_through_files(void** state)
{
    mkdir("test_dir", 0755);
    _create_test_file("test_dir/file1.txt");
    _create_test_file("test_dir/file2.txt");

    // 1st TAB
    char* res1 = cmd_ac_complete_filepath("/sendfile test_dir/file", "/sendfile", FALSE);
    assert_non_null(res1);
    assert_string_equal(res1, "/sendfile test_dir/file1.txt");

    // 2nd TAB
    char* res2 = cmd_ac_complete_filepath(res1, "/sendfile", FALSE);
    assert_non_null(res2);
    assert_string_equal(res2, "/sendfile test_dir/file2.txt");

    // 3rd TAB
    char* res3 = cmd_ac_complete_filepath(res2, "/sendfile", FALSE);
    assert_non_null(res3);
    assert_string_equal(res3, "/sendfile test_dir/file1.txt");

    // SHIFT-TAB
    char* res4 = cmd_ac_complete_filepath(res3, "/sendfile", TRUE);
    assert_non_null(res4);
    assert_string_equal(res4, "/sendfile test_dir/file2.txt");

    free(res1);
    free(res2);
    free(res3);
    free(res4);

    remove("test_dir/file1.txt");
    remove("test_dir/file2.txt");
    rmdir("test_dir");
}
