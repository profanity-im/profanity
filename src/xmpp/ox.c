/*
 * ox.c
 * vim: expandtab:ts=4:sts=4:sw=4
 *
 * Copyright (C) 2020 Stefan Kropp <stefan@debxwoody.de>
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

#include "config.h"

#include <glib.h>
#include <assert.h>

#include "log.h"
#include "ui/ui.h"
#include "xmpp/connection.h"
#include "xmpp/stanza.h"
#include "pgp/gpg.h"

#ifdef HAVE_LIBGPGME

#define KEYID_LENGTH 40

static void _ox_metadata_node__public_key(const char* const fingerprint);
static int _ox_metadata_result(xmpp_conn_t* const conn, xmpp_stanza_t* const stanza, void* const userdata);

static void _ox_request_public_key(const char* const jid, const char* const fingerprint);
static int _ox_public_key_result(xmpp_conn_t* const conn, xmpp_stanza_t* const stanza, void* const userdata);

/*!
 * \brief Current Date and Time.
 *
 * XEP-0082: XMPP Date and Time Profiles
 * https://xmpp.org/extensions/xep-0082.html
 *
 * \return YYYY-MM-DDThh:mm:ssZ
 *
 */

static char* _gettimestamp();

/*!
 *
<pre>
<iq type='set' from='juliet@example.org/balcony' id='publish1'>
  <pubsub xmlns='http://jabber.org/protocol/pubsub'>
    <publish node='urn:xmpp:openpgp:0:public-keys:123456789ABCDEF1234567891238484848484848'>
      <item id='2020-01-21T10:46:21Z'>
        <pubkey xmlns='urn:xmpp:openpgp:0'>
           <data>
             BASE64_OPENPGP_PUBLIC_KEY
           </data>
        </pubkey>
      </item>
    </publish>
  </pubsub>
</iq>
</pre>
 *
 */

gboolean
ox_announce_public_key(const char* const filename)
{
    assert(filename);

    cons_show("Annonuce OpenPGP Key for OX %s ...", filename);
    log_info("[OX] Annonuce OpenPGP Key of OX: %s", filename);

    // key the key and the fingerprint via GnuPG from file
    char* key = NULL;
    char* fp = NULL;
    p_ox_gpg_readkey(filename, &key, &fp);

    if (!(key && fp)) {
        cons_show("Error during OpenPGP OX announce. See log file for more information");
        return FALSE;
    } else {
        log_info("[OX] Annonuce OpenPGP Key for Fingerprint: %s", fp);
        xmpp_ctx_t* const ctx = connection_get_ctx();
        char* id = xmpp_uuid_gen(ctx);
        xmpp_stanza_t* iq = xmpp_iq_new(ctx, STANZA_TYPE_SET, id);
        xmpp_stanza_set_from(iq, xmpp_conn_get_jid(connection_get_conn()));

        xmpp_stanza_t* pubsub = xmpp_stanza_new(ctx);
        xmpp_stanza_set_name(pubsub, STANZA_NAME_PUBSUB);
        xmpp_stanza_set_ns(pubsub, XMPP_FEATURE_PUBSUB);

        GString* node_name = g_string_new(STANZA_NS_OPENPGP_0_PUBLIC_KEYS);
        g_string_append(node_name, ":");
        g_string_append(node_name, fp);

        xmpp_stanza_t* publish = xmpp_stanza_new(ctx);
        xmpp_stanza_set_name(publish, STANZA_NAME_PUBLISH);
        xmpp_stanza_set_attribute(publish, STANZA_ATTR_NODE, node_name->str);

        xmpp_stanza_t* item = xmpp_stanza_new(ctx);
        xmpp_stanza_set_name(item, STANZA_NAME_ITEM);
        char* timestamp = _gettimestamp();
        xmpp_stanza_set_attribute(item, STANZA_ATTR_ID, timestamp);
        free(timestamp);

        xmpp_stanza_t* pubkey = xmpp_stanza_new(ctx);
        xmpp_stanza_set_name(pubkey, STANZA_NAME_PUPKEY);
        xmpp_stanza_set_ns(pubkey, STANZA_NS_OPENPGP_0);

        xmpp_stanza_t* data = xmpp_stanza_new(ctx);
        xmpp_stanza_set_name(data, STANZA_NAME_DATA);
        xmpp_stanza_t* keydata = xmpp_stanza_new(ctx);
        xmpp_stanza_set_text(keydata, key);

        xmpp_stanza_add_child(data, keydata);
        xmpp_stanza_add_child(pubkey, data);
        xmpp_stanza_add_child(item, pubkey);
        xmpp_stanza_add_child(publish, item);
        xmpp_stanza_add_child(pubsub, publish);
        xmpp_stanza_add_child(iq, pubsub);
        xmpp_send(connection_get_conn(), iq);

        _ox_metadata_node__public_key(fp);
    }
    return TRUE;
}

