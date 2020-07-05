#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>
#include <stdlib.h>
#include <string.h>
#include <glib.h>

#include "config.h"

#include "tools/http_download.h"

typedef struct {
    char *url;
    char *filename;
} url_test_t;

typedef struct {
    char *header;
    char *filename;
} header_test_t;

void http_filename_from_url_td(void **state) {
    int num_tests = 5;
    url_test_t tests[] = {
      (url_test_t){
        .url = "https://host.test/image.jpeg",
        .filename = "image.jpeg",
      },
      (url_test_t){
        .url = "https://host.test/images/",
        .filename = "images",
      },
      (url_test_t){
        .url = "https://host.test/",
        .filename = "index.html",
      },
      (url_test_t){
        .url = "https://host.test",
        .filename = "index.html",
      },
      (url_test_t){
        .url = "aesgcm://host.test",
        .filename = "index.html",
      },
    };

    char *filename;
    for(int i = 0; i < num_tests; i++) {
      filename = http_filename_from_url(tests[i].url);
      assert_string_equal(filename, tests[i].filename);
    }
}

void http_filename_from_header_td(void **state) {
    int num_tests = 11;
    header_test_t tests[] = {
      (header_test_t){
        .header = "Content-Disposition: filename=image.jpeg",
        .filename = "image.jpeg",
      },
      (header_test_t){
        .header = "Content-Disposition:filename=image.jpeg",
        .filename = "image.jpeg",
      },
      (header_test_t){
        .header = "CoNteNt-DiSpoSItioN: filename=image.jpeg",
        .filename = "image.jpeg",
      },
      (header_test_t){
        .header = "Content-Disposition: attachment; filename=image.jpeg",
        .filename = "image.jpeg",
      },
      (header_test_t){
        .header = "Content-Disposition: filename=",
        .filename = NULL,
      },
      (header_test_t){
        .header = "Content-Disposition: filename=;",
        .filename = NULL,
      },
      (header_test_t){
        .header = "Content-Disposition: inline",
        .filename = NULL,
      },
      (header_test_t){
        .header = "Content-Disposition:",
        .filename = NULL,
      },
      (header_test_t){
        .header = "Last-Modified: Wed, 21 Oct 2015 07:28:00 GMT ",
        .filename = NULL,
      },
      (header_test_t){
        .header = "",
        .filename = NULL,
      },
      (header_test_t){
        .header = NULL,
        .filename = NULL,
      },
    };

    char *got_filename;
    char *exp_filename;
    char *header;
    for(int i = 0; i < num_tests; i++) {
        header = tests[i].header;
        exp_filename = tests[i].filename;

        got_filename = http_filename_from_header(header);

        if (exp_filename == NULL) {
            assert_null(got_filename);
        } else {
            assert_string_equal(got_filename, exp_filename);
        }
    }
}
