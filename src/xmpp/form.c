/*
 * form.c
 *
 * Copyright (C) 2012 - 2014 James Booth <boothj5@gmail.com>
 *
 * This file is part of Profanity.
 *
 * Profanity is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Profanity is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Profanity.  If not, see <http://www.gnu.org/licenses/>.
 *
 * In addition, as a special exception, the copyright holders give permission to
 * link the code of portions of this program with the OpenSSL library under
 * certain conditions as described in each individual source file, and
 * distribute linked combinations including the two.
 *
 * You must obey the GNU General Public License in all respects for all of the
 * code used other than OpenSSL. If you modify file(s) with this exception, you
 * may extend this exception to your version of the file(s), but you are not
 * obligated to do so. If you do not wish to do so, delete this exception
 * statement from your version. If you delete this exception statement from all
 * source files in the program, then also delete it here.
 *
 */

#include <string.h>
#include <stdlib.h>

#include <strophe.h>

#include "xmpp/xmpp.h"
#include "xmpp/connection.h"

static int _field_compare(FormField *f1, FormField *f2);

DataForm *
form_create(xmpp_stanza_t * const stanza)
{
    DataForm *result = NULL;
    xmpp_ctx_t *ctx = connection_get_ctx();

    xmpp_stanza_t *child = xmpp_stanza_get_children(stanza);

    if (child != NULL) {
        result = malloc(sizeof(DataForm));
        result->form_type = NULL;
        result->fields = NULL;
    }

    //handle fields
    while (child != NULL) {
        char *label = xmpp_stanza_get_attribute(child, "label");
        char *type = xmpp_stanza_get_attribute(child, "type");
        char *var = xmpp_stanza_get_attribute(child, "var");

        // handle FORM_TYPE
        if (g_strcmp0(var, "FORM_TYPE") == 0) {
            xmpp_stanza_t *value = xmpp_stanza_get_child_by_name(child, "value");
            char *value_text = xmpp_stanza_get_text(value);
            if (value_text != NULL) {
                result->form_type = strdup(value_text);
                xmpp_free(ctx, value_text);
            }

        // handle regular fields
        } else {
            FormField *field = malloc(sizeof(FormField));
            field->label = NULL;
            field->type = NULL;
            field->var = NULL;

            if (label != NULL) {
                field->label = strdup(label);
            }
            if (type != NULL) {
                field->type = strdup(type);
            }
            if (var != NULL) {
                field->var = strdup(var);
            }

            // handle values
            field->values = NULL;
            xmpp_stanza_t *value = xmpp_stanza_get_children(child);
            while (value != NULL) {
                char *text = xmpp_stanza_get_text(value);
                if (text != NULL) {
                    field->values = g_slist_insert_sorted(field->values, strdup(text), (GCompareFunc)strcmp);
                    xmpp_free(ctx, text);
                }
                value = xmpp_stanza_get_next(value);
            }

            result->fields = g_slist_insert_sorted(result->fields, field, (GCompareFunc)_field_compare);
        }

        child = xmpp_stanza_get_next(child);
    }

    return result;
}

void
_form_destroy(DataForm *form)
{
    if (form != NULL) {
        if (form->fields != NULL) {
            GSList *curr_field = form->fields;
            while (curr_field != NULL) {
                FormField *field = curr_field->data;
                free(field->var);
                if ((field->values) != NULL) {
                    g_slist_free_full(field->values, free);
                }
                curr_field = curr_field->next;
            }
            g_slist_free_full(form->fields, free);
        }

        free(form->form_type);
        free(form);
    }
}

static int
_field_compare(FormField *f1, FormField *f2)
{
    return strcmp(f1->var, f2->var);
}

void
form_init_module(void)
{
    form_destroy = _form_destroy;
}
