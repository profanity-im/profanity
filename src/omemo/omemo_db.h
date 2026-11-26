/*
 * omemo_db.h
 * vim: expandtab:ts=4:sts=4:sw=4
 *
 * Copyright (C) 2026 Michael Vetter <jubalh@iodoru.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later WITH OpenSSL-exception
 */

#ifndef OMEMO_DB_H
#define OMEMO_DB_H

#include "config.h"

#include <glib.h>
#include <stdint.h>
#include "config/account.h"

#define OMEMO_TRUST_GROUP_VERIFIED "verified_contacts"

gboolean omemo_db_init(const ProfAccount* account);
void omemo_db_close(void);

void omemo_db_migrate_legacy_files(const ProfAccount* account);

gboolean omemo_db_has_identity(void);
gboolean omemo_db_save_identity(uint32_t device_id, uint32_t registration_id,
                                const uint8_t* pub_key, size_t pub_key_len,
                                const uint8_t* priv_key, size_t priv_key_len);
gboolean omemo_db_load_identity(uint32_t* device_id, uint32_t* registration_id,
                                uint8_t** pub_key, size_t* pub_key_len,
                                uint8_t** priv_key, size_t* priv_key_len);

gboolean omemo_db_save_pre_key(uint32_t pre_key_id, const uint8_t* record, size_t record_len);
gboolean omemo_db_remove_pre_key(uint32_t pre_key_id);
gint omemo_db_load_all_pre_keys(GHashTable* pre_key_store, uint32_t* max_pre_key_id);

gboolean omemo_db_save_signed_pre_key(uint32_t signed_pre_key_id, const uint8_t* record, size_t record_len);
gboolean omemo_db_remove_signed_pre_key(uint32_t signed_pre_key_id);
gint omemo_db_load_all_signed_pre_keys(GHashTable* signed_pre_key_store, uint32_t* signed_pre_key_id);

gboolean omemo_db_save_session(const char* name, uint32_t device_id, const uint8_t* record, size_t record_len);
gboolean omemo_db_delete_session(const char* name, uint32_t device_id);
gint omemo_db_delete_all_sessions(const char* name);
void omemo_db_load_all_sessions(GHashTable* session_store);

gboolean omemo_db_save_trust(const char* name, uint32_t device_id, const uint8_t* key, size_t key_len);
gboolean omemo_db_remove_trust(const char* name, uint32_t device_id);
void omemo_db_load_all_trust(GHashTable* trusted_table);
gboolean omemo_db_is_contact_verified(const char* barejid);
void omemo_db_set_contact_verified(const char* barejid, gboolean verified);

gboolean omemo_db_save_device(const char* name, uint32_t device_id, const char* fingerprint);
void omemo_db_load_all_known_devices(GHashTable* known_devices, GHashTable* fingerprint_ac);

#endif
