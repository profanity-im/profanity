# Profanity
![Build Status](https://github.com/profanity-im/profanity/workflows/CI/badge.svg) [![GitHub contributors](https://img.shields.io/github/contributors/profanity-im/profanity.svg)](https://github.com/profanity-im/profanity/graphs/contributors/) [![Packaging status](https://repology.org/badge/tiny-repos/profanity.svg)](https://repology.org/project/profanity/versions)

Profanity is a console based XMPP client inspired by [Irssi](http://www.irssi.org/).

If you like Profanity consider becoming a sponsor or [donate](https://profanity-im.github.io/donate.html) some money.

![alt tag](https://profanity-im.github.io/images/prof-2.png)

See the [User Guide](https://profanity-im.github.io/userguide.html) for information on installing and using Profanity.

## Project
This project is about freedom, privacy and choice. We want to enable people to chat with one another in a safe way. Thus supporting encryption (OTR, PGP, OMEMO, OX) and being decentralized, meaning everyone can run their own server. We believe [XMPP](https://xmpp.org/) is a great proven protocol with an excellent community serving this purpose well.

Feel free to follow us on [twitter](https://twitter.com/profanityim), join our [mailing list](https://lists.posteo.de/listinfo/profanity) and/or [MUC](xmpp:profanity@rooms.dismail.de?join).

## Installation
Our [user guide](https://profanity-im.github.io/userguide.html) contains an [install](https://profanity-im.github.io/guide/latest/install.html) section and a section for [building](https://profanity-im.github.io/guide/latest/build.html) from source yourself.

## Security Features

### TLS Certificate Verification with SHA256 Fingerprints

Profanity implements robust TLS certificate verification using **SHA256 fingerprints** for enhanced security. This addresses the deprecation of SHA1 and provides stronger protection against man-in-the-middle attacks.

#### Key Features

- **SHA256 Fingerprint Support**: Certificate fingerprints now use SHA256 hashing algorithm instead of deprecated SHA1
- **Certificate Pinning**: Pin specific certificates by their SHA256 fingerprint to prevent MITM attacks
- **Trust On First Use (TOFU)**: Securely establish trust with servers on first connection
- **Manual Trust Management**: Full control over which certificates to trust
- **Backward Compatibility**: Automatically falls back to SHA1 for older libstrophe versions

#### Why SHA256?

SHA256 provides significant security improvements over SHA1:
- **Collision Resistant**: SHA1 has known collision vulnerabilities; SHA256 is cryptographically secure
- **Industry Standard**: SHA256 is the current standard for certificate fingerprinting (NIST SP 800-131A)
- **Future-Proof**: Provides long-term security for certificate verification
- **Compliance**: Meets modern security requirements and best practices

#### TLS Commands

```
/tls cert                    Display current server's certificate with SHA256 fingerprint
/tls cert <fingerprint>      Show details of a trusted certificate by SHA256 fingerprint
/tls trust                   Add current certificate to manually trusted certificates
/tls trusted                 List all manually trusted certificates with SHA256 checksums
/tls revoke <fingerprint>    Remove a certificate from trusted list by SHA256 fingerprint
/tls allow                   Allow connection with current certificate (one-time)
/tls always                  Always allow connections with current certificate
/tls deny                    Abort connection with untrusted certificate
/tls certpath                Show the trusted certificate path
/tls certpath set <path>     Specify filesystem path containing trusted certificates
/tls certpath clear          Clear the trusted certificate path
/tls certpath default        Use default system certificate path
```

#### Usage Example

When connecting to a server with an untrusted certificate, Profanity will display:

```
TLS certificate verification failed: certificate not trusted
Certificate:
  Subject:
    Common name        : xmpp.example.com
  Issuer:
    Common name        : xmpp.example.com
  Fingerprint (SHA256): A1:B2:C3:D4:E5:F6:07:08:09:0A:1B:2C:3D:4E:5F:60:71:82:93:A4:B5:C6:D7:E8:F9:0A:1B:2C:3D:4E:5F:60

Use '/tls allow' to accept this certificate.
Use '/tls always' to accept this certificate permanently.
Use '/tls deny' to reject this certificate.
```

To permanently trust this certificate:
```
/tls always
```

To view all trusted certificates:
```
/tls trusted
```

#### Implementation Details

This feature is implemented across multiple components:

- **Core**: `src/xmpp/connection.c` - Uses `XMPP_CERT_FINGERPRINT_SHA256` from libstrophe
- **Storage**: `src/config/tlscerts.c` - Stores trusted certificates with fingerprints
- **UI**: `src/ui/console.c` - Displays SHA256 fingerprints to users
- **Commands**: `src/command/cmd_funcs.c` - Implements TLS management commands

#### Requirements

- libstrophe >= 0.12.3 (provides SHA256 fingerprint support)
- OpenSSL or GnuTLS (for TLS support)

#### Security Considerations

- **Self-Signed Certificates**: SHA256 fingerprints allow secure use of self-signed certificates
- **Certificate Changes**: Users are alerted when a server's certificate changes
- **Persistent Trust**: Trusted certificates are stored locally for future connections
- **Manual Verification**: Users should verify fingerprints through a trusted channel before trusting

## Donations
We would highly appreciate it if you support us via [GitHub Sponsors](https://github.com/sponsors/jubalh/). Especially if you make feature requests or need help using Profanity.
Sponsoring enables us to spend time on Profanity.

An alternative way to support us would be to ask for our IBAN or use Bitcoin: `bc1qx265eat7hfasqkqmk9qf38delydnrnuvzhzy0x`.

Issues backed by a sponsor will be tagged with the [sponsored](https://github.com/profanity-im/profanity/issues?q=label%3Asponsored+) label.
Feature requests that we consider out of scope, either because of interest or because of time needed to implement them, will be marked with the [unfunded](https://github.com/profanity-im/profanity/issues?q=label%3Aunfunded) label.

Another way to sponsor us or get an issue solved is to comment on an issue that you are willing to sponsor it. 

Tasks from our [sponsors](SPONSORS.md) will be tackled first.

Thank you! <3

## How to contribute
We tried to sum things up on our [helpout](https://profanity-im.github.io/helpout.html) page.
Additionally you can check out our [blog](https://profanity-im.github.io/blog/) where we have articles like:
[How to get a backtrace](https://profanity-im.github.io/blog/post/how-to-get-a-backtrace/) and [Contributing a Patch via GitHub](https://profanity-im.github.io/blog/post/contributing-a-patch-via-github/).
For more technical details check out our [CONTRIBUTING.md](CONTRIBUTING.md) file.

## Getting help
To get help, first read our [User Guide](https://profanity-im.github.io/userguide.html) then check out the [FAQ](https://profanity-im.github.io/faq.html).
If you have are having a problem then first search the [issue tracker](https://github.com/profanity-im/profanity/issues).
If you don't find anything there either come to our [MUC](xmpp:profanity@rooms.dismail.de?join) or create a new issue depending on what your problem is.

## Links

### Website
URL: https://profanity-im.github.io

Repo: https://github.com/profanity-im/profanity-im.github.io

### Blog
URL: https://profanity-im.github.io/blog

Repo: https://github.com/profanity-im/blog

### Mailinglist
Mailing List: https://lists.posteo.de/listinfo/profanity

### Chatroom
MUC: profanity@rooms.dismail.de

### Plugins
Plugins repository: https://github.com/profanity-im/profanity-plugins