/*!
 * <pre>

<iq from='romeo@example.org/orchard'
    to='juliet@example.org'
    type='get'
    id='getmeta'>
  <pubsub xmlns='http://jabber.org/protocol/pubsub'>
    <items node='urn:xmpp:openpgp:0:public-keys'/>
  </pubsub>
</iq>

 * </pre>
 *
*/

void
ox_discover_public_key(const char* const jid)
{
    assert(jid && strlen(jid) > 0);
    log_info("[OX] Discovering Public Key for %s", jid);
    cons_show("Discovering Public Key for %s", jid);
    // iq
    xmpp_ctx_t* const ctx = connection_get_ctx();
    char* id = xmpp_uuid_gen(ctx);
    xmpp_stanza_t* iq = xmpp_iq_new(ctx, STANZA_TYPE_GET, id);
    xmpp_stanza_set_from(iq, xmpp_conn_get_jid(connection_get_conn()));
    xmpp_stanza_set_to(iq, jid);
    // pubsub
    xmpp_stanza_t* pubsub = xmpp_stanza_new(ctx);
    xmpp_stanza_set_name(pubsub, STANZA_NAME_PUBSUB);
    xmpp_stanza_set_ns(pubsub, XMPP_FEATURE_PUBSUB);
    // items
    xmpp_stanza_t* items = xmpp_stanza_new(ctx);
    xmpp_stanza_set_name(items, STANZA_NAME_ITEMS);
    xmpp_stanza_set_attribute(items, STANZA_ATTR_NODE, STANZA_NS_OPENPGP_0_PUBLIC_KEYS);

    xmpp_stanza_add_child(pubsub, items);
    xmpp_stanza_add_child(iq, pubsub);

    xmpp_id_handler_add(connection_get_conn(), _ox_metadata_result, id, strdup(jid));
    xmpp_send(connection_get_conn(), iq);
    xmpp_stanza_release(iq);
}

void
ox_request_public_key(const char* const jid, const char* const fingerprint)
{
    _ox_request_public_key(jid, fingerprint);
}

/*!
 *
 *
 *
<pre>
<iq type='set' from='juliet@example.org/balcony' id='publish1'>
  <pubsub xmlns='http://jabber.org/protocol/pubsub'>
    <publish node='urn:xmpp:openpgp:0:public-keys'>
      <item>
        <public-keys-list xmlns='urn:xmpp:openpgp:0'>
          <pubkey-metadata
            v4-fingerprint='1234512345678122ABCDE2222222222222222222'
            date='2018-03-01T15:26:12Z'
            />
          <pubkey-metadata
            v4-fingerprint='1234ABCD1234409865ABCD234482728939483472'
            date='1953-05-16T12:00:00Z'
            />
        </public-keys-list>
      </item>
    </publish>
  </pubsub>
</iq>
</pre>
 *
 */

void
_ox_metadata_node__public_key(const char* const fingerprint)
{
    log_info("Annonuce OpenPGP metadata: %s", fingerprint);
    assert(fingerprint);
    assert(strlen(fingerprint) == KEYID_LENGTH);
    // iq
    xmpp_ctx_t* const ctx = connection_get_ctx();
    char* id = xmpp_uuid_gen(ctx);
    xmpp_stanza_t* iq = xmpp_iq_new(ctx, STANZA_TYPE_SET, id);
    xmpp_stanza_set_from(iq, xmpp_conn_get_jid(connection_get_conn()));
    // pubsub
    xmpp_stanza_t* pubsub = xmpp_stanza_new(ctx);
    xmpp_stanza_set_name(pubsub, STANZA_NAME_PUBSUB);
    xmpp_stanza_set_ns(pubsub, XMPP_FEATURE_PUBSUB);
    // publish
    xmpp_stanza_t* publish = xmpp_stanza_new(ctx);
    xmpp_stanza_set_name(publish, STANZA_NAME_PUBLISH);
    xmpp_stanza_set_attribute(publish, STANZA_ATTR_NODE, STANZA_NS_OPENPGP_0_PUBLIC_KEYS);
    // item
    xmpp_stanza_t* item = xmpp_stanza_new(ctx);
    xmpp_stanza_set_name(item, STANZA_NAME_ITEM);
    // public-keys-list
    xmpp_stanza_t* publickeyslist = xmpp_stanza_new(ctx);
    xmpp_stanza_set_name(publickeyslist, STANZA_NAME_PUBLIC_KEYS_LIST);
    xmpp_stanza_set_ns(publickeyslist, STANZA_NS_OPENPGP_0);
    // pubkey-metadata
    xmpp_stanza_t* pubkeymetadata = xmpp_stanza_new(ctx);
    xmpp_stanza_set_name(pubkeymetadata, STANZA_NAME_PUBKEY_METADATA);
    xmpp_stanza_set_attribute(pubkeymetadata, STANZA_ATTR_V4_FINGERPRINT, fingerprint);
    xmpp_stanza_set_attribute(pubkeymetadata, STANZA_ATTR_DATE, _gettimestamp());

    xmpp_stanza_add_child(publickeyslist, pubkeymetadata);
    xmpp_stanza_add_child(item, publickeyslist);
    xmpp_stanza_add_child(publish, item);
    xmpp_stanza_add_child(pubsub, publish);
    xmpp_stanza_add_child(iq, pubsub);
    xmpp_send(connection_get_conn(), iq);
}

