#ifndef TESTS_TEST_MUC_H
#define TESTS_TEST_MUC_H

int muc_before_test(void** state);
int muc_after_test(void** state);

void muc_invites_add__updates__invites_list(void** state);
void muc_invites_remove__updates__invites_list(void** state);
void muc_invites_count__returns__0_when_no_invites(void** state);
void muc_invites_count__returns__5_when_five_invites_added(void** state);
void muc_active__is__false_when_not_joined(void** state);
void muc_active__is__true_when_joined(void** state);

#endif
