/*
 * store.c
 *
 * Copyright (C) 2019 Paul Fariello <paul@fariello.eu>
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
#include <glib.h>
#include <signal/signal_protocol.h>

#include "config.h"
#include "omemo/omemo.h"
#include "omemo/store.h"

GHashTable *
session_store_new(void)
{
    return g_hash_table_new_full(g_str_hash, g_str_equal, free, NULL);
}

GHashTable *
pre_key_store_new(void)
{
    return g_hash_table_new_full(g_direct_hash, g_direct_equal, NULL, (GDestroyNotify)signal_buffer_free);
}

GHashTable *
signed_pre_key_store_new(void)
{
    return g_hash_table_new_full(g_direct_hash, g_direct_equal, NULL, (GDestroyNotify)signal_buffer_free);
}

void
identity_key_store_new(identity_key_store_t *identity_key_store)
{
    identity_key_store->trusted = g_hash_table_new_full(g_str_hash, g_str_equal, free, (GDestroyNotify)signal_buffer_free);
    identity_key_store->private = NULL;
    identity_key_store->public = NULL;
}

#ifdef HAVE_LIBSIGNAL_LT_2_3_2
int
load_session(signal_buffer **record, const signal_protocol_address *address,
    void *user_data)
#else
int
load_session(signal_buffer **record, signal_buffer **user_record,
    const signal_protocol_address *address, void *user_data)
#endif
{
    GHashTable *session_store = (GHashTable *)user_data;
    GHashTable *device_store = NULL;

    device_store = g_hash_table_lookup(session_store, address->name);
    if (!device_store) {
        *record = NULL;
        return 0;
    }

    signal_buffer *original = g_hash_table_lookup(device_store, GINT_TO_POINTER(address->device_id));
    if (!original) {
        *record = NULL;
        return 0;
    }
    *record = signal_buffer_copy(original);
    return 1;
}

int
get_sub_device_sessions(signal_int_list **sessions, const char *name,
    size_t name_len, void *user_data)
{
    GHashTable *session_store = (GHashTable *)user_data;
    GHashTable *device_store = NULL;
    GHashTableIter iter;
    gpointer key, value;

    device_store = g_hash_table_lookup(session_store, name);
    if (!device_store) {
        return SG_SUCCESS;
    }

    *sessions = signal_int_list_alloc();
    g_hash_table_iter_init(&iter, device_store);
    while (g_hash_table_iter_next(&iter, &key, &value)) {
        signal_int_list_push_back(*sessions, GPOINTER_TO_INT(key));
    }


    return SG_SUCCESS;
}

#ifdef HAVE_LIBSIGNAL_LT_2_3_2
int
store_session(const signal_protocol_address *address, uint8_t *record,
    size_t record_len, void *user_data)
#else
int
store_session(const signal_protocol_address *address,
    uint8_t *record, size_t record_len,
    uint8_t *user_record, size_t user_record_len,
    void *user_data)
#endif
{
    GHashTable *session_store = (GHashTable *)user_data;
    GHashTable *device_store = NULL;

    device_store = g_hash_table_lookup(session_store, (void *)address->name);
    if (!device_store) {
        device_store = g_hash_table_new_full(g_direct_hash, g_direct_equal, NULL, (GDestroyNotify)signal_buffer_free);
        g_hash_table_insert(session_store, strdup(address->name), device_store);
    }

    signal_buffer *buffer = signal_buffer_create(record, record_len);
    g_hash_table_insert(device_store, GINT_TO_POINTER(address->device_id), buffer);


    char *record_b64 = g_base64_encode(record, record_len);
    char *device_id = g_strdup_printf("%d", address->device_id);
    g_key_file_set_string(omemo_sessions_keyfile(), address->name, device_id, record_b64);
    free(device_id);
    g_free(record_b64);

    omemo_sessions_keyfile_save();

    return SG_SUCCESS;
}

int
contains_session(const signal_protocol_address *address, void *user_data)
{
    GHashTable *session_store = (GHashTable *)user_data;
    GHashTable *device_store = NULL;

    device_store = g_hash_table_lookup(session_store, address->name);
    if (!device_store) {
        return 0;
    }

    if (!g_hash_table_lookup(device_store, GINT_TO_POINTER(address->device_id))) {
        return 0;
    }

    return 1;
}

int
delete_session(const signal_protocol_address *address, void *user_data)
{
    GHashTable *session_store = (GHashTable *)user_data;
    GHashTable *device_store = NULL;

    device_store = g_hash_table_lookup(session_store, address->name);
    if (!device_store) {
        return SG_SUCCESS;
    }

    g_hash_table_remove(device_store, GINT_TO_POINTER(address->device_id));

    char *device_id_str = g_strdup_printf("%d", address->device_id);
    g_key_file_remove_key(omemo_sessions_keyfile(), address->name, device_id_str, NULL);
    g_free(device_id_str);
    omemo_sessions_keyfile_save();

    return SG_SUCCESS;
}

int
delete_all_sessions(const char *name, size_t name_len, void *user_data)
{
    GHashTable *session_store = (GHashTable *)user_data;
    GHashTable *device_store = NULL;

    device_store = g_hash_table_lookup(session_store, name);
    if (!device_store) {
        return SG_SUCCESS;
    }

    guint len = g_hash_table_size(device_store);
    g_hash_table_remove_all(device_store);
    return len;
}

int
load_pre_key(signal_buffer **record, uint32_t pre_key_id, void *user_data)
{
    signal_buffer *original;
    GHashTable *pre_key_store = (GHashTable *)user_data;

    original = g_hash_table_lookup(pre_key_store, GINT_TO_POINTER(pre_key_id));
    if (original == NULL) {
        return SG_ERR_INVALID_KEY_ID;
    }

    *record = signal_buffer_copy(original);
    return SG_SUCCESS;
}

int
store_pre_key(uint32_t pre_key_id, uint8_t *record, size_t record_len,
    void *user_data)
{
    GHashTable *pre_key_store = (GHashTable *)user_data;

    signal_buffer *buffer = signal_buffer_create(record, record_len);
    g_hash_table_insert(pre_key_store, GINT_TO_POINTER(pre_key_id), buffer);

    /* Long term storage */
    char *pre_key_id_str = g_strdup_printf("%d", pre_key_id);
    char *record_b64 = g_base64_encode(record, record_len);
    g_key_file_set_string(omemo_identity_keyfile(), OMEMO_STORE_GROUP_PREKEYS, pre_key_id_str, record_b64);
    g_free(pre_key_id_str);
    g_free(record_b64);

    omemo_identity_keyfile_save();

    return SG_SUCCESS;
}

