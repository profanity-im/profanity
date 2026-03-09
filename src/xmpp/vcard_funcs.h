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
void vcard_user_set_name_middle(char* middle);
void vcard_user_set_name_prefix(char* prefix);
void vcard_user_set_name_suffix(char* suffix);

void vcard_user_add_element(vcard_element_t* element);
void vcard_user_remove_element(unsigned int index);
vcard_element_t* vcard_user_get_element_index(unsigned int index);
ProfWin* vcard_user_create_win();

void vcard_user_free(void);
#endif
