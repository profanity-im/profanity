/*
 * vcard.h
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

#ifndef XMPP_VCARD_H
#define XMPP_VCARD_H

#include <glib.h>

// 17 bits
typedef enum {
    VCARD_HOME = 1,
    VCARD_WORK = 2,
    VCARD_POSTAL = 4,
    VCARD_PARCEL = 8,
    VCARD_INTL = 16,
    VCARD_PREF = 32,
    VCARD_TEL_VOICE = 64,
    VCARD_TEL_FAX = 128,
    VCARD_TEL_PAGER = 256,
    VCARD_TEL_MSG = 512,
    VCARD_TEL_CELL = 1024,
    VCARD_TEL_VIDEO = 2048,
    VCARD_TEL_BBS = 4096,
    VCARD_TEL_MODEM = 8192,
    VCARD_TEL_ISDN = 16384,
    VCARD_TEL_PCS = 32768,
    VCARD_EMAIL_X400 = 65536,
    VCARD_EMAIL_INTERNET = 131072,
    VCARD_DOM = 262144
} vcard_element_options_t;

typedef struct _vcard_name
{
    char *family, *given, *middle, *prefix, *suffix;
} vcard_name_t;

typedef struct _vcard_element_photo
{
    union {
        struct
        {
            guchar* data;
            char* type;
            gsize length;
        };
        char* extval;
    };
    gboolean external;
} vcard_element_photo_t;

typedef struct _vcard_element_address
{
    char *pobox, *extaddr, *street, *locality, *region, *pcode, *country;

    // Options used:
    // VCARD_HOME
    // VCARD_WORK
    // VCARD_POSTAL
    // VCARD_PARCEL
    // VCARD_DOM
    // VCARD_INTL
    // VCARD_PREF
    vcard_element_options_t options;
} vcard_element_address_t;

typedef struct _vcard_element_telephone
{
    char* number;

    // Options used:
    // VCARD_HOME
    // VCARD_WORK
    // VCARD_TEL_VOICE
    // VCARD_TEL_FAX
    // VCARD_TEL_PAGER
    // VCARD_TEL_MSG
    // VCARD_TEL_CELL
    // VCARD_TEL_VIDEO
    // VCARD_TEL_BBS
    // VCARD_TEL_MODEM
    // VCARD_TEL_ISDN
    // VCARD_TEL_PCS
    // VCARD_PREF
    vcard_element_options_t options;
} vcard_element_telephone_t;

typedef struct _vcard_element_email
{
    char* userid;

    // Options used:
    // VCARD_HOME
    // VCARD_WORK
    // VCARD_EMAIL_X400
    // VCARD_PREF
    vcard_element_options_t options;
} vcard_element_email_t;

typedef enum _vcard_element_type {
    VCARD_NICKNAME,
    VCARD_PHOTO,
    VCARD_BIRTHDAY,
    VCARD_ADDRESS,
    VCARD_TELEPHONE,
    VCARD_EMAIL,
    VCARD_JID,
    VCARD_TITLE,
    VCARD_ROLE,
    VCARD_NOTE,
    VCARD_URL
} vcard_element_type;

typedef struct _vcard_element
{
    vcard_element_type type;

    union {
        char *nickname, *jid, *title, *role, *note, *url;
        vcard_element_photo_t photo;
        GDateTime* birthday;
        vcard_element_address_t address;
        vcard_element_telephone_t telephone;
        vcard_element_email_t email;
    };
} vcard_element_t;

typedef struct _vcard
{
    // These elements are only meant to appear once (per DTD)
    vcard_name_t name;
    char* fullname;

    gboolean modified;
    gboolean avatar_modified;

    // GQueue of vcard_element*
    GQueue* elements;
} vCard;

#endif