static int
_ox_metadata_result(xmpp_conn_t* const conn, xmpp_stanza_t* const stanza, void* const userdata)
{
    log_debug("[OX] Processing result %s's metadata.", (char*)userdata);

    if (g_strcmp0(xmpp_stanza_get_type(stanza), "result") != 0) {
        log_debug("[OX] Error: Unable to load metadata of user %s - Not a stanza result type", (char*)userdata);
        return FALSE;
    }
    // pubsub
    xmpp_stanza_t* pubsub = xmpp_stanza_get_child_by_name_and_ns(stanza, STANZA_NAME_PUBSUB, XMPP_FEATURE_PUBSUB);
    if (!pubsub) {
        cons_show("[OX] Error: No pubsub");
        return FALSE;
    }

    xmpp_stanza_t* items = xmpp_stanza_get_child_by_name(pubsub, STANZA_NAME_ITEMS);
    if (!items) {
        cons_show("[OX] Error: No items");
        return FALSE;
    }

    xmpp_stanza_t* item = xmpp_stanza_get_child_by_name(items, STANZA_NAME_ITEM);
    if (!item) {
        cons_show("[OX] Error: No item");
        return FALSE;
    }

    xmpp_stanza_t* publickeyslist = xmpp_stanza_get_child_by_name_and_ns(item, STANZA_NAME_PUBLIC_KEYS_LIST, STANZA_NS_OPENPGP_0);
    if (!publickeyslist) {
        cons_show("[OX] Error: No publickeyslist");
        return FALSE;
    }

    xmpp_stanza_t* pubkeymetadata = xmpp_stanza_get_children(publickeyslist);

    while (pubkeymetadata) {
        const char* fingerprint = xmpp_stanza_get_attribute(pubkeymetadata, STANZA_ATTR_V4_FINGERPRINT);
        if (strlen(fingerprint) == KEYID_LENGTH) {
            cons_show(fingerprint);
        } else {
            cons_show("OX: Wrong char size of public key");
            log_error("[OX] Wrong chat size of public key %s", fingerprint);
        }
        pubkeymetadata = xmpp_stanza_get_next(pubkeymetadata);
    }

    return FALSE;
}

/*!
 *
 * <pre>

<iq from='romeo@example.org/orchard'
    to='juliet@example.org'
    type='get'
    id='getpub'>
  <pubsub xmlns='http://jabber.org/protocol/pubsub'>
    <items node='urn:xmpp:openpgp:0:public-keys:1234567890ABCDF12349ABCD1293848292983833'
           max_items='1'/>
  </pubsub>
</iq>

 * </pre>
 */

