/*
 * form.c
 *
 * Copyright (C) 2012 - 2017 James Booth <boothj5@gmail.com>
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
 * along with Profanity.  If not, see <https://www.gnu.org/licenses/>.
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

#include "config.h"

#include <string.h>
#include <stdlib.h>

#ifdef HAVE_LIBMESODE
#include <mesode.h>
#endif

#ifdef HAVE_LIBSTROPHE
#include <strophe.h>
#endif

#include <glib.h>

#include "log.h"
#include "xmpp/xmpp.h"
#include "xmpp/stanza.h"
#include "xmpp/connection.h"

static gboolean
_is_valid_form_element(xmpp_stanza_t *stanza)
{
    const char *name = xmpp_stanza_get_name(stanza);
    if (g_strcmp0(name, STANZA_NAME_X) != 0) {
        log_error("Error parsing form, root element not <x/>.");
        return FALSE;
    }

    const char *ns = xmpp_stanza_get_ns(stanza);
    if (g_strcmp0(ns, STANZA_NS_DATA) != 0) {
        log_error("Error parsing form, namespace not %s.", STANZA_NS_DATA);
        return FALSE;
    }

    const char *type = xmpp_stanza_get_type(stanza);
    if ((g_strcmp0(type, "form") != 0) &&
            (g_strcmp0(type, "submit") != 0) &&
            (g_strcmp0(type, "cancel") != 0) &&
            (g_strcmp0(type, "result") != 0)) {
        log_error("Error parsing form, unknown type.");
        return FALSE;
    }

    return TRUE;
}

static DataForm*
_form_new(void)
{
    DataForm *form = malloc(sizeof(DataForm));
    form->type = NULL;
    form->title = NULL;
    form->instructions = NULL;
    form->fields = NULL;
    form->var_to_tag = NULL;
    form->tag_to_var = NULL;
    form->tag_ac = NULL;

    return form;
}

static FormField*
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

static char*
_get_property(xmpp_stanza_t *const stanza, const char *const property)
{
    char *result = NULL;
    xmpp_ctx_t *ctx = connection_get_ctx();

    xmpp_stanza_t *child = xmpp_stanza_get_child_by_name(stanza, property);
    if (child) {
        char *child_text = xmpp_stanza_get_text(child);
        if (child_text) {
            result = strdup(child_text);
            xmpp_free(ctx, child_text);
        }
    }

    return result;
}

static char*
_get_attr(xmpp_stanza_t *const stanza, const char *const attr)
{
    const char *result = xmpp_stanza_get_attribute(stanza, attr);
    if (result) {
        return strdup(result);
    } else {
        return NULL;
    }
}

static gboolean
_is_required(xmpp_stanza_t *const stanza)
{
    xmpp_stanza_t *child = xmpp_stanza_get_child_by_name(stanza, "required");
    if (child) {
        return TRUE;
    } else {
        return FALSE;
    }
}

static form_field_type_t
_get_field_type(const char *const type)
{
    if (g_strcmp0(type, "hidden") == 0) {
        return FIELD_HIDDEN;
    }
    if (g_strcmp0(type, "text-single") == 0) {
        return FIELD_TEXT_SINGLE;
    }
    if (g_strcmp0(type, "text-private") == 0) {
        return FIELD_TEXT_PRIVATE;
    }
    if (g_strcmp0(type, "text-multi") == 0) {
        return FIELD_TEXT_MULTI;
    }
    if (g_strcmp0(type, "boolean") == 0) {
        return FIELD_BOOLEAN;
    }
    if (g_strcmp0(type, "list-single") == 0) {
        return FIELD_LIST_SINGLE;
    }
    if (g_strcmp0(type, "list-multi") == 0) {
        return FIELD_LIST_MULTI;
    }
    if (g_strcmp0(type, "jid-single") == 0) {
        return FIELD_JID_SINGLE;
    }
    if (g_strcmp0(type, "jid-multi") == 0) {
        return FIELD_JID_MULTI;
    }
    if (g_strcmp0(type, "fixed") == 0) {
        return FIELD_FIXED;
    }
    return FIELD_UNKNOWN;
}

DataForm*
form_create(xmpp_stanza_t *const form_stanza)
{
    xmpp_ctx_t *ctx = connection_get_ctx();

    if (!_is_valid_form_element(form_stanza)) {
        return NULL;
    }

    DataForm *form = _form_new();
    form->type = _get_attr(form_stanza, "type");
    form->title = _get_property(form_stanza, "title");
    form->instructions = _get_property(form_stanza, "instructions");
    form->var_to_tag = g_hash_table_new_full(g_str_hash, g_str_equal, free, free);
    form->tag_to_var = g_hash_table_new_full(g_str_hash, g_str_equal, free, free);
    form->tag_ac = autocomplete_new();
    form->modified = FALSE;

    int tag_num = 1;

    // get fields
    xmpp_stanza_t *form_child = xmpp_stanza_get_children(form_stanza);
    while (form_child) {
        const char *child_name = xmpp_stanza_get_name(form_child);
        if (g_strcmp0(child_name, "field") == 0) {
            xmpp_stanza_t *field_stanza = form_child;

            FormField *field = _field_new();
            field->label = _get_attr(field_stanza, "label");
            field->type = _get_attr(field_stanza, "type");
            field->type_t = _get_field_type(field->type);
            field->value_ac = autocomplete_new();

            field->var = _get_attr(field_stanza, "var");

            if (field->type_t != FIELD_HIDDEN && field->var) {
                GString *tag = g_string_new("");
                g_string_printf(tag, "field%d", tag_num++);
                g_hash_table_insert(form->var_to_tag, strdup(field->var), strdup(tag->str));
                g_hash_table_insert(form->tag_to_var, strdup(tag->str), strdup(field->var));
                autocomplete_add(form->tag_ac, tag->str);
                g_string_free(tag, TRUE);
            }

            field->description = _get_property(field_stanza, "desc");
            field->required = _is_required(field_stanza);

            // handle repeated field children
            xmpp_stanza_t *field_child = xmpp_stanza_get_children(field_stanza);
            int value_index = 1;
            while (field_child) {
                child_name = xmpp_stanza_get_name(field_child);

                // handle values
                if (g_strcmp0(child_name, "value") == 0) {
                    char *value = xmpp_stanza_get_text(field_child);
                    if (value) {
                        field->values = g_slist_append(field->values, strdup(value));

                        if (field->type_t == FIELD_TEXT_MULTI) {
                            GString *ac_val = g_string_new("");
                            g_string_printf(ac_val, "val%d", value_index++);
                            autocomplete_add(field->value_ac, ac_val->str);
                            g_string_free(ac_val, TRUE);
                        }
                        if (field->type_t == FIELD_JID_MULTI) {
                            autocomplete_add(field->value_ac, value);
                        }

                        xmpp_free(ctx, value);
                    }

                // handle options
                } else if (g_strcmp0(child_name, "option") == 0) {
                    FormOption *option = malloc(sizeof(FormOption));
                    option->label = _get_attr(field_child, "label");
                    option->value = _get_property(field_child, "value");

                    if ((field->type_t == FIELD_LIST_SINGLE) || (field->type_t == FIELD_LIST_MULTI)) {
                        autocomplete_add(field->value_ac, option->value);
                    }

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

xmpp_stanza_t*
form_create_submission(DataForm *form)
{
    xmpp_ctx_t *ctx = connection_get_ctx();

    xmpp_stanza_t *x = xmpp_stanza_new(ctx);
    xmpp_stanza_set_name(x, STANZA_NAME_X);
    xmpp_stanza_set_ns(x, STANZA_NS_DATA);
    xmpp_stanza_set_type(x, "submit");

    GSList *curr_field = form->fields;
    while (curr_field) {
        FormField *field = curr_field->data;

        if (field->type_t != FIELD_FIXED) {
            xmpp_stanza_t *field_stanza = xmpp_stanza_new(ctx);
            xmpp_stanza_set_name(field_stanza, "field");
            xmpp_stanza_set_attribute(field_stanza, "var", field->var);

            xmpp_stanza_t *value_stanza = NULL;
            GSList *curr_value = NULL;

            switch (field->type_t) {
                case FIELD_HIDDEN:
                case FIELD_TEXT_SINGLE:
                case FIELD_TEXT_PRIVATE:
                case FIELD_BOOLEAN:
                case FIELD_LIST_SINGLE:
                case FIELD_JID_SINGLE:
                    value_stanza = xmpp_stanza_new(ctx);
                    xmpp_stanza_set_name(value_stanza, "value");
                    if (field->values) {
                        if (field->values->data) {
                            xmpp_stanza_t *text_stanza = xmpp_stanza_new(ctx);
                            xmpp_stanza_set_text(text_stanza, field->values->data);
                            xmpp_stanza_add_child(value_stanza, text_stanza);
                            xmpp_stanza_release(text_stanza);
                        }
                    }
                    xmpp_stanza_add_child(field_stanza, value_stanza);
                    xmpp_stanza_release(value_stanza);

                    break;

                case FIELD_TEXT_MULTI:
                case FIELD_LIST_MULTI:
                case FIELD_JID_MULTI:
                    curr_value = field->values;
                    while (curr_value) {
                        char *value = curr_value->data;

                        value_stanza = xmpp_stanza_new(ctx);
                        xmpp_stanza_set_name(value_stanza, "value");
                        if (value) {
                            xmpp_stanza_t *text_stanza = xmpp_stanza_new(ctx);
                            xmpp_stanza_set_text(text_stanza, value);
                            xmpp_stanza_add_child(value_stanza, text_stanza);
                            xmpp_stanza_release(text_stanza);
                        }

                        xmpp_stanza_add_child(field_stanza, value_stanza);
                        xmpp_stanza_release(value_stanza);

                        curr_value = g_slist_next(curr_value);
                    }
                    break;
                case FIELD_FIXED:
                default:
                    break;
            }

            xmpp_stanza_add_child(x, field_stanza);
            xmpp_stanza_release(field_stanza);
        }

        curr_field = g_slist_next(curr_field);
    }

    return x;
}

static void
_free_option(FormOption *option)
{
    if (option) {
        free(option->label);
        free(option->value);
        free(option);
    }
}

static void
_free_field(FormField *field)
{
    if (field) {
        free(field->label);
        free(field->type);
        free(field->var);
        free(field->description);
        g_slist_free_full(field->values, free);
        g_slist_free_full(field->options, (GDestroyNotify)_free_option);
        autocomplete_free(field->value_ac);
        free(field);
    }
}

void
form_destroy(DataForm *form)
{
    if (form) {
        free(form->type);
        free(form->title);
        free(form->instructions);
        g_slist_free_full(form->fields, (GDestroyNotify)_free_field);
        g_hash_table_destroy(form->var_to_tag);
        g_hash_table_destroy(form->tag_to_var);
        autocomplete_free(form->tag_ac);
        free(form);
    }
}

static int
_field_compare_by_var(FormField *a, FormField *b)
{
    return g_strcmp0(a->var, b->var);
}

GSList*
form_get_non_form_type_fields_sorted(DataForm *form)
{
    GSList *sorted = NULL;
    GSList *curr = form->fields;
    while (curr) {
        FormField *field = curr->data;
        if (g_strcmp0(field->var, "FORM_TYPE") != 0) {
            sorted = g_slist_insert_sorted(sorted, field, (GCompareFunc)_field_compare_by_var);
        }
        curr = g_slist_next(curr);
    }

    return sorted;
}

GSList*
form_get_field_values_sorted(FormField *field)
{
    GSList *sorted = NULL;
    GSList *curr = field->values;
    while (curr) {
        char *value = curr->data;
        if (value) {
            sorted = g_slist_insert_sorted(sorted, value, (GCompareFunc)g_strcmp0);
        }
        curr = g_slist_next(curr);
    }

    return sorted;
}

char*
form_get_form_type_field(DataForm *form)
{
    GSList *curr = form->fields;
    while (curr) {
        FormField *field = curr->data;
        if (g_strcmp0(field->var, "FORM_TYPE") == 0) {
            return field->values->data;
        }
        curr = g_slist_next(curr);
    }

    return NULL;
}

gboolean
form_tag_exists(DataForm *form, const char *const tag)
{
    GList *tags = g_hash_table_get_keys(form->tag_to_var);
    GList *curr = tags;
    while (curr) {
        if (g_strcmp0(curr->data, tag) == 0) {
            return TRUE;
        }
        curr = g_list_next(curr);
    }

    g_list_free(tags);
    return FALSE;
}

form_field_type_t
form_get_field_type(DataForm *form, const char *const tag)
{
    char *var = g_hash_table_lookup(form->tag_to_var, tag);
    if (var) {
        GSList *curr = form->fields;
        while (curr) {
            FormField *field = curr->data;
            if (g_strcmp0(field->var, var) == 0) {
                return field->type_t;
            }
            curr = g_slist_next(curr);
        }
    }
    return FIELD_UNKNOWN;
}

void
form_set_value(DataForm *form, const char *const tag, char *value)
{
    char *var = g_hash_table_lookup(form->tag_to_var, tag);
    if (var) {
        GSList *curr = form->fields;
        while (curr) {
            FormField *field = curr->data;
            if (g_strcmp0(field->var, var) == 0) {
                if (g_slist_length(field->values) == 0) {
                    field->values = g_slist_append(field->values, strdup(value));
                    form->modified = TRUE;
                    return;
                } else if (g_slist_length(field->values) == 1) {
                    free(field->values->data);
                    field->values->data = strdup(value);
                    form->modified = TRUE;
                    return;
                }
            }
            curr = g_slist_next(curr);
        }
    }
}

void
form_add_value(DataForm *form, const char *const tag, char *value)
{
    char *var = g_hash_table_lookup(form->tag_to_var, tag);
    if (var) {
        GSList *curr = form->fields;
        while (curr) {
            FormField *field = curr->data;
            if (g_strcmp0(field->var, var) == 0) {
                field->values = g_slist_append(field->values, strdup(value));
                if (field->type_t == FIELD_TEXT_MULTI) {
                    int total = g_slist_length(field->values);
                    GString *value_index = g_string_new("");
                    g_string_printf(value_index, "val%d", total);
                    autocomplete_add(field->value_ac, value_index->str);
                    g_string_free(value_index, TRUE);
                }
                form->modified = TRUE;
                return;
            }
            curr = g_slist_next(curr);
        }
    }
}

gboolean
form_add_unique_value(DataForm *form, const char *const tag, char *value)
{
    char *var = g_hash_table_lookup(form->tag_to_var, tag);
    if (var) {
        GSList *curr = form->fields;
        while (curr) {
            FormField *field = curr->data;
            if (g_strcmp0(field->var, var) == 0) {
                GSList *curr_value = field->values;
                while (curr_value) {
                    if (g_strcmp0(curr_value->data, value) == 0) {
                        return FALSE;
                    }
                    curr_value = g_slist_next(curr_value);
                }

                field->values = g_slist_append(field->values, strdup(value));
                if (field->type_t == FIELD_JID_MULTI) {
                    autocomplete_add(field->value_ac, value);
                }
                form->modified = TRUE;
                return TRUE;
            }
            curr = g_slist_next(curr);
        }
    }

    return FALSE;
}

gboolean
form_remove_value(DataForm *form, const char *const tag, char *value)
{
    char *var = g_hash_table_lookup(form->tag_to_var, tag);
    if (var) {
        GSList *curr = form->fields;
        while (curr) {
            FormField *field = curr->data;
            if (g_strcmp0(field->var, var) == 0) {
                GSList *found = g_slist_find_custom(field->values, value, (GCompareFunc)g_strcmp0);
                if (found) {
                    free(found->data);
                    found->data = NULL;
                    field->values = g_slist_delete_link(field->values, found);
                    if (field->type_t == FIELD_JID_MULTI) {
                        autocomplete_remove(field->value_ac, value);
                    }
                    form->modified = TRUE;
                    return TRUE;
                } else {
                    return FALSE;
                }
            }
            curr = g_slist_next(curr);
        }
    }

    return FALSE;
}

gboolean
form_remove_text_multi_value(DataForm *form, const char *const tag, int index)
{
    index--;
    char *var = g_hash_table_lookup(form->tag_to_var, tag);
    if (var) {
        GSList *curr = form->fields;
        while (curr) {
            FormField *field = curr->data;
            if (g_strcmp0(field->var, var) == 0) {
                GSList *item = g_slist_nth(field->values, index);
                if (item) {
                    free(item->data);
                    item->data = NULL;
                    field->values = g_slist_delete_link(field->values, item);
                    GString *value_index = g_string_new("");
                    g_string_printf(value_index, "val%d", index+1);
                    autocomplete_remove(field->value_ac, value_index->str);
                    g_string_free(value_index, TRUE);
                    form->modified = TRUE;
                    return TRUE;
                } else {
                    return FALSE;
                }
            }
            curr = g_slist_next(curr);
        }
    }

    return FALSE;
}

int
form_get_value_count(DataForm *form, const char *const tag)
{
    char *var = g_hash_table_lookup(form->tag_to_var, tag);
    if (var) {
        GSList *curr = form->fields;
        while (curr) {
            FormField *field = curr->data;
            if (g_strcmp0(field->var, var) == 0) {
                if ((g_slist_length(field->values) == 1) && (field->values->data == NULL)) {
                    return 0;
                } else {
                    return g_slist_length(field->values);
                }
            }
            curr = g_slist_next(curr);
        }
    }

    return 0;
}

gboolean
form_field_contains_option(DataForm *form, const char *const tag, char *value)
{
    char *var = g_hash_table_lookup(form->tag_to_var, tag);
    if (var) {
        GSList *curr = form->fields;
        while (curr) {
            FormField *field = curr->data;
            if (g_strcmp0(field->var, var) == 0) {
                GSList *curr_option = field->options;
                while (curr_option) {
                    FormOption *option = curr_option->data;
                    if (g_strcmp0(option->value, value) == 0) {
                        return TRUE;
                    }
                    curr_option = g_slist_next(curr_option);
                }
            }
            curr = g_slist_next(curr);
        }
    }

    return FALSE;
}

FormField*
form_get_field_by_tag(DataForm *form, const char *const tag)
{
    char *var = g_hash_table_lookup(form->tag_to_var, tag);
    if (var) {
        GSList *curr = form->fields;
        while (curr) {
            FormField *field = curr->data;
            if (g_strcmp0(field->var, var) == 0) {
                return field;
            }
            curr = g_slist_next(curr);
        }
    }
    return NULL;
}

Autocomplete
form_get_value_ac(DataForm *form, const char *const tag)
{
    char *var = g_hash_table_lookup(form->tag_to_var, tag);
    if (var) {
        GSList *curr = form->fields;
        while (curr) {
            FormField *field = curr->data;
            if (g_strcmp0(field->var, var) == 0) {
                return field->value_ac;
            }
            curr = g_slist_next(curr);
        }
    }
    return NULL;
}

void
form_reset_autocompleters(DataForm *form)
{
    autocomplete_reset(form->tag_ac);
    GSList *curr_field = form->fields;
    while (curr_field) {
        FormField *field = curr_field->data;
        autocomplete_reset(field->value_ac);
        curr_field = g_slist_next(curr_field);
    }
}
