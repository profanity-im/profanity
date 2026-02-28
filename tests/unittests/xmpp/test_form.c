#include <string.h>
#include "prof_cmocka.h"
#include <stdlib.h>

#include "xmpp/form.h"

xmpp_ctx_t*
connection_get_ctx(void)
{
    return NULL;
}

static DataForm*
_new_form(void)
{
    DataForm* form = g_new0(DataForm, 1);
    form->type = NULL;
    form->title = NULL;
    form->instructions = NULL;
    form->fields = NULL;
    form->var_to_tag = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, g_free);
    form->tag_to_var = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, g_free);
    form->tag_ac = NULL;

    return form;
}

static FormField*
_new_field(void)
{
    FormField* field = g_new0(FormField, 1);
    field->label = NULL;
    field->type = NULL;
    field->description = NULL;
    field->required = FALSE;
    field->options = NULL;
    field->var = NULL;
    field->values = NULL;
    field->value_ac = NULL;

    return field;
}

void
form_get_form_type_field__returns__null_no_fields(void** state)
{
    DataForm* form = _new_form();

    char* result = form_get_form_type_field(form);

    assert_null(result);

    form_destroy(form);
}

void
form_get_form_type_field__returns__null_when_not_present(void** state)
{
    DataForm* form = _new_form();
    FormField* field = _new_field();
    field->var = g_strdup("var1");
    field->values = g_slist_append(field->values, g_strdup("value1"));
    form->fields = g_slist_append(form->fields, field);

    char* result = form_get_form_type_field(form);

    assert_null(result);

    form_destroy(form);
}

void
form_get_form_type_field__returns__value_when_present(void** state)
{
    DataForm* form = _new_form();

    FormField* field1 = _new_field();
    field1->var = g_strdup("var1");
    field1->values = g_slist_append(field1->values, g_strdup("value1"));
    form->fields = g_slist_append(form->fields, field1);

    FormField* field2 = _new_field();
    field2->var = g_strdup("FORM_TYPE");
    field2->values = g_slist_append(field2->values, g_strdup("value2"));
    form->fields = g_slist_append(form->fields, field2);

    FormField* field3 = _new_field();
    field3->var = g_strdup("var3");
    field3->values = g_slist_append(field3->values, g_strdup("value3"));
    form->fields = g_slist_append(form->fields, field3);

    char* result = form_get_form_type_field(form);

    assert_string_equal(result, "value2");

    form_destroy(form);
}

void
form_get_field_type__returns__unknown_when_no_fields(void** state)
{
    DataForm* form = _new_form();

    form_field_type_t result = form_get_field_type(form, "tag");

    assert_int_equal(result, FIELD_UNKNOWN);

    form_destroy(form);
}

void
form_get_field_type__returns__correct_type(void** state)
{
    DataForm* form = _new_form();
    g_hash_table_insert(form->tag_to_var, g_strdup("tag1"), g_strdup("var1"));
    g_hash_table_insert(form->tag_to_var, g_strdup("tag2"), g_strdup("var2"));

    FormField* field1 = _new_field();
    field1->var = g_strdup("var1");
    field1->type_t = FIELD_TEXT_SINGLE;
    field1->values = g_slist_append(field1->values, g_strdup("value1"));
    form->fields = g_slist_append(form->fields, field1);

    FormField* field2 = _new_field();
    field2->var = g_strdup("var2");
    field2->type_t = FIELD_TEXT_MULTI;
    field2->values = g_slist_append(field2->values, g_strdup("value2"));
    form->fields = g_slist_append(form->fields, field2);

    form_field_type_t result = form_get_field_type(form, "tag2");

    assert_int_equal(result, FIELD_TEXT_MULTI);

    form_destroy(form);
}

