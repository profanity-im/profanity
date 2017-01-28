/*
 * mucconfwin.c
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

#include <assert.h>
#include <stdlib.h>

#include "ui/ui.h"
#include "ui/window.h"
#include "ui/win_types.h"
#include "ui/window_list.h"

static void _mucconfwin_form_field(ProfWin *window, char *tag, FormField *field);

void
mucconfwin_show_form(ProfMucConfWin *confwin)
{
    ProfWin *window = (ProfWin*) confwin;
    if (confwin->form->title) {
        win_print(window, THEME_DEFAULT, '-', "Form title: ");
        win_appendln(window, THEME_DEFAULT, "%s", confwin->form->title);
    } else {
        win_println(window, THEME_DEFAULT, '-', "Configuration for room %s.", confwin->roomjid);
    }
    win_println(window, THEME_DEFAULT, '-', "");

    mucconfwin_form_help(confwin);

    GSList *fields = confwin->form->fields;
    GSList *curr_field = fields;
    while (curr_field) {
        FormField *field = curr_field->data;

        if ((g_strcmp0(field->type, "fixed") == 0) && field->values) {
            if (field->values) {
                char *value = field->values->data;
                win_println(window, THEME_DEFAULT, '-', "%s", value);
            }
        } else if (g_strcmp0(field->type, "hidden") != 0 && field->var) {
            char *tag = g_hash_table_lookup(confwin->form->var_to_tag, field->var);
            _mucconfwin_form_field(window, tag, field);
        }

        curr_field = g_slist_next(curr_field);
    }
}

void
mucconfwin_show_form_field(ProfMucConfWin *confwin, DataForm *form, char *tag)
{
    assert(confwin != NULL);

    FormField *field = form_get_field_by_tag(form, tag);
    ProfWin *window = (ProfWin*)confwin;
    _mucconfwin_form_field(window, tag, field);
    win_println(window, THEME_DEFAULT, '-', "");
}

void
mucconfwin_handle_configuration(ProfMucConfWin *confwin, DataForm *form)
{
    assert(confwin != NULL);

    ProfWin *window = (ProfWin*)confwin;
    ui_focus_win(window);

    mucconfwin_show_form(confwin);

    win_println(window, THEME_DEFAULT, '-', "");
    win_println(window, THEME_DEFAULT, '-', "Use '/form submit' to save changes.");
    win_println(window, THEME_DEFAULT, '-', "Use '/form cancel' to cancel changes.");
    win_println(window, THEME_DEFAULT, '-', "See '/form help' for more information.");
    win_println(window, THEME_DEFAULT, '-', "");
}

void
mucconfwin_field_help(ProfMucConfWin *confwin, char *tag)
{
    assert(confwin != NULL);

    ProfWin *window = (ProfWin*) confwin;
    FormField *field = form_get_field_by_tag(confwin->form, tag);
    if (field) {
        win_print(window, THEME_DEFAULT, '-', "%s", field->label);
        if (field->required) {
            win_appendln(window, THEME_DEFAULT, " (Required):");
        } else {
            win_appendln(window, THEME_DEFAULT, ":");
        }
        if (field->description) {
            win_println(window, THEME_DEFAULT, '-', "  Description : %s", field->description);
        }
        win_println(window, THEME_DEFAULT, '-', "  Type        : %s", field->type);

        int num_values = 0;
        GSList *curr_option = NULL;
        FormOption *option = NULL;

        switch (field->type_t) {
        case FIELD_TEXT_SINGLE:
        case FIELD_TEXT_PRIVATE:
            win_println(window, THEME_DEFAULT, '-', "  Set         : /%s <value>", tag);
            win_println(window, THEME_DEFAULT, '-', "  Where       : <value> is any text");
            break;
        case FIELD_TEXT_MULTI:
            num_values = form_get_value_count(confwin->form, tag);
            win_println(window, THEME_DEFAULT, '-', "  Add         : /%s add <value>", tag);
            win_println(window, THEME_DEFAULT, '-', "  Where       : <value> is any text");
            if (num_values > 0) {
                win_println(window, THEME_DEFAULT, '-', "  Remove      : /%s remove <value>", tag);
                win_println(window, THEME_DEFAULT, '-', "  Where       : <value> between 'val1' and 'val%d'", num_values);
            }
            break;
        case FIELD_BOOLEAN:
            win_println(window, THEME_DEFAULT, '-', "  Set         : /%s <value>", tag);
            win_println(window, THEME_DEFAULT, '-', "  Where       : <value> is either 'on' or 'off'");
            break;
        case FIELD_LIST_SINGLE:
            win_println(window, THEME_DEFAULT, '-', "  Set         : /%s <value>", tag);
            win_println(window, THEME_DEFAULT, '-', "  Where       : <value> is one of");
            curr_option = field->options;
            while (curr_option) {
                option = curr_option->data;
                win_println(window, THEME_DEFAULT, '-', "                  %s", option->value);
                curr_option = g_slist_next(curr_option);
            }
            break;
        case FIELD_LIST_MULTI:
            win_println(window, THEME_DEFAULT, '-', "  Add         : /%s add <value>", tag);
            win_println(window, THEME_DEFAULT, '-', "  Remove      : /%s remove <value>", tag);
            win_println(window, THEME_DEFAULT, '-', "  Where       : <value> is one of");
            curr_option = field->options;
            while (curr_option) {
                option = curr_option->data;
                win_println(window, THEME_DEFAULT, '-', "                  %s", option->value);
                curr_option = g_slist_next(curr_option);
            }
            break;
        case FIELD_JID_SINGLE:
            win_println(window, THEME_DEFAULT, '-', "  Set         : /%s <value>", tag);
            win_println(window, THEME_DEFAULT, '-', "  Where       : <value> is a valid Jabber ID");
            break;
        case FIELD_JID_MULTI:
            win_println(window, THEME_DEFAULT, '-', "  Add         : /%s add <value>", tag);
            win_println(window, THEME_DEFAULT, '-', "  Remove      : /%s remove <value>", tag);
            win_println(window, THEME_DEFAULT, '-', "  Where       : <value> is a valid Jabber ID");
            break;
        case FIELD_FIXED:
        case FIELD_UNKNOWN:
        case FIELD_HIDDEN:
        default:
            break;
        }
    } else {
        win_println(window, THEME_DEFAULT, '-', "No such field %s", tag);
    }
}

void
mucconfwin_form_help(ProfMucConfWin *confwin)
{
    assert(confwin != NULL);

    if (confwin->form->instructions) {
        ProfWin *window = (ProfWin*) confwin;
        win_println(window, THEME_DEFAULT, '-', "Supplied instructions:");
        win_println(window, THEME_DEFAULT, '-', "%s", confwin->form->instructions);
        win_println(window, THEME_DEFAULT, '-', "");
    }
}

static void
_mucconfwin_form_field(ProfWin *window, char *tag, FormField *field)
{
    win_print(window, THEME_AWAY, '-', "[%s] ", tag);
    win_append(window, THEME_DEFAULT, "%s", field->label);
    if (field->required) {
        win_append(window, THEME_DEFAULT, " (required): ");
    } else {
        win_append(window, THEME_DEFAULT, ": ");
    }

    GSList *values = field->values;
    GSList *curr_value = values;

    switch (field->type_t) {
    case FIELD_HIDDEN:
        break;
    case FIELD_TEXT_SINGLE:
        if (curr_value) {
            char *value = curr_value->data;
            if (value) {
                if (g_strcmp0(field->var, "muc#roomconfig_roomsecret") == 0) {
                    win_append(window, THEME_ONLINE, "[hidden]");
                } else {
                    win_append(window, THEME_ONLINE, "%s", value);
                }
            }
        }
        win_newline(window);
        break;
    case FIELD_TEXT_PRIVATE:
        if (curr_value) {
            char *value = curr_value->data;
            if (value) {
                win_append(window, THEME_ONLINE, "[hidden]");
            }
        }
        win_newline(window);
        break;
    case FIELD_TEXT_MULTI:
        win_newline(window);
        int index = 1;
        while (curr_value) {
            char *value = curr_value->data;
            GString *val_tag = g_string_new("");
            g_string_printf(val_tag, "val%d", index++);
            win_println(window, THEME_ONLINE, '-', "  [%s] %s", val_tag->str, value);
            g_string_free(val_tag, TRUE);
            curr_value = g_slist_next(curr_value);
        }
        break;
    case FIELD_BOOLEAN:
        if (curr_value == NULL) {
            win_appendln(window, THEME_OFFLINE, "FALSE");
        } else {
            char *value = curr_value->data;
            if (value == NULL) {
                win_appendln(window, THEME_OFFLINE, "FALSE");
            } else {
                if (g_strcmp0(value, "0") == 0) {
                    win_appendln(window, THEME_OFFLINE, "FALSE");
                } else {
                    win_appendln(window, THEME_ONLINE, "TRUE");
                }
            }
        }
        break;
    case FIELD_LIST_SINGLE:
        if (curr_value) {
            win_newline(window);
            char *value = curr_value->data;
            GSList *options = field->options;
            GSList *curr_option = options;
            while (curr_option) {
                FormOption *option = curr_option->data;
                if (g_strcmp0(option->value, value) == 0) {
                    win_println(window, THEME_ONLINE, '-', "  [%s] %s", option->value, option->label);
                } else {
                    win_println(window, THEME_OFFLINE, '-', "  [%s] %s", option->value, option->label);
                }
                curr_option = g_slist_next(curr_option);
            }
        }
        break;
    case FIELD_LIST_MULTI:
        if (curr_value) {
            win_newline(window);
            GSList *options = field->options;
            GSList *curr_option = options;
            while (curr_option) {
                FormOption *option = curr_option->data;
                if (g_slist_find_custom(curr_value, option->value, (GCompareFunc)g_strcmp0)) {
                    win_println(window, THEME_ONLINE, '-', "  [%s] %s", option->value, option->label);
                } else {
                    win_println(window, THEME_OFFLINE, '-', "  [%s] %s", option->value, option->label);
                }
                curr_option = g_slist_next(curr_option);
            }
        }
        break;
    case FIELD_JID_SINGLE:
        if (curr_value) {
            char *value = curr_value->data;
            if (value) {
                win_append(window, THEME_ONLINE, "%s", value);
            }
        }
        win_newline(window);
        break;
    case FIELD_JID_MULTI:
        win_newline(window);
        while (curr_value) {
            char *value = curr_value->data;
            win_println(window, THEME_ONLINE, '-', "  %s", value);
            curr_value = g_slist_next(curr_value);
        }
        break;
    case FIELD_FIXED:
        if (curr_value) {
            char *value = curr_value->data;
            if (value) {
                win_append(window, THEME_DEFAULT, "%s", value);
            }
        }
        win_newline(window);
        break;
    default:
        break;
    }
}

char*
mucconfwin_get_string(ProfMucConfWin *confwin)
{
    assert(confwin != NULL);

    GString *res = g_string_new("");

    char *title = win_get_title((ProfWin*)confwin);
    g_string_append(res, title);
    free(title);

    char *resstr = res->str;
    g_string_free(res, FALSE);

    return resstr;
}
