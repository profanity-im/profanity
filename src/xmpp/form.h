/*
 * form.h
 * vim: expandtab:ts=4:sts=4:sw=4
 *
 * Copyright (C) 2012 - 2019 James Booth <boothj5@gmail.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later WITH OpenSSL-exception
 */

#ifndef XMPP_FORM_H
#define XMPP_FORM_H

#include <strophe.h>

#include "xmpp/xmpp.h"

DataForm* form_create(xmpp_stanza_t* const stanza);
DataForm* form_create_empty(const gchar* type, const gchar* title, const gchar* instructions);
void form_add_field_text(DataForm* form, const gchar* var, const gchar* label, const gchar* value);
const gchar* form_get_field_value(DataForm* form, const gchar* var);
xmpp_stanza_t* form_create_submission(DataForm* form);
gchar* form_get_form_type_field(DataForm* form);
GSList* form_get_non_form_type_fields_sorted(DataForm* form);
GSList* form_get_field_values_sorted(FormField* field);

#endif