void
_ox_request_public_key(const char* const jid, const char* const fingerprint)
{
    assert(jid);
    assert(fingerprint);
    assert(strlen(fingerprint) == KEYID_LENGTH);
    cons_show("Requesting Public Key %s for %s", fingerprint, jid);
    log_info("[OX] Request %s's public key %s.", jid, fingerprint);
    // iq
    xmpp_ctx_t* const ctx = connection_get_ctx();
    char* id = xmpp_uuid_gen(ctx);
    xmpp_stanza_t* iq = xmpp_iq_new(ctx, STANZA_TYPE_GET, id);
    xmpp_stanza_set_from(iq, xmpp_conn_get_jid(connection_get_conn()));
    xmpp_stanza_set_to(iq, jid);
    // pubsub
    xmpp_stanza_t* pubsub = xmpp_stanza_new(ctx);
    xmpp_stanza_set_name(pubsub, STANZA_NAME_PUBSUB);
    xmpp_stanza_set_ns(pubsub, XMPP_FEATURE_PUBSUB);
    // items
    GString* node_name = g_string_new(STANZA_NS_OPENPGP_0_PUBLIC_KEYS);
    g_string_append(node_name, ":");
    g_string_append(node_name, fingerprint);

    xmpp_stanza_t* items = xmpp_stanza_new(ctx);
    xmpp_stanza_set_name(items, STANZA_NAME_ITEMS);
    xmpp_stanza_set_attribute(items, STANZA_ATTR_NODE, node_name->str);
    xmpp_stanza_set_attribute(items, "max_items", "1");

    xmpp_stanza_add_child(pubsub, items);
    xmpp_stanza_add_child(iq, pubsub);

    xmpp_id_handler_add(connection_get_conn(), _ox_public_key_result, id, NULL);

    xmpp_send(connection_get_conn(), iq);
}

/*!
 *
 * <pre>

<iq from='juliet@example.org'
    to='romeo@example.org/orchard'
    type='result'
    id='getpub'>
  <pubsub xmlns='http://jabber.org/protocol/pubsub'>
    <items node='urn:xmpp:openpgp:0:public-keys:123454678819283823ABCDEF1234566789001234'>
      <item id='2020-01-21T10:46:21Z'>
        <pubkey xmlns='urn:xmpp:openpgp:0'>
          <data>
            BASE64_OPENPGP_PUBLIC_KEY
          </data>
        </pubkey>
      </item>
    </items>
  </pubsub>
</iq>

 * </pre>
 */

int
_ox_public_key_result(xmpp_conn_t* const conn, xmpp_stanza_t* const stanza, void* const userdata)
{
    log_debug("[OX] Processing result public key");

    if (g_strcmp0(xmpp_stanza_get_type(stanza), "result") != 0) {
        cons_show("Public Key import failed. Check log for details.");
        log_error("[OX] Public Key response type is wrong");
        return FALSE;
    }
    // pubsub
    xmpp_stanza_t* pubsub = xmpp_stanza_get_child_by_name_and_ns(stanza, STANZA_NAME_PUBSUB, XMPP_FEATURE_PUBSUB);
    if (!pubsub) {
        cons_show("Public Key import failed. Check log for details.");
        log_error("[OX] Public key request response failed: No <pubsub/>");
        return FALSE;
    }

    xmpp_stanza_t* items = xmpp_stanza_get_child_by_name(pubsub, STANZA_NAME_ITEMS);
    if (!items) {
        cons_show("Public Key import failed. Check log for details.");
        log_error("[OX] Public key request response failed: No <items/>");
        return FALSE;
    }

    xmpp_stanza_t* item = xmpp_stanza_get_child_by_name(items, STANZA_NAME_ITEM);
    if (!item) {
        cons_show("Public Key import failed. Check log for details.");
        log_error("[OX] Public key request response failed: No <item/>");
        return FALSE;
    }

    xmpp_stanza_t* pubkey = xmpp_stanza_get_child_by_name_and_ns(item, STANZA_NAME_PUPKEY, STANZA_NS_OPENPGP_0);
    if (!pubkey) {
        cons_show("Public Key import failed. Check log for details.");
        log_error("[OX] Public key request response failed: No <pubkey/>");
        return FALSE;
    }

    xmpp_stanza_t* data = xmpp_stanza_get_child_by_name(pubkey, STANZA_NAME_DATA);
    if (!data) {
        log_error("[OX] No data");
    }

    char* base64_data = xmpp_stanza_get_text(data);
    if (base64_data) {
        log_debug("Key data: %s", base64_data);

        if (p_ox_gpg_import(base64_data)) {
            cons_show("Public Key imported");
        } else {
            cons_show("Public Key import failed. Check log for details.");
        }

        free(base64_data);
    }

    return FALSE;
}

// Date and Time (XEP-0082)
char*
_gettimestamp()
{
    time_t now = time(NULL);
    struct tm* tm = localtime(&now);
    char buf[255];
    strftime(buf, sizeof(buf), "%FT%T", tm);
    GString* d = g_string_new(buf);
    g_string_append(d, "Z");
    return strdup(d->str);
}

#endif // HAVE_LIBGPGME
