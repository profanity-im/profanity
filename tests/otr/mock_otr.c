#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>
#include <stdlib.h>
#include <string.h>
#include <glib.h>

#include "otr/otr.h"
#include "config/account.h"

static void
_mock_otr_keygen(ProfAccount *account)
{
    check_expected(account);
}

static char *
_mock_otr_libotr_version(void)
{
    return (char *)mock();
}

static char *
_mock_otr_get_my_fingerprint(void)
{
    return (char *)mock();
}

static char *
_mock_otr_get_their_fingerprint(const char * const recipient)
{
    check_expected(recipient);
    return (char *)mock();
}

void
otr_keygen_expect(ProfAccount *account)
{
    otr_keygen = _mock_otr_keygen;
    expect_memory(_mock_otr_keygen, account, account, sizeof(ProfAccount));
}

void
otr_libotr_version_returns(char *version)
{
    otr_libotr_version = _mock_otr_libotr_version;
    will_return(_mock_otr_libotr_version, version);
}

void
otr_get_my_fingerprint_returns(char *fingerprint)
{
    otr_get_my_fingerprint = _mock_otr_get_my_fingerprint;
    will_return(_mock_otr_get_my_fingerprint, fingerprint);
}

void
otr_get_their_fingerprint_expect_and_return(char *recipient, char *fingerprint)
{
    otr_get_their_fingerprint = _mock_otr_get_their_fingerprint;
    expect_string(_mock_otr_get_their_fingerprint, recipient, recipient);
    will_return(_mock_otr_get_their_fingerprint, fingerprint);
}
