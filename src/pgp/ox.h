/*
 * ox.h
 * vim: expandtab:ts=4:sts=4:sw=4
 *
 * Copyright (C) 2020 Stefan Kropp <stefan@debxwoody.de>
 * Copyright (C) 2020 - 2026 Michael Vetter <jubalh@iodoru.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later WITH OpenSSL-exception
 */

#ifndef PGP_OX_H
#define PGP_OX_H

#include <pgp/gpg.h>

gchar* p_ox_gpg_signcrypt(const gchar* const sender_barejid, const gchar* const recipient_barejid, const gchar* const message);
gchar* p_ox_gpg_decrypt(const gchar* base64);
void p_ox_gpg_readkey(const gchar* const filename, gchar** key, gchar** fp);
gboolean p_ox_gpg_import(const gchar* base64_public_key);

GHashTable* ox_gpg_public_keys(void);
GHashTable* ox_gpg_list_keys(void);

gboolean ox_is_private_key_available(const gchar* const barejid);
gboolean ox_is_public_key_available(const gchar* const barejid);

#endif