void
form_set_value__updates__adds_when_none(void** state)
{
    DataForm* form = _new_form();
    g_hash_table_insert(form->tag_to_var, g_strdup("tag1"), g_strdup("var1"));
    g_hash_table_insert(form->tag_to_var, g_strdup("tag2"), g_strdup("var2"));

    FormField* field1 = _new_field();
    field1->var = g_strdup("var1");
    field1->type_t = FIELD_TEXT_SINGLE;
    field1->values = g_slist_append(field1->values, g_strdup("value1"));
    form->fields = g_slist_append(form->fields, field1);

    FormField* field2 = _new_field();
    field2->var = g_strdup("var2");
    field2->type_t = FIELD_LIST_SINGLE;
    form->fields = g_slist_append(form->fields, field2);

    form_set_value(form, "tag2", "a new value");

    int length = 0;
    char* value = NULL;
    GSList* curr_field = form->fields;
    while (curr_field != NULL) {
        FormField* field = curr_field->data;
        if (g_strcmp0(field->var, "var2") == 0) {
            length = g_slist_length(field->values);
            value = field->values->data;
            break;
        }
        curr_field = g_slist_next(curr_field);
    }

    assert_int_equal(length, 1);
    assert_string_equal(value, "a new value");

    form_destroy(form);
}

void
form_set_value__updates__updates_when_one(void** state)
{
    DataForm* form = _new_form();
    g_hash_table_insert(form->tag_to_var, g_strdup("tag1"), g_strdup("var1"));
    g_hash_table_insert(form->tag_to_var, g_strdup("tag2"), g_strdup("var2"));

    FormField* field1 = _new_field();
    field1->var = g_strdup("var1");
    field1->type_t = FIELD_TEXT_SINGLE;
    form->fields = g_slist_append(form->fields, field1);

    FormField* field2 = _new_field();
    field2->var = g_strdup("var2");
    field2->type_t = FIELD_LIST_SINGLE;
    field2->values = g_slist_append(field2->values, g_strdup("value2"));
    form->fields = g_slist_append(form->fields, field2);

    form_set_value(form, "tag2", "a new value");

    int length = 0;
    char* value = NULL;
    GSList* curr_field = form->fields;
    while (curr_field != NULL) {
        FormField* field = curr_field->data;
        if (g_strcmp0(field->var, "var2") == 0) {
            length = g_slist_length(field->values);
            value = field->values->data;
            break;
        }
        curr_field = g_slist_next(curr_field);
    }

    assert_int_equal(length, 1);
    assert_string_equal(value, "a new value");

    form_destroy(form);
}

void
form_add_unique_value__updates__adds_when_none(void** state)
{
    DataForm* form = _new_form();
    g_hash_table_insert(form->tag_to_var, g_strdup("tag1"), g_strdup("var1"));
    g_hash_table_insert(form->tag_to_var, g_strdup("tag2"), g_strdup("var2"));

    FormField* field1 = _new_field();
    field1->var = g_strdup("var1");
    field1->type_t = FIELD_JID_MULTI;
    form->fields = g_slist_append(form->fields, field1);

    FormField* field2 = _new_field();
    field2->var = g_strdup("var2");
    field2->type_t = FIELD_LIST_SINGLE;
    field2->values = g_slist_append(field2->values, g_strdup("value2"));
    form->fields = g_slist_append(form->fields, field2);

    gboolean ret = form_add_unique_value(form, "tag1", "me@server.com");

    int length = 0;
    char* value = NULL;
    GSList* curr_field = form->fields;
    while (curr_field != NULL) {
        FormField* field = curr_field->data;
        if (g_strcmp0(field->var, "var1") == 0) {
            length = g_slist_length(field->values);
            value = field->values->data;
            break;
        }
        curr_field = g_slist_next(curr_field);
    }

    assert_true(ret);
    assert_int_equal(length, 1);
    assert_string_equal(value, "me@server.com");

    form_destroy(form);
}

