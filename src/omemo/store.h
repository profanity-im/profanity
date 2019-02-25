#include <signal/signal_protocol.h>

typedef struct {
   signal_buffer *public;
   signal_buffer *private;
   uint32_t registration_id;
   GHashTable * identity_key_store;
} identity_key_store_t;

GHashTable * session_store_new(void);
GHashTable * pre_key_store_new(void);
GHashTable * signed_pre_key_store_new(void);
void identity_key_store_new(identity_key_store_t *identity_key_store);

int load_session(signal_buffer **record, const signal_protocol_address *address, void *user_data);
int get_sub_device_sessions(signal_int_list **sessions, const char *name, size_t name_len, void *user_data);
int store_session(const signal_protocol_address *address, uint8_t *record, size_t record_len, void *user_data);
int contains_session(const signal_protocol_address *address, void *user_data);
int delete_session(const signal_protocol_address *address, void *user_data);
int delete_all_sessions(const char *name, size_t name_len, void *user_data);
int load_pre_key(signal_buffer **record, uint32_t pre_key_id, void *user_data);
int store_pre_key(uint32_t pre_key_id, uint8_t *record, size_t record_len, void *user_data);
int contains_pre_key(uint32_t pre_key_id, void *user_data);
int remove_pre_key(uint32_t pre_key_id, void *user_data);
int load_signed_pre_key(signal_buffer **record, uint32_t signed_pre_key_id, void *user_data);
int store_signed_pre_key(uint32_t signed_pre_key_id, uint8_t *record, size_t record_len, void *user_data);
int contains_signed_pre_key(uint32_t signed_pre_key_id, void *user_data);
int remove_signed_pre_key(uint32_t signed_pre_key_id, void *user_data);
int get_identity_key_pair(signal_buffer **public_data, signal_buffer **private_data, void *user_data);
int get_local_registration_id(void *user_data, uint32_t *registration_id);
int save_identity(const signal_protocol_address *address, uint8_t *key_data, size_t key_len, void *user_data);
int is_trusted_identity(const signal_protocol_address *address, uint8_t *key_data, size_t key_len, void *user_data);
int store_sender_key(const signal_protocol_sender_key_name *sender_key_name, uint8_t *record, size_t record_len, uint8_t *user_record, size_t user_record_len, void *user_data);
int load_sender_key(signal_buffer **record, signal_buffer **user_record, const signal_protocol_sender_key_name *sender_key_name, void *user_data);
