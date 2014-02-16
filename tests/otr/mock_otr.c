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

void
mock_otr_libotr_version(void)
{
    otr_libotr_version = _mock_otr_libotr_version;
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
    will_return(_mock_otr_libotr_version, version);
}