void
form_add_unique_value__updates__does_nothing_when_exists(void** state)
{
    DataForm* form = _new_form();
    g_hash_table_insert(form->tag_to_var, g_strdup("tag1"), g_strdup("var1"));
    g_hash_table_insert(form->tag_to_var, g_strdup("tag2"), g_strdup("var2"));

    FormField* field1 = _new_field();
    field1->var = g_strdup("var1");
    field1->type_t = FIELD_JID_MULTI;
    field1->values = g_slist_append(field1->values, g_strdup("me@server.com"));
    form->fields = g_slist_append(form->fields, field1);

    FormField* field2 = _new_field();
    field2->var = g_strdup("var2");
    field2->type_t = FIELD_LIST_SINGLE;
    field2->values = g_slist_append(field2->values, g_strdup("value2"));
    form->fields = g_slist_append(form->fields, field2);

    gboolean ret = form_add_unique_value(form, "tag1", "me@server.com");

    int length = 0;
    char* value = NULL;
    GSList* curr_field = form->fields;
    while (curr_field != NULL) {
        FormField* field = curr_field->data;
        if (g_strcmp0(field->var, "var1") == 0) {
            length = g_slist_length(field->values);
            value = field->values->data;
            break;
        }
        curr_field = g_slist_next(curr_field);
    }

    assert_false(ret);
    assert_int_equal(length, 1);
    assert_string_equal(value, "me@server.com");

    form_destroy(form);
}

void
form_add_unique_value__updates__adds_when_doesnt_exist(void** state)
{
    DataForm* form = _new_form();
    g_hash_table_insert(form->tag_to_var, g_strdup("tag1"), g_strdup("var1"));
    g_hash_table_insert(form->tag_to_var, g_strdup("tag2"), g_strdup("var2"));

    FormField* field1 = _new_field();
    field1->var = g_strdup("var1");
    field1->type_t = FIELD_JID_MULTI;
    field1->values = g_slist_append(field1->values, g_strdup("dolan@server.com"));
    field1->values = g_slist_append(field1->values, g_strdup("kieran@server.com"));
    field1->values = g_slist_append(field1->values, g_strdup("chi@server.com"));
    form->fields = g_slist_append(form->fields, field1);

    FormField* field2 = _new_field();
    field2->var = g_strdup("var2");
    field2->type_t = FIELD_LIST_SINGLE;
    field2->values = g_slist_append(field2->values, g_strdup("value2"));
    form->fields = g_slist_append(form->fields, field2);

    gboolean ret = form_add_unique_value(form, "tag1", "me@server.com");

    int length = 0;
    int count = 0;
    GSList* curr_field = form->fields;
    while (curr_field != NULL) {
        FormField* field = curr_field->data;
        if (g_strcmp0(field->var, "var1") == 0) {
            length = g_slist_length(field->values);
            GSList* curr_value = field->values;
            while (curr_value != NULL) {
                if (g_strcmp0(curr_value->data, "me@server.com") == 0) {
                    count++;
                }
                curr_value = g_slist_next(curr_value);
            }
            break;
        }
        curr_field = g_slist_next(curr_field);
    }

    assert_true(ret);
    assert_int_equal(length, 4);
    assert_int_equal(count, 1);

    form_destroy(form);
}

void
form_add_value__updates__adds_when_none(void** state)
{
    DataForm* form = _new_form();
    g_hash_table_insert(form->tag_to_var, g_strdup("tag1"), g_strdup("var1"));

    FormField* field1 = _new_field();
    field1->var = g_strdup("var1");
    field1->type_t = FIELD_LIST_MULTI;
    form->fields = g_slist_append(form->fields, field1);

    form_add_value(form, "tag1", "somevalue");

    int length = 0;
    char* value = NULL;
    GSList* curr_field = form->fields;
    while (curr_field != NULL) {
        FormField* field = curr_field->data;
        if (g_strcmp0(field->var, "var1") == 0) {
            length = g_slist_length(field->values);
            value = field->values->data;
            break;
        }
        curr_field = g_slist_next(curr_field);
    }

    assert_int_equal(length, 1);
    assert_string_equal(value, "somevalue");

    form_destroy(form);
}