int
contains_pre_key(uint32_t pre_key_id, void *user_data)
{
    GHashTable *pre_key_store = (GHashTable *)user_data;

    return g_hash_table_lookup(pre_key_store, GINT_TO_POINTER(pre_key_id)) != NULL;
}

int
remove_pre_key(uint32_t pre_key_id, void *user_data)
{
    GHashTable *pre_key_store = (GHashTable *)user_data;

    int ret = g_hash_table_remove(pre_key_store, GINT_TO_POINTER(pre_key_id));

    /* Long term storage */
    char *pre_key_id_str = g_strdup_printf("%d", pre_key_id);
    g_key_file_remove_key(omemo_identity_keyfile(), OMEMO_STORE_GROUP_PREKEYS, pre_key_id_str, NULL);
    g_free(pre_key_id_str);

    omemo_identity_keyfile_save();

    if (ret > 0) {
        return SG_SUCCESS;
    } else {
        return SG_ERR_INVALID_KEY_ID;
    }
}

int
load_signed_pre_key(signal_buffer **record, uint32_t signed_pre_key_id,
    void *user_data)
{
    signal_buffer *original;
    GHashTable *signed_pre_key_store = (GHashTable *)user_data;

    original = g_hash_table_lookup(signed_pre_key_store, GINT_TO_POINTER(signed_pre_key_id));
    if (!original) {
        return SG_ERR_INVALID_KEY_ID;
    }

    *record = signal_buffer_copy(original);
    return SG_SUCCESS;
}

int
store_signed_pre_key(uint32_t signed_pre_key_id, uint8_t *record,
    size_t record_len, void *user_data)
{
    GHashTable *signed_pre_key_store = (GHashTable *)user_data;

    signal_buffer *buffer = signal_buffer_create(record, record_len);
    g_hash_table_insert(signed_pre_key_store, GINT_TO_POINTER(signed_pre_key_id), buffer);

    /* Long term storage */
    char *signed_pre_key_id_str = g_strdup_printf("%d", signed_pre_key_id);
    char *record_b64 = g_base64_encode(record, record_len);
    g_key_file_set_string(omemo_identity_keyfile(), OMEMO_STORE_GROUP_SIGNED_PREKEYS, signed_pre_key_id_str, record_b64);
    g_free(signed_pre_key_id_str);
    g_free(record_b64);

    omemo_identity_keyfile_save();

    return SG_SUCCESS;
}

