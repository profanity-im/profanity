/*
 * otrlib.h
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

#ifndef OTR_OTRLIB_H
#define OTR_OTRLIB_H

OtrlPolicy otrlib_policy(void);

char* otrlib_start_query(void);

void otrlib_init_ops(OtrlMessageAppOps *ops);

void otrlib_init_timer(void);
void otrlib_poll(void);

ConnContext* otrlib_context_find(OtrlUserState user_state, const char *const recipient, char *jid);

void otrlib_end_session(OtrlUserState user_state, const char *const recipient, char *jid, OtrlMessageAppOps *ops);

gcry_error_t otrlib_encrypt_message(OtrlUserState user_state, OtrlMessageAppOps *ops, char *jid, const char *const to,
    const char *const message, char **newmessage);

int otrlib_decrypt_message(OtrlUserState user_state, OtrlMessageAppOps *ops, char *jid, const char *const from,
    const char *const message, char **decrypted, OtrlTLV **tlvs);

void otrlib_handle_tlvs(OtrlUserState user_state, OtrlMessageAppOps *ops, ConnContext *context, OtrlTLV *tlvs, GHashTable *smp_initiators);

#endif