void
form_add_value__updates__adds_when_some(void** state)
{
    DataForm* form = _new_form();
    g_hash_table_insert(form->tag_to_var, g_strdup("tag1"), g_strdup("var1"));

    FormField* field1 = _new_field();
    field1->var = g_strdup("var1");
    field1->type_t = FIELD_LIST_MULTI;
    field1->values = g_slist_append(field1->values, g_strdup("some text"));
    field1->values = g_slist_append(field1->values, g_strdup("some more text"));
    field1->values = g_slist_append(field1->values, g_strdup("yet some more text"));
    form->fields = g_slist_append(form->fields, field1);

    form_add_value(form, "tag1", "new value");

    int num_values = 0;
    int new_value_count = 0;
    GSList* curr_field = form->fields;
    while (curr_field != NULL) {
        FormField* field = curr_field->data;
        if (g_strcmp0(field->var, "var1") == 0) {
            GSList* curr_value = field->values;
            while (curr_value != NULL) {
                num_values++;
                if (g_strcmp0(curr_value->data, "new value") == 0) {
                    new_value_count++;
                }
                curr_value = g_slist_next(curr_value);
            }
            break;
        }
        curr_field = g_slist_next(curr_field);
    }

    assert_int_equal(num_values, 4);
    assert_int_equal(new_value_count, 1);

    form_destroy(form);
}

void
form_add_value__updates__adds_when_exists(void** state)
{
    DataForm* form = _new_form();
    g_hash_table_insert(form->tag_to_var, g_strdup("tag1"), g_strdup("var1"));

    FormField* field1 = _new_field();
    field1->var = g_strdup("var1");
    field1->type_t = FIELD_LIST_MULTI;
    field1->values = g_slist_append(field1->values, g_strdup("some text"));
    field1->values = g_slist_append(field1->values, g_strdup("some more text"));
    field1->values = g_slist_append(field1->values, g_strdup("yet some more text"));
    field1->values = g_slist_append(field1->values, g_strdup("new value"));
    form->fields = g_slist_append(form->fields, field1);

    form_add_value(form, "tag1", "new value");

    int num_values = 0;
    int new_value_count = 0;
    GSList* curr_field = form->fields;
    while (curr_field != NULL) {
        FormField* field = curr_field->data;
        if (g_strcmp0(field->var, "var1") == 0) {
            GSList* curr_value = field->values;
            while (curr_value != NULL) {
                num_values++;
                if (g_strcmp0(curr_value->data, "new value") == 0) {
                    new_value_count++;
                }
                curr_value = g_slist_next(curr_value);
            }
            break;
        }
        curr_field = g_slist_next(curr_field);
    }

    assert_int_equal(num_values, 5);
    assert_int_equal(new_value_count, 2);

    form_destroy(form);
}

void
form_remove_value__updates__does_nothing_when_none(void** state)
{
    DataForm* form = _new_form();
    g_hash_table_insert(form->tag_to_var, g_strdup("tag1"), g_strdup("var1"));

    FormField* field1 = _new_field();
    field1->var = g_strdup("var1");
    field1->type_t = FIELD_LIST_MULTI;
    form->fields = g_slist_append(form->fields, field1);

    gboolean res = form_remove_value(form, "tag1", "some value");

    int length = -1;
    GSList* curr_field = form->fields;
    while (curr_field != NULL) {
        FormField* field = curr_field->data;
        if (g_strcmp0(field->var, "var1") == 0) {
            length = g_slist_length(field->values);
        }
        curr_field = g_slist_next(curr_field);
    }

    assert_false(res);
    assert_int_equal(length, 0);

    form_destroy(form);
}

