/*
 * ox.h
 * vim: expandtab:ts=4:sts=4:sw=4
 *
 * Copyright (C) 2020 Stefan Kropp <stefan@debxwoody.de>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later WITH OpenSSL-exception
 */

/*!
 * \page OX OX Implementation
 *
 * \section OX XEP-0373: OpenPGP for XMPP
 * XEP-0373: OpenPGP for XMPP (OX) is the implementation of OpenPGP for XMPP
 * replace the XEP-0027.
 *
 * https://xmpp.org/extensions/xep-0373.html
 */

/*!
 * \brief Announcing OpenPGP public key from file to PEP.
 *
 * Reads the public key from the given file. Checks the key-information and
 * pushes the key on PEP.
 *
 * https://xmpp.org/extensions/xep-0373.html#announcing-pubkey
 *
 * \param filename name of the file with the public key
 * \return TRUE: success; FALSE: failed
 */

gboolean ox_announce_public_key(const char* const filename);

/*!
 * \brief Discovering Public Keys of a User.
 *
 * Reads the public key from a JIDs PEP.
 *
 * \param jid JID
 */

void ox_discover_public_key(const char* const jid);

void ox_request_public_key(const char* const jid, const char* const fingerprint);
