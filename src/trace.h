/*
 * trace.h
 * vim: expandtab:ts=4:sts=4:sw=4
 *
 * Profanity - Copyright (C) 2012-2023 contributors
 * License: GPLv3 (https://www.gnu.org/licenses/)
 * Exception: Portions linked with OpenSSL (see individual files)
 *
 * Macros to print stack trace and custom assert that will print error in log.
 *
 * Inspired by pppoat2's trace.h
 */

#ifndef _PROF_TRACE_H
#define _PROF_TRACE_H

#include "log.h"

#include <execinfo.h>
#include <errno.h>
#include <stdlib.h>

#define TRACE_LOC_F "%s:%d: %s():"
#define TRACE_LOC_P __FILE__, __LINE__, __func__

#define BT_BUF_SIZE 100

#define PRINT_STACK_TRACE()                                                \
    do {                                                                   \
        void* stack_trace[BT_BUF_SIZE];                                    \
        int stack_size = backtrace(stack_trace, BT_BUF_SIZE);              \
        char** stack_symbols = backtrace_symbols(stack_trace, stack_size); \
        if (stack_symbols == NULL) {                                       \
            log_error("Unable to fetch callstack.");                       \
            break;                                                         \
        }                                                                  \
        log_error("Stack Trace:");                                         \
        for (int i = 0; i < stack_size; ++i) {                             \
            log_error("%s", stack_symbols[i]);                             \
        }                                                                  \
        free(stack_symbols);                                               \
    } while (0)

#define PROF_ASSERT_INFO_IMPL(expr, fmt, ...)                          \
    do {                                                               \
        const char* __fmt = (fmt);                                     \
        if (!(expr)) {                                                 \
            log_error(TRACE_LOC_F " Assertion '%s' failed (errno=%d)", \
                      TRACE_LOC_P, #expr, errno);                      \
            if (__fmt != NULL) {                                       \
                log_error(__fmt, ##__VA_ARGS__);                       \
            }                                                          \
            PRINT_STACK_TRACE();                                       \
            abort();                                                   \
        }                                                              \
    } while (0)

#define PROF_ASSERT_INFO(expr, fmt, ...) PROF_ASSERT_INFO_IMPL((expr), (fmt), ##__VA_ARGS__)

#define PROF_ASSERT(expr) PROF_ASSERT_INFO(expr, NULL)

#endif