void
form_remove_value__updates__does_nothing_when_doesnt_exist(void** state)
{
    DataForm* form = _new_form();
    g_hash_table_insert(form->tag_to_var, g_strdup("tag1"), g_strdup("var1"));

    FormField* field1 = _new_field();
    field1->var = g_strdup("var1");
    field1->type_t = FIELD_LIST_MULTI;
    field1->values = g_slist_append(field1->values, g_strdup("value1"));
    field1->values = g_slist_append(field1->values, g_strdup("value2"));
    field1->values = g_slist_append(field1->values, g_strdup("value3"));
    field1->values = g_slist_append(field1->values, g_strdup("value4"));
    form->fields = g_slist_append(form->fields, field1);

    gboolean res = form_remove_value(form, "tag1", "value5");

    int length = -1;
    int value_count = 0;
    GSList* curr_field = form->fields;
    while (curr_field != NULL) {
        FormField* field = curr_field->data;
        if (g_strcmp0(field->var, "var1") == 0) {
            length = g_slist_length(field->values);
            GSList* curr_value = field->values;
            while (curr_value != NULL) {
                if (g_strcmp0(curr_value->data, "value5") == 0) {
                    value_count++;
                }
                curr_value = g_slist_next(curr_value);
            }
        }
        curr_field = g_slist_next(curr_field);
    }

    assert_false(res);
    assert_int_equal(length, 4);
    assert_int_equal(value_count, 0);

    form_destroy(form);
}

void
form_remove_value__updates__removes_when_one(void** state)
{
    DataForm* form = _new_form();
    g_hash_table_insert(form->tag_to_var, g_strdup("tag1"), g_strdup("var1"));

    FormField* field1 = _new_field();
    field1->var = g_strdup("var1");
    field1->type_t = FIELD_LIST_MULTI;
    field1->values = g_slist_append(field1->values, g_strdup("value4"));
    form->fields = g_slist_append(form->fields, field1);

    gboolean res = form_remove_value(form, "tag1", "value4");

    int length = -1;
    GSList* curr_field = form->fields;
    while (curr_field != NULL) {
        FormField* field = curr_field->data;
        if (g_strcmp0(field->var, "var1") == 0) {
            length = g_slist_length(field->values);
        }
        curr_field = g_slist_next(curr_field);
    }

    assert_true(res);
    assert_int_equal(length, 0);

    form_destroy(form);
}

void
form_remove_value__updates__removes_when_many(void** state)
{
    DataForm* form = _new_form();
    g_hash_table_insert(form->tag_to_var, g_strdup("tag1"), g_strdup("var1"));

    FormField* field1 = _new_field();
    field1->var = g_strdup("var1");
    field1->type_t = FIELD_LIST_MULTI;
    field1->values = g_slist_append(field1->values, g_strdup("value1"));
    field1->values = g_slist_append(field1->values, g_strdup("value2"));
    field1->values = g_slist_append(field1->values, g_strdup("value3"));
    field1->values = g_slist_append(field1->values, g_strdup("value4"));
    form->fields = g_slist_append(form->fields, field1);

    gboolean res = form_remove_value(form, "tag1", "value2");

    int length = -1;
    int value_count = 0;
    GSList* curr_field = form->fields;
    while (curr_field != NULL) {
        FormField* field = curr_field->data;
        if (g_strcmp0(field->var, "var1") == 0) {
            length = g_slist_length(field->values);
            GSList* curr_value = field->values;
            while (curr_value != NULL) {
                if (g_strcmp0(curr_value->data, "value2") == 0) {
                    value_count++;
                }
                curr_value = g_slist_next(curr_value);
            }
        }
        curr_field = g_slist_next(curr_field);
    }

    assert_true(res);
    assert_int_equal(length, 3);
    assert_int_equal(value_count, 0);

    form_destroy(form);
}

void
form_remove_text_multi_value__updates__does_nothing_when_none(void** state)
{
    DataForm* form = _new_form();
    g_hash_table_insert(form->tag_to_var, g_strdup("tag1"), g_strdup("var1"));

    FormField* field1 = _new_field();
    field1->var = g_strdup("var1");
    field1->type_t = FIELD_LIST_MULTI;
    form->fields = g_slist_append(form->fields, field1);

    gboolean res = form_remove_text_multi_value(form, "tag1", 3);

    int length = -1;
    GSList* curr_field = form->fields;
    while (curr_field != NULL) {
        FormField* field = curr_field->data;
        if (g_strcmp0(field->var, "var1") == 0) {
            length = g_slist_length(field->values);
        }
        curr_field = g_slist_next(curr_field);
    }

    assert_false(res);
    assert_int_equal(length, 0);

    form_destroy(form);
}

