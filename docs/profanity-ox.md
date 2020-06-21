# Profanity - OpenPGP for XMPP

Implementation of XEP-0373 - OpenPGP for XMPP (OX) in profanity.

## Overview
The current version (2020-05-23) of profanity provides *XEP-0027: Current Jabber
OpenPGP Usage* via the `/pgp` command. This XEP is *Obsolete*. We should
implement *XEP-0373 - OpenPGP for XMPP (OX)* in profanity.

## pgp

```
14:37:52 - Synopsis
14:37:52 - /pgp libver
14:37:52 - /pgp keys
14:37:52 - /pgp contacts
14:37:52 - /pgp setkey <contact> <keyid>
14:37:52 - /pgp start [<contact>]
14:37:52 - /pgp end
14:37:52 - /pgp log on|off|redact
14:37:52 - /pgp char <char>
14:37:52 - 
14:37:52 - Description
14:37:52 - Open PGP commands to manage keys, and perform PGP encryption during chat sessions. See the /account command to set your own PGP key.
14:37:52 - 
14:37:52 - Arguments 
14:37:52 - libver                   : Show which version of the libgpgme library is being used.
14:37:52 - keys                     : List all keys known to the system.
14:37:52 - contacts                 : Show contacts with assigned public keys.
14:37:52 - setkey <contact> <keyid> : Manually associate a contact with a public key.
14:37:52 - start [<contact>]        : Start PGP encrypted chat, current contact will be used if not specified.
14:37:52 - end                      : End PGP encrypted chat with the current recipient.
14:37:52 - log on|off               : Enable or disable plaintext logging of PGP encrypted messages.
14:37:52 - log redact               : Log PGP encrypted messages, but replace the contents with [redacted]. This is the default.
14:37:52 - char <char>              : Set the character to be displayed next to PGP encrypted messages.
```
## OX
We should implement the `/ox` command which can be used for XEP-0373 instead of
XEP-0027. 

```
/ox keys - List all public keys known to the system (gnupg's keyring)
/ox contacts - Shows contacts with an assigned public key.
```

The `keys` command will list all public keys of gnupg's Keyring, independent if
the key is in use for XMPP or not. 

In profanity we are going to implement the key lookup with a XMPP-URI as OpenPGP
User-ID. An OpenPGP public key can only be used, if the owner of the public key
created an User-ID with the XMPP-URI as Name. https://xmpp.org/extensions/xep-0373.html#openpgp-user-ids
It's not required and possible to assign a contact to an public key. 

```
sec   rsa3072 2020-05-01 [SC] [verfällt: 2022-05-01]
      7FA1EB8644BAC07E7F18E7C9F121E6A6F3A0C7A5
uid        [ ultimativ ] Doctor Snuggles <doctor.snuggles@domain.tld>
uid        [ ultimativ ] xmpp:doctor.snuggles@domain.tld
ssb   rsa3072 2020-05-01 [E] [verfällt: 2022-05-01]
```

The `contacts` command will show all contacts of the roster with a public key in
the keyring, if there is a xmpp user-id within the public key.

OX provides the elements: `<signcrypt/>`, `<sign/>` and `<crypt/>`. Profanity
implements signcrypt, only.


## Keys command
The command `keys` is independent of the XEP. Should we move common commands
(e.g. /pgp keys /ox keys) to /openpgp which will will be the function which are
related to gnupg itself.

## Appendix

* https://xmpp.org/extensions/xep-0373.html - 0.4.0 (2018-07-30)


