/*
 * otrlib.h
 *
 * Copyright (C) 2012 - 2014 James Booth <boothj5@gmail.com>
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
 * along with Profanity.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#ifndef OTRLIB_H
#define OTRLIB_H

OtrlPolicy otrlib_policy(void);

char* otrlib_start_query(void);

void otrlib_init_ops(OtrlMessageAppOps *ops);

ConnContext * otrlib_context_find(OtrlUserState user_state, const char * const recipient, char *jid);

void otrlib_end_session(OtrlUserState user_state, const char * const recipient, char *jid, OtrlMessageAppOps *ops);

gcry_error_t otrlib_encrypt_message(OtrlUserState user_state, OtrlMessageAppOps *ops, char *jid, const char * const to,
    const char * const message, char **newmessage);

int otrlib_decrypt_message(OtrlUserState user_state, OtrlMessageAppOps *ops, char *jid, const char * const from,
    const char * const message, char **decrypted, OtrlTLV **tlvs);

#endif
