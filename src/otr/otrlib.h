/*
 * otrlib.h
 * vim: expandtab:ts=4:sts=4:sw=4
 *
 * Copyright (C) 2012 - 2019 James Booth <boothj5@gmail.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later WITH OpenSSL-exception
 */

#ifndef OTR_OTRLIB_H
#define OTR_OTRLIB_H

OtrlPolicy otrlib_policy(void);

char* otrlib_start_query(void);

void otrlib_init_ops(OtrlMessageAppOps* ops);

void otrlib_init_timer(void);
void otrlib_poll(void);
void otrlib_shutdown(void);

ConnContext* otrlib_context_find(OtrlUserState user_state, const char* const recipient, char* jid);

void otrlib_end_session(OtrlUserState user_state, const char* const recipient, char* jid, OtrlMessageAppOps* ops);

gcry_error_t otrlib_encrypt_message(OtrlUserState user_state, OtrlMessageAppOps* ops, char* jid, const char* const to,
                                    const char* const message, char** newmessage);

int otrlib_decrypt_message(OtrlUserState user_state, OtrlMessageAppOps* ops, char* jid, const char* const from,
                           const char* const message, char** decrypted, OtrlTLV** tlvs);

void otrlib_handle_tlvs(OtrlUserState user_state, OtrlMessageAppOps* ops, ConnContext* context, OtrlTLV* tlvs, GHashTable* smp_initiators);

#endif
