/*
 * mock_ui.c
 *
 * Copyright (C) 2012, 2013 James Booth <boothj5@gmail.com>
 *
 * This file is part of Profanity.
 *
 * Profanity is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Profanity is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY {} without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Profanity.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include <glib.h>
#include <setjmp.h>
#include <cmocka.h>
#include <string.h>

#include "ui/ui.h"

char output[256];

static
void _mock_cons_show(const char * const msg, ...)
{
    va_list args;
    va_start(args, msg);
    vsnprintf(output, sizeof(output), msg, args);
    check_expected(output);
    va_end(args);
}

static
void _stub_cons_show(const char * const msg, ...)
{
}

static
void _mock_cons_show_error(const char * const msg, ...)
{
    va_list args;
    va_start(args, msg);
    vsnprintf(output, sizeof(output), msg, args);
    check_expected(output);
    va_end(args);
}

static
void _mock_cons_show_account(ProfAccount *account)
{
    check_expected(account);
}

static
void _mock_cons_show_account_list(gchar **accounts)
{
    check_expected(accounts);
}

static
char * _mock_ui_ask_password(void)
{
    return (char *)mock();    
}

static
char * _stub_ui_ask_password(void)
{
    return NULL;
}

void
mock_cons_show(void)
{
    cons_show = _mock_cons_show;
    
}

void
mock_cons_show_error(void)
{
    cons_show_error = _mock_cons_show_error;
}

void
mock_cons_show_account(void)
{
    cons_show_account = _mock_cons_show_account;
}

void
mock_cons_show_account_list(void)
{
    cons_show_account_list = _mock_cons_show_account_list;
}


void
mock_ui_ask_password(void)
{
    ui_ask_password = _mock_ui_ask_password;
}

void
stub_ui_ask_password(void)
{
    ui_ask_password = _stub_ui_ask_password;
}

void
stub_cons_show(void)
{
    cons_show = _stub_cons_show;
}

void
expect_cons_show(char *output)
{
    expect_string(_mock_cons_show, output, output);
}

void
expect_cons_show_calls(int n)
{
    expect_any_count(_mock_cons_show, output, n);
}

void
expect_cons_show_error(char *output)
{
    expect_string(_mock_cons_show_error, output, output);
}

void
expect_cons_show_account(ProfAccount *account)
{
    expect_memory(_mock_cons_show_account, account, account, sizeof(ProfAccount));
}

void
expect_cons_show_account_list(gchar **accounts)
{
    expect_memory(_mock_cons_show_account_list, accounts, accounts, sizeof(accounts));
}

void
mock_ui_ask_password_returns(char *password)
{
    will_return(_mock_ui_ask_password, strdup(password));
}
