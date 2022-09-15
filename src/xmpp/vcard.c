/*
 * vcard.c
 * vim: expandtab:ts=4:sts=4:sw=4
 *
 * Copyright (C) 2022 Marouane L. <techmetx11@disroot.org>
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
#include <errno.h>
#include <glib.h>
#include <strophe.h>
#include <sys/stat.h>

#include "xmpp/vcard.h"
#include "config/files.h"
#include "config/preferences.h"
#include "ui/ui.h"
#include "ui/window_list.h"
#include "xmpp/connection.h"
#include "xmpp/iq.h"
#include "xmpp/stanza.h"

// Connected account's vCard
vCard* vcard_user = NULL;

typedef struct
{
    vCard* vcard;
    ProfWin* window;

    // for photo
    int photo_index;
    gboolean open;
    char* filename;
} _userdata;

static void
_free_vcard_element(void* velement)
{
    vcard_element_t* element = (vcard_element_t*)velement;

    switch (element->type) {
    case VCARD_NICKNAME:
    {
        if (element->nickname) {
            free(element->nickname);
        }
        break;
    }
    case VCARD_PHOTO:
    {
        if (element->photo.external) {
            if (element->photo.extval) {
                free(element->photo.extval);
            }
        } else {
            if (element->photo.data) {
                g_free(element->photo.data);
            }
            if (element->photo.type) {
                free(element->photo.type);
            }
        }
        break;
    }
    case VCARD_BIRTHDAY:
    {
        g_date_time_unref(element->birthday);
        break;
    }
    case VCARD_ADDRESS:
    {
        if (element->address.pobox) {
            free(element->address.pobox);
        }
        if (element->address.extaddr) {
            free(element->address.extaddr);
        }
        if (element->address.street) {
            free(element->address.street);
        }
        if (element->address.locality) {
            free(element->address.locality);
        }
        if (element->address.region) {
            free(element->address.region);
        }
        if (element->address.pcode) {
            free(element->address.pcode);
        }
        if (element->address.country) {
            free(element->address.country);
        }
        break;
    }
    case VCARD_TELEPHONE:
    {
        if (element->telephone.number) {
            free(element->telephone.number);
        }
        break;
    }
    case VCARD_EMAIL:
    {
        if (element->email.userid) {
            free(element->email.userid);
        }
        break;
    }
    case VCARD_JID:
    {
        if (element->jid) {
            free(element->jid);
        }
        break;
    }
    case VCARD_TITLE:
    {
        if (element->title) {
            free(element->title);
        }
        break;
    }
    case VCARD_ROLE:
    {
        if (element->role) {
            free(element->role);
        }
        break;
    }
    case VCARD_NOTE:
    {
        if (element->note) {
            free(element->note);
        }
        break;
    }
    case VCARD_URL:
        if (element->url) {
            free(element->url);
        }
        break;
    }

    free(element);
}

void
vcard_free_full(vCard* vcard)
{
    if (!vcard) {
        return;
    }

    // Free the fullname element
    if (vcard->fullname) {
        free(vcard->fullname);
    }

    // Free the name element
    if (vcard->name.family) {
        free(vcard->name.family);
    }
    if (vcard->name.given) {
        free(vcard->name.given);
    }
    if (vcard->name.middle) {
        free(vcard->name.middle);
    }
    if (vcard->name.prefix) {
        free(vcard->name.prefix);
    }
    if (vcard->name.suffix) {
        free(vcard->name.suffix);
    }

    // Clear the elements queue
    g_queue_clear_full(vcard->elements, _free_vcard_element);
}

vCard*
vcard_new(void)
{
    vCard* vcard = calloc(1, sizeof(vCard));

    if (!vcard) {
        return NULL;
    }

    vcard->elements = g_queue_new();

    return vcard;
}

void
vcard_free(vCard* vcard)
{
    if (!vcard) {
        return;
    }

    g_queue_free(vcard->elements);
    free(vcard);
}

static void
_free_userdata(_userdata* data)
{
    vcard_free_full(data->vcard);
    vcard_free(data->vcard);

    if (data->filename) {
        free(data->filename);
    }

    free(data);
}

// Function must be called with <vCard> root element
gboolean
vcard_parse(xmpp_stanza_t* vcard_xml, vCard* vcard)
{
    // 2 bits
    // 01 = fullname is set
    // 10 = name is set
    // Fullname and name can only be set once, if another
    // fullname, or name element is found, then it will be
    // ignored
    char flags = 0;

    if (!vcard_xml) {
        // vCard XML is null, stop
        return FALSE;
    }

    // Pointer to children of the vCard element
    xmpp_stanza_t* child_pointer = xmpp_stanza_get_children(vcard_xml);

    // Connection context, for freeing elements allocated by Strophe
    const xmpp_ctx_t* ctx = connection_get_ctx();

    // Pointer to the children of the children
    xmpp_stanza_t* child_pointer2 = NULL;

    for (; child_pointer != NULL; child_pointer = xmpp_stanza_get_next(child_pointer)) {
        if (!g_strcmp0(xmpp_stanza_get_name(child_pointer), "FN") && !(flags & 1)) {
            vcard->fullname = stanza_text_strdup(child_pointer);
            flags |= 1;
        } else if (!g_strcmp0(xmpp_stanza_get_name(child_pointer), "N") && !(flags & 2)) {
            child_pointer2 = xmpp_stanza_get_child_by_name(child_pointer, "FAMILY");
            vcard->name.family = child_pointer2 ? stanza_text_strdup(child_pointer2) : NULL;
            child_pointer2 = xmpp_stanza_get_child_by_name(child_pointer, "GIVEN");
            vcard->name.given = child_pointer2 ? stanza_text_strdup(child_pointer2) : NULL;
            child_pointer2 = xmpp_stanza_get_child_by_name(child_pointer, "MIDDLE");
            vcard->name.middle = child_pointer2 ? stanza_text_strdup(child_pointer2) : NULL;
            child_pointer2 = xmpp_stanza_get_child_by_name(child_pointer, "PREFIX");
            vcard->name.prefix = child_pointer2 ? stanza_text_strdup(child_pointer2) : NULL;
            child_pointer2 = xmpp_stanza_get_child_by_name(child_pointer, "SUFFIX");
            vcard->name.suffix = child_pointer2 ? stanza_text_strdup(child_pointer2) : NULL;
            flags |= 2;
        } else if (!g_strcmp0(xmpp_stanza_get_name(child_pointer), "NICKNAME")) {
            vcard_element_t* element = calloc(1, sizeof(vcard_element_t));
            if (!element) {
                // Allocation failed
                continue;
            }

            element->type = VCARD_NICKNAME;
            element->nickname = stanza_text_strdup(child_pointer);

            if (!element->nickname) {
                // Invaild element, free and do not push
                free(element);
                continue;
            }

            g_queue_push_tail(vcard->elements, element);
        } else if (!g_strcmp0(xmpp_stanza_get_name(child_pointer), "PHOTO")) {
            vcard_element_t* element = calloc(1, sizeof(vcard_element_t));
            if (!element) {
                // Allocation failed
                continue;
            }

            element->type = VCARD_PHOTO;

            child_pointer2 = xmpp_stanza_get_child_by_name(child_pointer, "EXTVAL");
            if (child_pointer2) {
                element->photo.external = TRUE;
                element->photo.extval = stanza_text_strdup(child_pointer2);
            } else {
                element->photo.external = FALSE;
                child_pointer2 = xmpp_stanza_get_child_by_name(child_pointer, "BINVAL");
                if (!child_pointer2) {
                    // No EXTVAL or BINVAL, invalid photo, skipping
                    free(element);
                    continue;
                }
                char* photo_base64 = xmpp_stanza_get_text(child_pointer2);
                if (!photo_base64) {
                    free(element);
                    continue;
                }
                element->photo.data = g_base64_decode(photo_base64, &element->photo.length);
                xmpp_free(ctx, photo_base64);

                child_pointer2 = xmpp_stanza_get_child_by_name(child_pointer, "TYPE");
                if (!child_pointer2) {
                    // No TYPE, invalid photo, skipping
                    if (element->photo.data) {
                        g_free(element->photo.data);
                    }
                    free(element);
                    continue;
                }

                element->photo.type = stanza_text_strdup(child_pointer2);
            }
            g_queue_push_tail(vcard->elements, element);
        } else if (!g_strcmp0(xmpp_stanza_get_name(child_pointer), "BDAY")) {
            vcard_element_t* element = calloc(1, sizeof(vcard_element_t));
            if (!element) {
                // Allocation failed
                continue;
            }

            element->type = VCARD_BIRTHDAY;

            char* bday_text = xmpp_stanza_get_text(child_pointer);
            if (!bday_text) {
                free(element);
                continue;
            }

            // Check if the birthday string is a date/time or date only string
            char* check_pointer = bday_text;
            gboolean is_datetime = FALSE;
            for (; *check_pointer != 0; check_pointer++) {
                if (*check_pointer == 'T' || *check_pointer == 't' || *check_pointer == ' ') {
                    is_datetime = TRUE;
                    break;
                }
            }

            if (!is_datetime) {
                // glib doesn't parse ISO8601 date only strings, so we're gonna
                // add a time string to make it parse
                GString* date_string = g_string_new(bday_text);
                g_string_append(date_string, "T00:00:00Z");

                element->birthday = g_date_time_new_from_iso8601(date_string->str, NULL);

                g_string_free(date_string, TRUE);
            } else {
                element->birthday = g_date_time_new_from_iso8601(bday_text, NULL);
            }

            xmpp_free(ctx, bday_text);

            if (!element->birthday) {
                // Invalid birthday ISO 8601 date, skipping
                free(element);
                continue;
            }
            g_queue_push_tail(vcard->elements, element);
        } else if (!g_strcmp0(xmpp_stanza_get_name(child_pointer), "ADR")) {
            vcard_element_t* element = calloc(1, sizeof(vcard_element_t));
            if (!element) {
                // Allocation failed
                continue;
            }

            element->type = VCARD_ADDRESS;
            if (xmpp_stanza_get_child_by_name(child_pointer, "HOME")) {
                element->address.options |= VCARD_HOME;
            }
            if (xmpp_stanza_get_child_by_name(child_pointer, "WORK")) {
                element->address.options |= VCARD_WORK;
            }
            if (xmpp_stanza_get_child_by_name(child_pointer, "POSTAL")) {
                element->address.options |= VCARD_POSTAL;
            }
            if (xmpp_stanza_get_child_by_name(child_pointer, "PARCEL")) {
                element->address.options |= VCARD_PARCEL;
            }

            if (xmpp_stanza_get_child_by_name(child_pointer, "DOM")) {
                element->address.options |= VCARD_DOM;
            } else if (xmpp_stanza_get_child_by_name(child_pointer, "INTL")) {
                element->address.options |= VCARD_INTL;
            }

            if (xmpp_stanza_get_child_by_name(child_pointer, "PREF")) {
                element->address.options |= VCARD_PREF;
            }

            child_pointer2 = xmpp_stanza_get_child_by_name(child_pointer, "POBOX");
            element->address.pobox = child_pointer2 ? stanza_text_strdup(child_pointer2) : NULL;
            child_pointer2 = xmpp_stanza_get_child_by_name(child_pointer, "EXTADD");
            element->address.extaddr = child_pointer2 ? stanza_text_strdup(child_pointer2) : NULL;
            child_pointer2 = xmpp_stanza_get_child_by_name(child_pointer, "STREET");
            element->address.street = child_pointer2 ? stanza_text_strdup(child_pointer2) : NULL;
            child_pointer2 = xmpp_stanza_get_child_by_name(child_pointer, "LOCALITY");
            element->address.locality = child_pointer2 ? stanza_text_strdup(child_pointer2) : NULL;
            child_pointer2 = xmpp_stanza_get_child_by_name(child_pointer, "REGION");
            element->address.region = child_pointer2 ? stanza_text_strdup(child_pointer2) : NULL;
            child_pointer2 = xmpp_stanza_get_child_by_name(child_pointer, "PCODE");
            element->address.pcode = child_pointer2 ? stanza_text_strdup(child_pointer2) : NULL;
            child_pointer2 = xmpp_stanza_get_child_by_name(child_pointer, "CTRY");
            element->address.country = child_pointer2 ? stanza_text_strdup(child_pointer2) : NULL;
            g_queue_push_tail(vcard->elements, element);
        } else if (!g_strcmp0(xmpp_stanza_get_name(child_pointer), "TEL")) {
            vcard_element_t* element = calloc(1, sizeof(vcard_element_t));
            if (!element) {
                // Allocation failed
                continue;
            }

            element->type = VCARD_TELEPHONE;
            if (xmpp_stanza_get_child_by_name(child_pointer, "HOME")) {
                element->telephone.options |= VCARD_HOME;
            }
            if (xmpp_stanza_get_child_by_name(child_pointer, "WORK")) {
                element->telephone.options |= VCARD_WORK;
            }
            if (xmpp_stanza_get_child_by_name(child_pointer, "VOICE")) {
                element->telephone.options |= VCARD_TEL_VOICE;
            }
            if (xmpp_stanza_get_child_by_name(child_pointer, "FAX")) {
                element->telephone.options |= VCARD_TEL_FAX;
            }
            if (xmpp_stanza_get_child_by_name(child_pointer, "PAGER")) {
                element->telephone.options |= VCARD_TEL_PAGER;
            }
            if (xmpp_stanza_get_child_by_name(child_pointer, "MSG")) {
                element->telephone.options |= VCARD_TEL_MSG;
            }
            if (xmpp_stanza_get_child_by_name(child_pointer, "CELL")) {
                element->telephone.options |= VCARD_TEL_CELL;
            }
            if (xmpp_stanza_get_child_by_name(child_pointer, "VIDEO")) {
                element->telephone.options |= VCARD_TEL_VIDEO;
            }
            if (xmpp_stanza_get_child_by_name(child_pointer, "BBS")) {
                element->telephone.options |= VCARD_TEL_BBS;
            }
            if (xmpp_stanza_get_child_by_name(child_pointer, "MODEM")) {
                element->telephone.options |= VCARD_TEL_MODEM;
            }
            if (xmpp_stanza_get_child_by_name(child_pointer, "ISDN")) {
                element->telephone.options |= VCARD_TEL_ISDN;
            }
            if (xmpp_stanza_get_child_by_name(child_pointer, "PCS")) {
                element->telephone.options |= VCARD_TEL_PCS;
            }
            if (xmpp_stanza_get_child_by_name(child_pointer, "PREF")) {
                element->telephone.options |= VCARD_PREF;
            }

            child_pointer2 = xmpp_stanza_get_child_by_name(child_pointer, "NUMBER");
            if (!child_pointer2) {
                // No NUMBER, invalid telephone, skipping
                free(element);
                continue;
            }
            element->telephone.number = stanza_text_strdup(child_pointer2);
            g_queue_push_tail(vcard->elements, element);
        } else if (!g_strcmp0(xmpp_stanza_get_name(child_pointer), "EMAIL")) {
            vcard_element_t* element = calloc(1, sizeof(vcard_element_t));
            if (!element) {
                // Allocation failed
                continue;
            }

            element->type = VCARD_EMAIL;
            if (xmpp_stanza_get_child_by_name(child_pointer, "HOME")) {
                element->email.options |= VCARD_HOME;
            }
            if (xmpp_stanza_get_child_by_name(child_pointer, "WORK")) {
                element->email.options |= VCARD_WORK;
            }
            if (xmpp_stanza_get_child_by_name(child_pointer, "INTERNET")) {
                element->email.options |= VCARD_EMAIL_INTERNET;
            }
            if (xmpp_stanza_get_child_by_name(child_pointer, "X400")) {
                element->email.options |= VCARD_EMAIL_X400;
            }
            if (xmpp_stanza_get_child_by_name(child_pointer, "PREF")) {
                element->email.options |= VCARD_PREF;
            }

            child_pointer2 = xmpp_stanza_get_child_by_name(child_pointer, "USERID");
            if (!child_pointer2) {
                // No USERID, invalid email, skipping
                free(element);
                continue;
            }
            element->email.userid = stanza_text_strdup(child_pointer2);
            g_queue_push_tail(vcard->elements, element);
        } else if (!g_strcmp0(xmpp_stanza_get_name(child_pointer), "JABBERID")) {
            vcard_element_t* element = calloc(1, sizeof(vcard_element_t));
            if (!element) {
                // Allocation failed
                continue;
            }

            element->type = VCARD_JID;
            element->jid = stanza_text_strdup(child_pointer);

            if (!element->jid) {
                // Invalid element, free and do not push
                free(element);
                continue;
            }

            g_queue_push_tail(vcard->elements, element);
        } else if (!g_strcmp0(xmpp_stanza_get_name(child_pointer), "TITLE")) {
            vcard_element_t* element = calloc(1, sizeof(vcard_element_t));
            if (!element) {
                // Allocation failed
                continue;
            }

            element->type = VCARD_TITLE;
            element->title = stanza_text_strdup(child_pointer);

            if (!element->title) {
                // Invalid element, free and do not push
                free(element);
                continue;
            }

            g_queue_push_tail(vcard->elements, element);
        } else if (!g_strcmp0(xmpp_stanza_get_name(child_pointer), "ROLE")) {
            vcard_element_t* element = calloc(1, sizeof(vcard_element_t));
            if (!element) {
                // Allocation failed
                continue;
            }

            element->type = VCARD_ROLE;
            element->role = stanza_text_strdup(child_pointer);

            if (!element->role) {
                // Invalid element, free and do not push
                free(element);
                continue;
            }

            g_queue_push_tail(vcard->elements, element);
        } else if (!g_strcmp0(xmpp_stanza_get_name(child_pointer), "NOTE")) {
            vcard_element_t* element = calloc(1, sizeof(vcard_element_t));
            if (!element) {
                // Allocation failed
                continue;
            }

            element->type = VCARD_NOTE;
            element->note = stanza_text_strdup(child_pointer);

            if (!element->note) {
                // Invalid element, free and do not push
                free(element);
                continue;
            }

            g_queue_push_tail(vcard->elements, element);
        } else if (!g_strcmp0(xmpp_stanza_get_name(child_pointer), "URL")) {
            vcard_element_t* element = calloc(1, sizeof(vcard_element_t));
            if (!element) {
                // Allocation failed
                continue;
            }

            element->type = VCARD_URL;
            element->url = stanza_text_strdup(child_pointer);

            if (!element->note) {
                // Invalid element, free and do not push
                free(element);
                continue;
            }

            g_queue_push_tail(vcard->elements, element);
        }
    }
    return TRUE;
}

xmpp_stanza_t*
vcard_to_xml(xmpp_ctx_t* const ctx, vCard* vcard)
{
    if (!vcard || !ctx) {
        return NULL;
    }

    xmpp_stanza_t* vcard_stanza = xmpp_stanza_new(ctx);
    xmpp_stanza_set_name(vcard_stanza, STANZA_NAME_VCARD);
    xmpp_stanza_set_ns(vcard_stanza, STANZA_NS_VCARD);

    if (vcard->fullname) {
        // <FN> element
        xmpp_stanza_t* fn = xmpp_stanza_new(ctx);
        xmpp_stanza_set_name(fn, "FN");

        xmpp_stanza_t* fn_text = xmpp_stanza_new(ctx);
        xmpp_stanza_set_text(fn_text, vcard->fullname);
        xmpp_stanza_add_child(fn, fn_text);
        xmpp_stanza_release(fn_text);

        xmpp_stanza_add_child(vcard_stanza, fn);
        xmpp_stanza_release(fn);
    }

    if (vcard->name.family || vcard->name.given || vcard->name.middle || vcard->name.prefix || vcard->name.suffix) {
        // <NAME> element
        xmpp_stanza_t* name = xmpp_stanza_new(ctx);
        xmpp_stanza_set_name(name, "NAME");

        if (vcard->name.family) {
            xmpp_stanza_t* name_family = xmpp_stanza_new(ctx);
            xmpp_stanza_set_name(name_family, "FAMILY");

            xmpp_stanza_t* name_family_text = xmpp_stanza_new(ctx);
            xmpp_stanza_set_text(name_family_text, vcard->name.family);
            xmpp_stanza_add_child(name_family, name_family_text);
            xmpp_stanza_release(name_family_text);

            xmpp_stanza_add_child(name, name_family);
            xmpp_stanza_release(name_family);
        }

        if (vcard->name.given) {
            xmpp_stanza_t* name_given = xmpp_stanza_new(ctx);
            xmpp_stanza_set_name(name_given, "GIVEN");

            xmpp_stanza_t* name_given_text = xmpp_stanza_new(ctx);
            xmpp_stanza_set_text(name_given_text, vcard->name.given);
            xmpp_stanza_add_child(name_given, name_given_text);
            xmpp_stanza_release(name_given_text);

            xmpp_stanza_add_child(name, name_given);
            xmpp_stanza_release(name_given);
        }

        if (vcard->name.middle) {
            xmpp_stanza_t* name_middle = xmpp_stanza_new(ctx);
            xmpp_stanza_set_name(name_middle, "MIDDLE");

            xmpp_stanza_t* name_middle_text = xmpp_stanza_new(ctx);
            xmpp_stanza_set_text(name_middle_text, vcard->name.middle);
            xmpp_stanza_add_child(name_middle, name_middle_text);
            xmpp_stanza_release(name_middle_text);

            xmpp_stanza_add_child(name, name_middle);
            xmpp_stanza_release(name_middle);
        }

        if (vcard->name.prefix) {
            xmpp_stanza_t* name_prefix = xmpp_stanza_new(ctx);
            xmpp_stanza_set_name(name_prefix, "PREFIX");

            xmpp_stanza_t* name_prefix_text = xmpp_stanza_new(ctx);
            xmpp_stanza_set_text(name_prefix_text, vcard->name.prefix);
            xmpp_stanza_add_child(name_prefix, name_prefix_text);
            xmpp_stanza_release(name_prefix_text);

            xmpp_stanza_add_child(name, name_prefix);
            xmpp_stanza_release(name_prefix);
        }

        if (vcard->name.suffix) {
            xmpp_stanza_t* name_suffix = xmpp_stanza_new(ctx);
            xmpp_stanza_set_name(name_suffix, "SUFFIX");

            xmpp_stanza_t* name_suffix_text = xmpp_stanza_new(ctx);
            xmpp_stanza_set_text(name_suffix_text, vcard->name.suffix);
            xmpp_stanza_add_child(name_suffix, name_suffix_text);
            xmpp_stanza_release(name_suffix_text);

            xmpp_stanza_add_child(name, name_suffix);
            xmpp_stanza_release(name_suffix);
        }

        xmpp_stanza_add_child(vcard_stanza, name);
        xmpp_stanza_release(name);
    }

    GList* pointer = g_queue_peek_head_link(vcard->elements);

    for (; pointer != NULL; pointer = pointer->next) {
        assert(pointer->data != NULL);
        vcard_element_t* element = (vcard_element_t*)pointer->data;

        switch (element->type) {
        case VCARD_NICKNAME:
        {
            // <NICKNAME> element
            xmpp_stanza_t* nickname = xmpp_stanza_new(ctx);
            xmpp_stanza_set_name(nickname, "NICKNAME");

            xmpp_stanza_t* nickname_text = xmpp_stanza_new(ctx);
            xmpp_stanza_set_text(nickname_text, element->nickname);
            xmpp_stanza_add_child(nickname, nickname_text);
            xmpp_stanza_release(nickname_text);

            xmpp_stanza_add_child(vcard_stanza, nickname);
            xmpp_stanza_release(nickname);
            break;
        }
        case VCARD_PHOTO:
        {
            // <PHOTO> element
            xmpp_stanza_t* photo = xmpp_stanza_new(ctx);
            xmpp_stanza_set_name(photo, "PHOTO");

            if (element->photo.external) {
                xmpp_stanza_t* extval = xmpp_stanza_new(ctx);
                xmpp_stanza_set_name(extval, "EXTVAL");

                xmpp_stanza_t* extval_text = xmpp_stanza_new(ctx);
                xmpp_stanza_set_text(extval_text, element->photo.extval);
                xmpp_stanza_add_child(extval, extval_text);
                xmpp_stanza_release(extval_text);

                xmpp_stanza_add_child(photo, extval);
                xmpp_stanza_release(extval);
            } else {
                xmpp_stanza_t* binval = xmpp_stanza_new(ctx);
                xmpp_stanza_set_name(binval, "BINVAL");

                gchar* base64 = g_base64_encode(element->photo.data, element->photo.length);
                xmpp_stanza_t* binval_text = xmpp_stanza_new(ctx);
                xmpp_stanza_set_text(binval_text, base64);
                g_free(base64);
                xmpp_stanza_add_child(binval, binval_text);
                xmpp_stanza_release(binval_text);

                xmpp_stanza_add_child(photo, binval);
                xmpp_stanza_release(binval);

                xmpp_stanza_t* type = xmpp_stanza_new(ctx);
                xmpp_stanza_set_name(type, "TYPE");

                xmpp_stanza_t* type_text = xmpp_stanza_new(ctx);
                xmpp_stanza_set_text(type_text, element->photo.type);
                xmpp_stanza_add_child(type, type_text);
                xmpp_stanza_release(type_text);

                xmpp_stanza_add_child(photo, type);
                xmpp_stanza_release(type);
            }

            xmpp_stanza_add_child(vcard_stanza, photo);
            xmpp_stanza_release(photo);
            break;
        }
        case VCARD_BIRTHDAY:
        {
            // <BDAY> element
            xmpp_stanza_t* birthday = xmpp_stanza_new(ctx);
            xmpp_stanza_set_name(birthday, "BDAY");

            gchar* bday_text = g_date_time_format(element->birthday, "%Y-%m-%d");
            xmpp_stanza_t* birthday_text = xmpp_stanza_new(ctx);
            xmpp_stanza_set_text(birthday_text, bday_text);
            g_free(bday_text);
            xmpp_stanza_add_child(birthday, birthday_text);
            xmpp_stanza_release(birthday_text);

            xmpp_stanza_add_child(vcard_stanza, birthday);
            xmpp_stanza_release(birthday);
            break;
        }
        case VCARD_ADDRESS:
        {
            // <ADR> element
            xmpp_stanza_t* address = xmpp_stanza_new(ctx);
            xmpp_stanza_set_name(address, "ADR");

            // Options
            if (element->address.options & VCARD_HOME) {
                xmpp_stanza_t* home = xmpp_stanza_new(ctx);
                xmpp_stanza_set_name(home, "HOME");

                xmpp_stanza_add_child(address, home);
                xmpp_stanza_release(home);
            }
            if ((element->address.options & VCARD_WORK) == VCARD_WORK) {
                xmpp_stanza_t* work = xmpp_stanza_new(ctx);
                xmpp_stanza_set_name(work, "WORK");

                xmpp_stanza_add_child(address, work);
                xmpp_stanza_release(work);
            }
            if ((element->address.options & VCARD_POSTAL) == VCARD_POSTAL) {
                xmpp_stanza_t* postal = xmpp_stanza_new(ctx);
                xmpp_stanza_set_name(postal, "POSTAL");

                xmpp_stanza_add_child(address, postal);
                xmpp_stanza_release(postal);
            }
            if ((element->address.options & VCARD_PARCEL) == VCARD_PARCEL) {
                xmpp_stanza_t* parcel = xmpp_stanza_new(ctx);
                xmpp_stanza_set_name(parcel, "PARCEL");

                xmpp_stanza_add_child(address, parcel);
                xmpp_stanza_release(parcel);
            }
            if ((element->address.options & VCARD_INTL) == VCARD_INTL) {
                xmpp_stanza_t* intl = xmpp_stanza_new(ctx);
                xmpp_stanza_set_name(intl, "INTL");

                xmpp_stanza_add_child(address, intl);
                xmpp_stanza_release(intl);
            }
            if ((element->address.options & VCARD_DOM) == VCARD_DOM) {
                xmpp_stanza_t* dom = xmpp_stanza_new(ctx);
                xmpp_stanza_set_name(dom, "DOM");

                xmpp_stanza_add_child(address, dom);
                xmpp_stanza_release(dom);
            }
            if ((element->address.options & VCARD_PREF) == VCARD_PREF) {
                xmpp_stanza_t* pref = xmpp_stanza_new(ctx);
                xmpp_stanza_set_name(pref, "PREF");

                xmpp_stanza_add_child(address, pref);
                xmpp_stanza_release(pref);
            }

            if (element->address.pobox) {
                xmpp_stanza_t* pobox = xmpp_stanza_new(ctx);
                xmpp_stanza_set_name(pobox, "POBOX");

                xmpp_stanza_t* pobox_text = xmpp_stanza_new(ctx);
                xmpp_stanza_set_text(pobox_text, element->address.pobox);
                xmpp_stanza_add_child(pobox, pobox_text);
                xmpp_stanza_release(pobox_text);

                xmpp_stanza_add_child(address, pobox);
                xmpp_stanza_release(pobox);
            }
            if (element->address.extaddr) {
                xmpp_stanza_t* extaddr = xmpp_stanza_new(ctx);
                xmpp_stanza_set_name(extaddr, "EXTADD");

                xmpp_stanza_t* extaddr_text = xmpp_stanza_new(ctx);
                xmpp_stanza_set_text(extaddr_text, element->address.extaddr);
                xmpp_stanza_add_child(extaddr, extaddr_text);
                xmpp_stanza_release(extaddr_text);

                xmpp_stanza_add_child(address, extaddr);
                xmpp_stanza_release(extaddr);
            }
            if (element->address.street) {
                xmpp_stanza_t* street = xmpp_stanza_new(ctx);
                xmpp_stanza_set_name(street, "STREET");

                xmpp_stanza_t* street_text = xmpp_stanza_new(ctx);
                xmpp_stanza_set_text(street_text, element->address.street);
                xmpp_stanza_add_child(street, street_text);
                xmpp_stanza_release(street_text);

                xmpp_stanza_add_child(address, street);
                xmpp_stanza_release(street);
            }
            if (element->address.locality) {
                xmpp_stanza_t* locality = xmpp_stanza_new(ctx);
                xmpp_stanza_set_name(locality, "LOCALITY");

                xmpp_stanza_t* locality_text = xmpp_stanza_new(ctx);
                xmpp_stanza_set_text(locality_text, element->address.locality);
                xmpp_stanza_add_child(locality, locality_text);
                xmpp_stanza_release(locality_text);

                xmpp_stanza_add_child(address, locality);
                xmpp_stanza_release(locality);
            }
            if (element->address.region) {
                xmpp_stanza_t* region = xmpp_stanza_new(ctx);
                xmpp_stanza_set_name(region, "REGION");

                xmpp_stanza_t* region_text = xmpp_stanza_new(ctx);
                xmpp_stanza_set_text(region_text, element->address.region);
                xmpp_stanza_add_child(region, region_text);
                xmpp_stanza_release(region_text);

                xmpp_stanza_add_child(address, region);
                xmpp_stanza_release(region);
            }
            if (element->address.pcode) {
                xmpp_stanza_t* pcode = xmpp_stanza_new(ctx);
                xmpp_stanza_set_name(pcode, "PCODE");

                xmpp_stanza_t* pcode_text = xmpp_stanza_new(ctx);
                xmpp_stanza_set_text(pcode_text, element->address.pcode);
                xmpp_stanza_add_child(pcode, pcode_text);
                xmpp_stanza_release(pcode_text);

                xmpp_stanza_add_child(address, pcode);
                xmpp_stanza_release(pcode);
            }
            if (element->address.country) {
                xmpp_stanza_t* ctry = xmpp_stanza_new(ctx);
                xmpp_stanza_set_name(ctry, "CTRY");

                xmpp_stanza_t* ctry_text = xmpp_stanza_new(ctx);
                xmpp_stanza_set_text(ctry_text, element->address.country);
                xmpp_stanza_add_child(ctry, ctry_text);
                xmpp_stanza_release(ctry_text);

                xmpp_stanza_add_child(address, ctry);
                xmpp_stanza_release(ctry);
            }

            xmpp_stanza_add_child(vcard_stanza, address);
            xmpp_stanza_release(address);
            break;
        }
        case VCARD_TELEPHONE:
        {
            // <TEL> element
            xmpp_stanza_t* tel = xmpp_stanza_new(ctx);
            xmpp_stanza_set_name(tel, "TEL");

            // Options
            if (element->telephone.options & VCARD_HOME) {
                xmpp_stanza_t* home = xmpp_stanza_new(ctx);
                xmpp_stanza_set_name(home, "HOME");

                xmpp_stanza_add_child(tel, home);
                xmpp_stanza_release(home);
            }
            if ((element->telephone.options & VCARD_WORK) == VCARD_WORK) {
                xmpp_stanza_t* work = xmpp_stanza_new(ctx);
                xmpp_stanza_set_name(work, "WORK");

                xmpp_stanza_add_child(tel, work);
                xmpp_stanza_release(work);
            }
            if ((element->telephone.options & VCARD_TEL_VOICE) == VCARD_TEL_VOICE) {
                xmpp_stanza_t* voice = xmpp_stanza_new(ctx);
                xmpp_stanza_set_name(voice, "VOICE");

                xmpp_stanza_add_child(tel, voice);
                xmpp_stanza_release(voice);
            }
            if ((element->telephone.options & VCARD_TEL_FAX) == VCARD_TEL_FAX) {
                xmpp_stanza_t* fax = xmpp_stanza_new(ctx);
                xmpp_stanza_set_name(fax, "FAX");

                xmpp_stanza_add_child(tel, fax);
                xmpp_stanza_release(fax);
            }
            if ((element->telephone.options & VCARD_TEL_PAGER) == VCARD_TEL_PAGER) {
                xmpp_stanza_t* pager = xmpp_stanza_new(ctx);
                xmpp_stanza_set_name(pager, "PAGER");

                xmpp_stanza_add_child(tel, pager);
                xmpp_stanza_release(pager);
            }
            if ((element->telephone.options & VCARD_TEL_MSG) == VCARD_TEL_MSG) {
                xmpp_stanza_t* msg = xmpp_stanza_new(ctx);
                xmpp_stanza_set_name(msg, "MSG");

                xmpp_stanza_add_child(tel, msg);
                xmpp_stanza_release(msg);
            }
            if ((element->telephone.options & VCARD_TEL_CELL) == VCARD_TEL_CELL) {
                xmpp_stanza_t* cell = xmpp_stanza_new(ctx);
                xmpp_stanza_set_name(cell, "CELL");

                xmpp_stanza_add_child(tel, cell);
                xmpp_stanza_release(cell);
            }
            if ((element->telephone.options & VCARD_TEL_VIDEO) == VCARD_TEL_VIDEO) {
                xmpp_stanza_t* video = xmpp_stanza_new(ctx);
                xmpp_stanza_set_name(video, "VIDEO");

                xmpp_stanza_add_child(tel, video);
                xmpp_stanza_release(video);
            }
            if ((element->telephone.options & VCARD_TEL_BBS) == VCARD_TEL_BBS) {
                xmpp_stanza_t* bbs = xmpp_stanza_new(ctx);
                xmpp_stanza_set_name(bbs, "BBS");

                xmpp_stanza_add_child(tel, bbs);
                xmpp_stanza_release(bbs);
            }
            if ((element->telephone.options & VCARD_TEL_MODEM) == VCARD_TEL_MODEM) {
                xmpp_stanza_t* modem = xmpp_stanza_new(ctx);
                xmpp_stanza_set_name(modem, "MODEM");

                xmpp_stanza_add_child(tel, modem);
                xmpp_stanza_release(modem);
            }
            if ((element->telephone.options & VCARD_TEL_ISDN) == VCARD_TEL_ISDN) {
                xmpp_stanza_t* isdn = xmpp_stanza_new(ctx);
                xmpp_stanza_set_name(isdn, "ISDN");

                xmpp_stanza_add_child(tel, isdn);
                xmpp_stanza_release(isdn);
            }
            if ((element->telephone.options & VCARD_TEL_PCS) == VCARD_TEL_PCS) {
                xmpp_stanza_t* pcs = xmpp_stanza_new(ctx);
                xmpp_stanza_set_name(pcs, "PCS");

                xmpp_stanza_add_child(tel, pcs);
                xmpp_stanza_release(pcs);
            }
            if ((element->telephone.options & VCARD_PREF) == VCARD_PREF) {
                xmpp_stanza_t* pref = xmpp_stanza_new(ctx);
                xmpp_stanza_set_name(pref, "PREF");

                xmpp_stanza_add_child(tel, pref);
                xmpp_stanza_release(pref);
            }

            if (element->telephone.number) {
                xmpp_stanza_t* number = xmpp_stanza_new(ctx);
                xmpp_stanza_set_name(number, "NUMBER");

                xmpp_stanza_t* number_text = xmpp_stanza_new(ctx);
                xmpp_stanza_set_text(number_text, element->telephone.number);
                xmpp_stanza_add_child(number, number_text);
                xmpp_stanza_release(number_text);

                xmpp_stanza_add_child(tel, number);
                xmpp_stanza_release(number);
            }
            xmpp_stanza_add_child(vcard_stanza, tel);
            xmpp_stanza_release(tel);
            break;
        }
        case VCARD_EMAIL:
        {
            xmpp_stanza_t* email = xmpp_stanza_new(ctx);
            xmpp_stanza_set_name(email, "EMAIL");

            if (element->email.options & VCARD_HOME) {
                xmpp_stanza_t* home = xmpp_stanza_new(ctx);
                xmpp_stanza_set_name(home, "HOME");

                xmpp_stanza_add_child(email, home);
                xmpp_stanza_release(home);
            }
            if ((element->email.options & VCARD_WORK) == VCARD_WORK) {
                xmpp_stanza_t* work = xmpp_stanza_new(ctx);
                xmpp_stanza_set_name(work, "WORK");

                xmpp_stanza_add_child(email, work);
                xmpp_stanza_release(work);
            }
            if ((element->email.options & VCARD_EMAIL_X400) == VCARD_EMAIL_X400) {
                xmpp_stanza_t* x400 = xmpp_stanza_new(ctx);
                xmpp_stanza_set_name(x400, "X400");

                xmpp_stanza_add_child(email, x400);
                xmpp_stanza_release(x400);
            }
            if ((element->email.options & VCARD_PREF) == VCARD_PREF) {
                xmpp_stanza_t* pref = xmpp_stanza_new(ctx);
                xmpp_stanza_set_name(pref, "PREF");

                xmpp_stanza_add_child(email, pref);
                xmpp_stanza_release(pref);
            }

            if (element->email.userid) {
                xmpp_stanza_t* userid = xmpp_stanza_new(ctx);
                xmpp_stanza_set_name(userid, "USERID");

                xmpp_stanza_t* userid_text = xmpp_stanza_new(ctx);
                xmpp_stanza_set_text(userid_text, element->email.userid);
                xmpp_stanza_add_child(userid, userid_text);
                xmpp_stanza_release(userid_text);

                xmpp_stanza_add_child(email, userid);
                xmpp_stanza_release(userid);
            }

            xmpp_stanza_add_child(vcard_stanza, email);
            xmpp_stanza_release(email);
            break;
        }
        case VCARD_JID:
        {
            xmpp_stanza_t* jid = xmpp_stanza_new(ctx);
            xmpp_stanza_set_name(jid, "JABBERID");

            if (element->jid) {
                xmpp_stanza_t* jid_text = xmpp_stanza_new(ctx);
                xmpp_stanza_set_text(jid_text, element->jid);
                xmpp_stanza_add_child(jid, jid_text);
                xmpp_stanza_release(jid_text);
            }

            xmpp_stanza_add_child(vcard_stanza, jid);
            xmpp_stanza_release(jid);
            break;
        }
        case VCARD_TITLE:
        {
            xmpp_stanza_t* title = xmpp_stanza_new(ctx);
            xmpp_stanza_set_name(title, "TITLE");

            if (element->title) {
                xmpp_stanza_t* title_text = xmpp_stanza_new(ctx);
                xmpp_stanza_set_text(title_text, element->title);
                xmpp_stanza_add_child(title, title_text);
                xmpp_stanza_release(title_text);
            }

            xmpp_stanza_add_child(vcard_stanza, title);
            xmpp_stanza_release(title);
            break;
        }
        case VCARD_ROLE:
        {
            xmpp_stanza_t* role = xmpp_stanza_new(ctx);
            xmpp_stanza_set_name(role, "ROLE");

            if (element->role) {
                xmpp_stanza_t* role_text = xmpp_stanza_new(ctx);
                xmpp_stanza_set_text(role_text, element->role);
                xmpp_stanza_add_child(role, role_text);
                xmpp_stanza_release(role_text);
            }

            xmpp_stanza_add_child(vcard_stanza, role);
            xmpp_stanza_release(role);
            break;
        }
        case VCARD_NOTE:
        {
            xmpp_stanza_t* note = xmpp_stanza_new(ctx);
            xmpp_stanza_set_name(note, "NOTE");

            if (element->note) {
                xmpp_stanza_t* note_text = xmpp_stanza_new(ctx);
                xmpp_stanza_set_text(note_text, element->note);
                xmpp_stanza_add_child(note, note_text);
                xmpp_stanza_release(note_text);
            }

            xmpp_stanza_add_child(vcard_stanza, note);
            xmpp_stanza_release(note);
            break;
        }
        case VCARD_URL:
        {
            xmpp_stanza_t* url = xmpp_stanza_new(ctx);
            xmpp_stanza_set_name(url, "URL");

            if (element->url) {
                xmpp_stanza_t* url_text = xmpp_stanza_new(ctx);
                xmpp_stanza_set_text(url_text, element->url);
                xmpp_stanza_add_child(url, url_text);
                xmpp_stanza_release(url_text);
            }

            xmpp_stanza_add_child(vcard_stanza, url);
            xmpp_stanza_release(url);
            break;
        }
        }
    }

    return vcard_stanza;
}

static int
_vcard_print_result(xmpp_stanza_t* const stanza, void* userdata)
{
    _userdata* data = (_userdata*)userdata;
    const char* from = xmpp_stanza_get_attribute(stanza, STANZA_ATTR_FROM);

    if (from) {
        win_println(data->window, THEME_DEFAULT, "!", "vCard for %s", from);
    } else {
        win_println(data->window, THEME_DEFAULT, "!", "This account's vCard");
    }

    xmpp_stanza_t* vcard_xml = xmpp_stanza_get_child_by_name(stanza, STANZA_NAME_VCARD);
    if (!vcard_parse(vcard_xml, data->vcard)) {
        return 1;
    }

    win_show_vcard(data->window, data->vcard);

    return 1;
}

void
vcard_print(xmpp_ctx_t* ctx, ProfWin* window, char* jid)
{
    if (!jid && vcard_user && vcard_user->modified) {
        win_println(window, THEME_DEFAULT, "!", "This account's vCard (modified, `/vcard upload` to push)");
        win_show_vcard(window, vcard_user);
        return;
    }

    _userdata* data = calloc(1, sizeof(_userdata));
    data->vcard = vcard_new();
    if (!data || !data->vcard) {
        if (data) {
            free(data);
        }

        cons_show("vCard allocation failed");
        return;
    }

    data->window = window;

    char* id = connection_create_stanza_id();
    xmpp_stanza_t* iq = stanza_create_vcard_request_iq(ctx, jid, id);

    iq_id_handler_add(id, _vcard_print_result, (ProfIqFreeCallback)_free_userdata, data);

    free(id);
    iq_send_stanza(iq);
    xmpp_stanza_release(iq);
}

static int
_vcard_photo_result(xmpp_stanza_t* const stanza, void* userdata)
{
    _userdata* data = (_userdata*)userdata;
    const char* from = xmpp_stanza_get_attribute(stanza, STANZA_ATTR_FROM);
    vcard_element_photo_t* photo = NULL;

    xmpp_stanza_t* vcard_xml = xmpp_stanza_get_child_by_name(stanza, STANZA_NAME_VCARD);
    if (!vcard_parse(vcard_xml, data->vcard)) {
        return 1;
    }

    if (data->photo_index < 0) {
        GList* list_pointer;
        for (list_pointer = g_queue_peek_head_link(data->vcard->elements); list_pointer != NULL; list_pointer = list_pointer->next) {
            vcard_element_t* element = list_pointer->data;

            if (element->type != VCARD_PHOTO) {
                continue;
            } else {
                photo = &element->photo;
            }
        }

        if (photo == NULL) {
            cons_show_error("No photo was found in vCard");
            return 1;
        }
    } else {
        vcard_element_t* element = (vcard_element_t*)g_queue_peek_nth(data->vcard->elements, data->photo_index);

        if (element == NULL) {
            cons_show_error("No element was found at index %d", data->photo_index);
            return 1;
        } else if (element->type != VCARD_PHOTO) {
            cons_show_error("Element is not a photo");
            return 1;
        }

        photo = &element->photo;
    }

    if (photo->external) {
        cons_show_error("Cannot handle external value: %s", photo->extval);
        return 1;
    }

    GString* filename;

    if (!data->filename) {
        char* path = files_get_data_path(DIR_PHOTOS);
        filename = g_string_new(path);
        free(path);
        g_string_append(filename, "/");

        errno = 0;
        int res = g_mkdir_with_parents(filename->str, S_IRWXU);
        if (res == -1) {
            const char* errmsg = strerror(errno);
            if (errmsg) {
                cons_show_error("Error creating directory %s: %s", filename->str, errmsg);
                g_string_free(filename, TRUE);
                return 1;
            } else {
                cons_show_error("Unknown error creating directory %s", filename->str);
                g_string_free(filename, TRUE);
            }
        }
        gchar* from1 = str_replace(from, "@", "_at_");
        gchar* from2 = str_replace(from1, "/", "_slash_");
        g_free(from1);

        g_string_append(filename, from2);
        g_free(from2);
    } else {
        filename = g_string_new(data->filename);
    }

    // check a few image types ourselves
    // TODO: we could use /etc/mime-types
    if (g_strcmp0(photo->type, "image/png") == 0) {
        g_string_append(filename, ".png");
    } else if (g_strcmp0(photo->type, "image/jpeg") == 0) {
        g_string_append(filename, ".jpeg");
    } else if (g_strcmp0(photo->type, "image/webp") == 0) {
        g_string_append(filename, ".webp");
    }

    GError* err = NULL;

    if (g_file_set_contents(filename->str, (gchar*)photo->data, photo->length, &err) == FALSE) {
        cons_show_error("Unable to save photo: %s", err->message);
        g_error_free(err);
        g_string_free(filename, TRUE);
        return 1;
    } else {
        cons_show("Photo saved as %s", filename->str);
    }

    if (data->open) {
        gchar** argv;
        gint argc;

        gchar* cmdtemplate = prefs_get_string(PREF_VCARD_PHOTO_CMD);

        // this makes it work with filenames that contain spaces
        g_string_prepend(filename, "\"");
        g_string_append(filename, "\"");
        gchar* cmd = str_replace(cmdtemplate, "%p", filename->str);
        g_free(cmdtemplate);

        if (g_shell_parse_argv(cmd, &argc, &argv, &err) == FALSE) {
            cons_show_error("Failed to parse command template");
            g_free(cmd);
        } else {
            if (!call_external(argv, NULL, NULL)) {
                cons_show_error("Unable to execute command");
            }
            g_strfreev(argv);
        }
    }

    g_string_free(filename, TRUE);

    return 1;
}

void
vcard_photo(xmpp_ctx_t* ctx, char* jid, char* filename, int index, gboolean open)
{
    _userdata* data = calloc(1, sizeof(_userdata));
    data->vcard = vcard_new();

    if (!data || !data->vcard) {
        if (data) {
            free(data);
        }

        cons_show("vCard allocation failed");
        return;
    }

    data->photo_index = index;
    data->open = open;

    if (filename) {
        data->filename = strdup(filename);
    }

    char* id = connection_create_stanza_id();
    xmpp_stanza_t* iq = stanza_create_vcard_request_iq(ctx, jid, id);

    iq_id_handler_add(id, _vcard_photo_result, (ProfIqFreeCallback)_free_userdata, data);

    free(id);
    iq_send_stanza(iq);
    xmpp_stanza_release(iq);
}

static int
_vcard_refresh_result(xmpp_stanza_t* const stanza, void* userdata)
{
    xmpp_stanza_t* vcard_xml = xmpp_stanza_get_child_by_name(stanza, STANZA_NAME_VCARD);

    vcard_free_full(vcard_user);
    if (!vcard_parse(vcard_xml, vcard_user)) {
        return 1;
    }

    cons_show("vCard refreshed");
    vcard_user->modified = FALSE;
    return 1;
}

void
vcard_user_refresh(void)
{
    if (!vcard_user) {
        vcard_user = vcard_new();
    }

    if (!vcard_user) {
        return;
    }

    char* id = connection_create_stanza_id();
    xmpp_stanza_t* iq = stanza_create_vcard_request_iq(connection_get_ctx(), NULL, id);

    iq_id_handler_add(id, _vcard_refresh_result, NULL, NULL);

    free(id);
    iq_send_stanza(iq);
    xmpp_stanza_release(iq);
}

// Upload a vCard and set it as the currently connected account's vCard
void
vcard_upload(xmpp_ctx_t* ctx, vCard* vcard)
{
    char* id = connection_create_stanza_id();
    xmpp_stanza_t* iq = xmpp_iq_new(ctx, STANZA_TYPE_SET, id);
    xmpp_stanza_set_from(iq, connection_get_fulljid());

    xmpp_stanza_t* vcard_stanza = vcard_to_xml(ctx, vcard);

    if (!vcard_stanza) {
        xmpp_stanza_release(iq);
        free(id);
        return;
    }

    xmpp_stanza_add_child(iq, vcard_stanza);
    xmpp_stanza_release(vcard_stanza);

    free(id);
    iq_send_stanza(iq);
    xmpp_stanza_release(iq);
}

void
vcard_user_save(void)
{
    vcard_upload(connection_get_ctx(), vcard_user);
    vcard_user->modified = FALSE;
    vcard_user->avatar_modified = FALSE;
}

void
vcard_user_set_fullname(char* fullname)
{
    if (vcard_user->fullname) {
        free(vcard_user->fullname);
    }
    if (fullname) {
        vcard_user->fullname = strdup(fullname);
    } else {
        vcard_user->fullname = NULL;
    }

    vcard_user->modified = TRUE;
}

void
vcard_user_set_name_family(char* family)
{
    if (vcard_user->name.family) {
        free(vcard_user->name.family);
    }
    if (family) {
        vcard_user->name.family = strdup(family);
    } else {
        vcard_user->name.family = NULL;
    }

    vcard_user->modified = TRUE;
}

void
vcard_user_set_name_given(char* given)
{
    if (vcard_user->name.given) {
        free(vcard_user->name.given);
    }
    if (given) {
        vcard_user->name.given = strdup(given);
    } else {
        vcard_user->name.given = NULL;
    }

    vcard_user->modified = TRUE;
}

void
vcard_user_set_name_middle(char* middle)
{
    if (vcard_user->name.middle) {
        free(vcard_user->name.middle);
    }
    if (middle) {
        vcard_user->name.middle = strdup(middle);
    } else {
        vcard_user->name.middle = NULL;
    }

    vcard_user->modified = TRUE;
}

void
vcard_user_set_name_prefix(char* prefix)
{
    if (vcard_user->name.prefix) {
        free(vcard_user->name.prefix);
    }
    if (prefix) {
        vcard_user->name.prefix = strdup(prefix);
    } else {
        vcard_user->name.prefix = NULL;
    }

    vcard_user->modified = TRUE;
}

void
vcard_user_set_name_suffix(char* suffix)
{
    if (vcard_user->name.suffix) {
        free(vcard_user->name.suffix);
    }
    if (suffix) {
        vcard_user->name.suffix = strdup(suffix);
    } else {
        vcard_user->name.suffix = NULL;
    }

    vcard_user->modified = TRUE;
}

void
vcard_user_add_element(vcard_element_t* element)
{
    g_queue_push_tail(vcard_user->elements, element);
    vcard_user->modified = TRUE;
}

void
vcard_user_remove_element(unsigned int index)
{
    void* pointer = g_queue_pop_nth(vcard_user->elements, index);

    if (pointer) {
        _free_vcard_element(pointer);
        vcard_user->modified = TRUE;
    }
}

vcard_element_t*
vcard_user_get_element_index(unsigned int index)
{
    return g_queue_peek_nth(vcard_user->elements, index);
}

ProfWin*
vcard_user_create_win(void)
{
    return wins_new_vcard(vcard_user);
}

void
vcard_user_free(void)
{
    if (vcard_user) {
        vcard_free_full(vcard_user);
        vcard_free(vcard_user);
    }
    vcard_user = NULL;
}
