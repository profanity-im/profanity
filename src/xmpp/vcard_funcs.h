/*
 * vcard_funcs.h
 * vim: expandtab:ts=4:sts=4:sw=4
 *
 * Copyright (C) 2022 Marouane L. <techmetx11@disroot.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later WITH OpenSSL-exception
 */

#ifndef XMPP_VCARD_FUNCS_H
#define XMPP_VCARD_FUNCS_H

#include "ui/win_types.h"
#include "xmpp/vcard.h"

vCard* vcard_new();
void vcard_free(vCard* vcard);
void vcard_free_full(vCard* vcard);

gboolean vcard_parse(xmpp_stanza_t* vcard_xml, vCard* vcard);

void vcard_print(xmpp_ctx_t* ctx, ProfWin* window, char* jid);
void vcard_photo(xmpp_ctx_t* ctx, char* jid, char* filename, int index, gboolean open);

void vcard_user_refresh(void);
void vcard_user_save(void);
void vcard_user_set_fullname(char* fullname);
void vcard_user_set_name_family(char* family);
void vcard_user_set_name_given(char* given);

void vcard_user_free(void);

DataForm* vcard_create_form(vCard* vcard);
DataForm* vcard_user_create_form(void);
void vcard_user_apply_form(DataForm* form);
#endif
