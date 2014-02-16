#ifndef MOCK_OTR_H
#define MOCK_OTR_H

#include "config/account.h"

void mock_otr_keygen(void);
void otr_keygen_expect(ProfAccount *account);

void mock_otr_libotr_version(void);
void otr_libotr_version_returns(char *version);

#endif
