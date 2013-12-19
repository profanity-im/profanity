#ifndef MOCK_UI_H
#define MICK_UI_H

#include <glib.h>
#include <setjmp.h>
#include <cmocka.h>

void stub_cons_show(void);

void mock_cons_show(void);
void expect_cons_show(char *output);
void expect_cons_show_calls(int n);

#endif
