#ifndef TESTS_TEST_JID_H
#define TESTS_TEST_JID_H

void jid_create__returns__null_from_null(void** state);
void jid_create__returns__null_from_empty_string(void** state);
void jid_create__returns__full_from_full(void** state);
void jid_create__returns__bare_from_full(void** state);
void jid_create__returns__resourcepart_from_full(void** state);
void jid_create__returns__localpart_from_full(void** state);
void jid_create__returns__domainpart_from_full(void** state);
void jid_create__returns__full_from_full_nolocal(void** state);
void jid_create__returns__bare_from_full_nolocal(void** state);
void jid_create__returns__resourcepart_from_full_nolocal(void** state);
void jid_create__returns__domainpart_from_full_nolocal(void** state);
void jid_create__returns__null_localpart_from_full_nolocal(void** state);
void jid_create__returns__null_full_from_bare(void** state);
void jid_create__returns__null_resource_from_bare(void** state);
void jid_create__returns__bare_from_bare(void** state);
void jid_create__returns__localpart_from_bare(void** state);
void jid_create__returns__domainpart_from_bare(void** state);
void jid_create_from_bare_and_resource__returns__room(void** state);
void jid_create_from_bare_and_resource__returns__nick(void** state);
void jid_create__returns__correct_parts_with_slash_in_resource(void** state);
void jid_create__returns__correct_parts_with_at_in_resource(void** state);
void jid_create__returns__correct_parts_with_at_and_slash_in_resource(void** state);
void jid_create__returns__correct_parts_with_trailing_slash(void** state);
void jid_fulljid_or_barejid__returns__fulljid_when_exists(void** state);
void jid_fulljid_or_barejid__returns__barejid_when_fulljid_not_exists(void** state);

#endif
