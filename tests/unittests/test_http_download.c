#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>
#include <stdlib.h>
#include <string.h>
#include <glib.h>

#include "config.h"

#include "tools/http_download.h"

typedef struct
{
    char* url;
    char* basename;
} url_test_t;

void
http_basename_from_url_td(void** state)
{
    int num_tests = 11;
    url_test_t tests[] = {
        (url_test_t){
            .url = "https://host.test/image.jpeg",
            .basename = "image.jpeg",
        },
        (url_test_t){
            .url = "https://host.test/image.jpeg#somefragment",
            .basename = "image.jpeg",
        },
        (url_test_t){
            .url = "https://host.test/image.jpeg?query=param",
            .basename = "image.jpeg",
        },
        (url_test_t){
            .url = "https://host.test/image.jpeg?query=param&another=one",
            .basename = "image.jpeg",
        },
        (url_test_t){
            .url = "https://host.test/images/",
            .basename = "images",
        },
        (url_test_t){
            .url = "https://host.test/images/../../file",
            .basename = "file",
        },
        (url_test_t){
            .url = "https://host.test/images/../../file/..",
            .basename = "index.html",
        },
        (url_test_t){
            .url = "https://host.test/images/..//",
            .basename = "index.html",
        },
        (url_test_t){
            .url = "https://host.test/",
            .basename = "index.html",
        },
        (url_test_t){
            .url = "https://host.test",
            .basename = "index.html",
        },
        (url_test_t){
            .url = "aesgcm://host.test",
            .basename = "index.html",
        },
    };

    char* basename;
    for (int i = 0; i < num_tests; i++) {
        basename = http_basename_from_url(tests[i].url);
        assert_string_equal(basename, tests[i].basename);
    }
}
