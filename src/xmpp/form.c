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
#include <glib.h>

#include "log.h"
#include "xmpp/xmpp.h"
#include "xmpp/stanza.h"
#include "xmpp/connection.h"

static gboolean
_is_valid_form_element(xmpp_stanza_t *stanza)
{
    char *name = xmpp_stanza_get_name(stanza);
    if (g_strcmp0(name, STANZA_NAME_X) != 0) {
        log_error("Error parsing form, root element not <x/>.");
        return FALSE;
    }

    char *ns = xmpp_stanza_get_ns(stanza);
    if (g_strcmp0(ns, STANZA_NS_DATA) != 0) {
        log_error("Error parsing form, namespace not %s.", STANZA_NS_DATA);
        return FALSE;
    }

    char *type = xmpp_stanza_get_attribute(stanza, STANZA_ATTR_TYPE);
    if ((g_strcmp0(type, "form") != 0) &&
            (g_strcmp0(type, "submit") != 0) &&
            (g_strcmp0(type, "cancel") != 0) &&
            (g_strcmp0(type, "result") != 0)) {
        log_error("Error parsing form, unknown type.");
        return FALSE;
    }

    return TRUE;
}

static DataForm *
_form_new(void)
{
    DataForm *form = malloc(sizeof(DataForm));
    form->type = NULL;
    form->title = NULL;
    form->instructions = NULL;
    form->fields = NULL;

    return form;
}

static FormField *
_field_new(void)
{
    FormField *field = malloc(sizeof(FormField));
    field->label = NULL;
    field->type = NULL;
    field->var = NULL;
    field->description = NULL;
    field->required = FALSE;
    field->values = NULL;
    field->options = NULL;

    return field;
}

static char *
_get_property(xmpp_stanza_t * const stanza, const char * const property)
{
    char *result = NULL;
    xmpp_ctx_t *ctx = connection_get_ctx();

    xmpp_stanza_t *child = xmpp_stanza_get_child_by_name(stanza, property);
    if (child != NULL) {
        char *child_text = xmpp_stanza_get_text(child);
        if (child_text != NULL) {
            result = strdup(child_text);
            xmpp_free(ctx, child_text);
        }
    }

    return result;
}

static char *
_get_attr(xmpp_stanza_t * const stanza, const char * const attr)
{
    char *result = xmpp_stanza_get_attribute(stanza, attr);
    if (result != NULL) {
        return strdup(result);
    } else {
        return NULL;
    }
}

static gboolean
_is_required(xmpp_stanza_t * const stanza)
{
    xmpp_stanza_t *child = xmpp_stanza_get_child_by_name(stanza, "required");
    if (child != NULL) {
        return TRUE;
    } else {
        return FALSE;
    }
}

DataForm *
form_create(xmpp_stanza_t * const form_stanza)
{
    xmpp_ctx_t *ctx = connection_get_ctx();

    if (!_is_valid_form_element(form_stanza)) {
        return NULL;
    }

    DataForm *form = _form_new();
    form->type = _get_attr(form_stanza, "type");
    form->title = _get_property(form_stanza, "title");
    form->instructions = _get_property(form_stanza, "instructions");

    // get fields
    xmpp_stanza_t *form_child = xmpp_stanza_get_children(form_stanza);
    while (form_child != NULL) {
        char *child_name = xmpp_stanza_get_name(form_child);
        if (g_strcmp0(child_name, "field") == 0) {
            xmpp_stanza_t *field_stanza = form_child;

            FormField *field = _field_new();
            field->label = _get_attr(field_stanza, "label");
            field->type = _get_attr(field_stanza, "type");
            field->var = _get_attr(field_stanza, "var");
            field->description = _get_property(field_stanza, "desc");
            field->required = _is_required(field_stanza);

            // handle repeated field children
            xmpp_stanza_t *field_child = xmpp_stanza_get_children(field_stanza);
            while (field_child != NULL) {
                child_name = xmpp_stanza_get_name(field_child);

                // handle values
                if (g_strcmp0(child_name, "value") == 0) {
                    char *value = xmpp_stanza_get_text(field_child);
                    if (value != NULL) {
                        field->values = g_slist_append(field->values, strdup(value));
                        xmpp_free(ctx, value);
                    }

                // handle options
                } else if (g_strcmp0(child_name, "option") == 0) {
                    FormOption *option = malloc(sizeof(FormOption));
                    option->label = _get_attr(field_child, "label");
                    option->value = _get_property(field_child, "value");

                    field->options = g_slist_append(field->options, option);
                }

                field_child = xmpp_stanza_get_next(field_child);
            }

            form->fields = g_slist_append(form->fields, field);
        }

        form_child = xmpp_stanza_get_next(form_child);
    }

    return form;
}

static void
_free_option(FormOption *option)
{
    if (option != NULL) {
        if (option->label != NULL) {
            free(option->label);
        }
        if (option->value != NULL) {
            free(option->value);
        }

        free(option);
    }
}

static void
_free_field(FormField *field)
{
    if (field != NULL) {
        if (field->label != NULL) {
            free(field->label);
        }
        if (field->type != NULL) {
            free(field->type);
        }
        if (field->var != NULL) {
            free(field->var);
        }
        if (field->description != NULL) {
            free(field->description);
        }
        if (field->values != NULL) {
            g_slist_free_full(field->values, free);
        }
        if (field->options != NULL) {
            g_slist_free_full(field->options, (GDestroyNotify)_free_option);
        }

        free(field);
    }
}

static void
_form_destroy(DataForm *form)
{
    if (form != NULL) {
        if (form->type != NULL) {
            free(form->type);
        }
        if (form->title != NULL) {
            free(form->title);
        }
        if (form->instructions != NULL) {
            free(form->instructions);
        }

        if (form->fields != NULL) {
            g_slist_free_full(form->fields, (GDestroyNotify)_free_field);
        }

        free(form);
    }
}

static char *
_form_get_field_by_var(DataForm *form, const char * const var)
{
    GSList *curr = form->fields;
    while (curr != NULL) {
        FormField *field = curr->data;
        if (g_strcmp0(field->var, var) == 0) {
            return field->values->data;
        }
        curr = g_slist_next(curr);
    }

    return NULL;
}

void
form_init_module(void)
{
    form_destroy = _form_destroy;
    form_get_field_by_var = _form_get_field_by_var;
}