void
form_remove_text_multi_value__updates__does_nothing_when_doesnt_exist(void** state)
{
    DataForm* form = _new_form();
    g_hash_table_insert(form->tag_to_var, g_strdup("tag1"), g_strdup("var1"));

    FormField* field1 = _new_field();
    field1->var = g_strdup("var1");
    field1->type_t = FIELD_LIST_MULTI;
    field1->values = g_slist_append(field1->values, g_strdup("value1"));
    field1->values = g_slist_append(field1->values, g_strdup("value2"));
    field1->values = g_slist_append(field1->values, g_strdup("value3"));
    field1->values = g_slist_append(field1->values, g_strdup("value4"));
    form->fields = g_slist_append(form->fields, field1);

    gboolean res = form_remove_text_multi_value(form, "tag1", 5);

    int length = -1;
    int value_count = 0;
    GSList* curr_field = form->fields;
    while (curr_field != NULL) {
        FormField* field = curr_field->data;
        if (g_strcmp0(field->var, "var1") == 0) {
            length = g_slist_length(field->values);
            GSList* curr_value = field->values;
            while (curr_value != NULL) {
                if (g_strcmp0(curr_value->data, "value5") == 0) {
                    value_count++;
                }
                curr_value = g_slist_next(curr_value);
            }
        }
        curr_field = g_slist_next(curr_field);
    }

    assert_false(res);
    assert_int_equal(length, 4);
    assert_int_equal(value_count, 0);

    form_destroy(form);
}

void
form_remove_text_multi_value__updates__removes_when_one(void** state)
{
    DataForm* form = _new_form();
    g_hash_table_insert(form->tag_to_var, g_strdup("tag1"), g_strdup("var1"));

    FormField* field1 = _new_field();
    field1->var = g_strdup("var1");
    field1->type_t = FIELD_LIST_MULTI;
    field1->values = g_slist_append(field1->values, g_strdup("value4"));
    form->fields = g_slist_append(form->fields, field1);

    gboolean res = form_remove_text_multi_value(form, "tag1", 1);

    int length = -1;
    GSList* curr_field = form->fields;
    while (curr_field != NULL) {
        FormField* field = curr_field->data;
        if (g_strcmp0(field->var, "var1") == 0) {
            length = g_slist_length(field->values);
        }
        curr_field = g_slist_next(curr_field);
    }

    assert_true(res);
    assert_int_equal(length, 0);

    form_destroy(form);
}

void
form_remove_text_multi_value__updates__removes_when_many(void** state)
{
    DataForm* form = _new_form();
    g_hash_table_insert(form->tag_to_var, g_strdup("tag1"), g_strdup("var1"));

    FormField* field1 = _new_field();
    field1->var = g_strdup("var1");
    field1->type_t = FIELD_LIST_MULTI;
    field1->values = g_slist_append(field1->values, g_strdup("value1"));
    field1->values = g_slist_append(field1->values, g_strdup("value2"));
    field1->values = g_slist_append(field1->values, g_strdup("value3"));
    field1->values = g_slist_append(field1->values, g_strdup("value4"));
    form->fields = g_slist_append(form->fields, field1);

    gboolean res = form_remove_text_multi_value(form, "tag1", 2);

    int length = -1;
    int value_count = 0;
    GSList* curr_field = form->fields;
    while (curr_field != NULL) {
        FormField* field = curr_field->data;
        if (g_strcmp0(field->var, "var1") == 0) {
            length = g_slist_length(field->values);
            GSList* curr_value = field->values;
            while (curr_value != NULL) {
                if (g_strcmp0(curr_value->data, "value2") == 0) {
                    value_count++;
                }
                curr_value = g_slist_next(curr_value);
            }
        }
        curr_field = g_slist_next(curr_field);
    }

    assert_true(res);
    assert_int_equal(length, 3);
    assert_int_equal(value_count, 0);

    form_destroy(form);
}
