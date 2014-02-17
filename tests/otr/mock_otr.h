#ifndef MOCK_OTR_H
#define MOCK_OTR_H

#include "config/account.h"

void otr_keygen_expect(ProfAccount *account);

void otr_libotr_version_returns(char *version);

void otr_get_my_fingerprint_returns(char *fingerprint);
void otr_get_their_fingerprint_expect_and_return(char *recipient, char *fingerprint);

#endif