int
contains_signed_pre_key(uint32_t signed_pre_key_id, void *user_data)
{
    GHashTable *signed_pre_key_store = (GHashTable *)user_data;

    return g_hash_table_lookup(signed_pre_key_store, GINT_TO_POINTER(signed_pre_key_id)) != NULL;
}

int
remove_signed_pre_key(uint32_t signed_pre_key_id, void *user_data)
{
    GHashTable *signed_pre_key_store = (GHashTable *)user_data;

    int ret = g_hash_table_remove(signed_pre_key_store, GINT_TO_POINTER(signed_pre_key_id));

    /* Long term storage */
    char *signed_pre_key_id_str = g_strdup_printf("%d", signed_pre_key_id);
    g_key_file_remove_key(omemo_identity_keyfile(), OMEMO_STORE_GROUP_PREKEYS, signed_pre_key_id_str, NULL);
    g_free(signed_pre_key_id_str);

    omemo_identity_keyfile_save();

    return ret;
}

int
get_identity_key_pair(signal_buffer **public_data, signal_buffer **private_data,
    void *user_data)
{
    identity_key_store_t *identity_key_store = (identity_key_store_t *)user_data;

    *public_data = signal_buffer_copy(identity_key_store->public);
    *private_data = signal_buffer_copy(identity_key_store->private);

    return SG_SUCCESS;
}

int
get_local_registration_id(void *user_data, uint32_t *registration_id)
{
    identity_key_store_t *identity_key_store = (identity_key_store_t *)user_data;

    *registration_id = identity_key_store->registration_id;

    return SG_SUCCESS;
}

int
save_identity(const signal_protocol_address *address, uint8_t *key_data,
    size_t key_len, void *user_data)
{
    identity_key_store_t *identity_key_store = (identity_key_store_t *)user_data;

    if (identity_key_store->recv && !identity_key_store->trusted_msg) {
        /* Do not trust identity automatically */
        return SG_SUCCESS;
    }

    signal_buffer *buffer = signal_buffer_create(key_data, key_len);

    GHashTable *trusted = g_hash_table_lookup(identity_key_store->trusted, strdup(address->name));
    if (!trusted) {
        trusted = g_hash_table_new_full(g_direct_hash, g_direct_equal, NULL, (GDestroyNotify)signal_buffer_free);
        g_hash_table_insert(identity_key_store->trusted, strdup(address->name), trusted);
    }
    g_hash_table_insert(trusted, GINT_TO_POINTER(address->device_id), buffer);

    /* Long term storage */
    char *key_b64 = g_base64_encode(key_data, key_len);
    char *device_id = g_strdup_printf("%d", address->device_id);
    g_key_file_set_string(omemo_trust_keyfile(), address->name, strdup(device_id), key_b64);
    g_free(device_id);
    g_free(key_b64);

    omemo_trust_keyfile_save();

    return SG_SUCCESS;
}

int
is_trusted_identity(const signal_protocol_address *address, uint8_t *key_data,
    size_t key_len, void *user_data)
{
    int ret;
    identity_key_store_t *identity_key_store = (identity_key_store_t *)user_data;

    GHashTable *trusted = g_hash_table_lookup(identity_key_store->trusted, address->name);
    if (!trusted) {
        if (identity_key_store->recv) {
            identity_key_store->trusted_msg = false;
            return 1;
        } else {
            return 0;
        }
    }

    signal_buffer *buffer = signal_buffer_create(key_data, key_len);
    signal_buffer *original = g_hash_table_lookup(trusted, GINT_TO_POINTER(address->device_id));

    ret = original != NULL && signal_buffer_compare(buffer, original) == 0;

    signal_buffer_free(buffer);


    if (identity_key_store->recv) {
        identity_key_store->trusted_msg = ret;
        return 1;
    } else {
        return ret;
    }
}

int
store_sender_key(const signal_protocol_sender_key_name *sender_key_name,
    uint8_t *record, size_t record_len, uint8_t *user_record,
    size_t user_record_len, void *user_data)
{
    return SG_SUCCESS;
}

int
load_sender_key(signal_buffer **record, signal_buffer **user_record,
                const signal_protocol_sender_key_name *sender_key_name,
                void *user_data)
{
    return SG_SUCCESS;
}
