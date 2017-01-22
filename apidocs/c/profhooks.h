/** @file
C Hooks.
*/

/**
Called when a plugin is loaded, either when profanity is started, or when the /plugins load or /plugins install commands are called
@param version the version of Profanity
@param status the package status of Profanity, "development" or "release"
@param account_name account name of the currently logged in account, or NULL if not logged in
@param fulljid the users full Jabber ID (barejid and resource) if logged in, NULL otherwise
*/
void prof_init(const char * const version, const char * const status, const char *const account_name, const char *const fulljid);

/**
Called when Profanity is started
*/
void prof_on_start(void);

/**
Called when the user quits Profanity
*/
void prof_on_shutdown(void);

/**
Called when a plugin is unloaded with the /plugins unload command
*/
void prof_on_unload(void);

/**
Called when the user connects with an account
@param account_name account name of the account used for logging in
@param fulljid the full Jabber ID (barejid and resource) of the account
*/
void prof_on_connect(const char * const account_name, const char * const fulljid);

/**
Called when the user disconnects an account
@param account_name account name of the account being disconnected
@param fulljid the full Jabber ID (barejid and resource) of the account
*/
void prof_on_disconnect(const char * const account_name, const char * const fulljid);

/**
Called before a chat message is displayed
@param barejid Jabber ID of the message sender
@param resource resource of the message sender
@param message the received message
@return the new message to display, or NULL to preserve the original message 
*/
char* prof_pre_chat_message_display(const char * const barejid, const char *const resource, const char *message);

/**
Called after a chat message is displayed
@param barejid Jabber ID of the message sender
@param resource resource of the message sender
@param message the received message
*/
void prof_post_chat_message_display(const char * const barejid, const char *const resource, const char *message);

/**
Called before a chat message is sent
@param barejid Jabber ID of the message recipient
@param message the message to be sent
@return the modified or original message to send, or NULL to cancel sending of the message
*/
char* prof_pre_chat_message_send(const char * const barejid, const char *message);

/**
Called after a chat message has been sent
@param barejid Jabber ID of the message recipient
@param message the sent message
*/
void prof_post_chat_message_send(const char * const barejid, const char *message);

/**
Called before a chat room message is displayed
@param barejid Jabber ID of the room
@param nick nickname of message sender
@param message the received message
@return the new message to display, or NULL to preserve the original message 
*/
char* prof_pre_room_message_display(const char * const barejid, const char * const nick, const char *message);

/**
Called after a chat room message is displayed
@param barejid Jabber ID of the room
@param nick nickname of the message sender 
@param message the received message
*/
void prof_post_room_message_display(const char * const barejid, const char * const nick, const char *message);

/**
Called before a chat room message is sent
@param barejid Jabber ID of the room
@param message the message to be sent
@return the modified or original message to send, or NULL to cancel sending of the message
*/
char* prof_pre_room_message_send(const char * const barejid, const char *message);

/**
Called after a chat room message has been sent
@param barejid Jabber ID of the room
@param message the sent message
*/
void prof_post_room_message_send(const char * const barejid, const char *message);

/**
Called when the server sends a chat room history message
@param barejid Jabber ID of the room
@param nick nickname of the message sender
@param message the message to be sent
@param timestamp time the message was originally sent to the room, in ISO8601 format
*/
void prof_on_room_history_message(const char * const barejid, const char *const nick, const char *const message, const char *const timestamp);

/**
Called before a private chat room message is displayed
@param barejid Jabber ID of the room
@param nick nickname of message sender
@param message the received message
@return the new message to display, or NULL to preserve the original message 
*/
char* prof_pre_priv_message_display(const char * const barejid, const char * const nick, const char *message);

/**
Called after a private chat room message is displayed
@param barejid Jabber ID of the room
@param nick nickname of the message sender 
@param message the received message
*/
void prof_post_priv_message_display(const char * const barejid, const char * const nick, const char *message);

/**
Called before a private chat room message is sent
@param barejid Jabber ID of the room
@param nick nickname of message recipient
@param message the message to be sent
@return the modified or original message to send, or NULL to cancel sending of the message
*/
char* prof_pre_priv_message_send(const char * const barejid, const char * const nick, const char *message);

/**
Called after a private chat room message has been sent
@param barejid Jabber ID of the room
@param nick nickname of the message recipient
@param message the sent message
*/
void prof_post_priv_message_send(const char * const barejid, const char * const nick, const char *message);

/**
Called before an XMPP message stanza is sent
@param stanza The stanza to send
@return The new stanza to send, or NULL to preserve the original stanza
*/
char* prof_on_message_stanza_send(const char *const stanza);

/**
Called when an XMPP message stanza is received
@param stanza The stanza received
@return 1 if Profanity should continue to process the message stanza, 0 otherwise
*/
int prof_on_message_stanza_receive(const char *const stanza);

/**
Called before an XMPP presence stanza is sent
@param stanza The stanza to send
@return The new stanza to send, or NULL to preserve the original stanza
*/
char* prof_on_presence_stanza_send(const char *const stanza);

/**
Called when an XMPP presence stanza is received
@param stanza The stanza received
@return 1 if Profanity should continue to process the presence stanza, 0 otherwise
*/
int prof_on_presence_stanza_receive(const char *const stanza);

/**
Called before an XMPP iq stanza is sent
@param stanza The stanza to send
@return The new stanza to send, or NULL to preserve the original stanza
*/
char* prof_on_iq_stanza_send(const char *const stanza);

/**
Called when an XMPP iq stanza is received
@param stanza The stanza received
@return 1 if Profanity should continue to process the iq stanza, 0 otherwise
*/
int prof_on_iq_stanza_receive(const char *const stanza);

/**
Called when a contact goes offline
@param barejid Jabber ID of the contact
@param resource the resource being disconnected
@param status the status message received with the offline presence, or NULL
*/
void prof_on_contact_offline(const char *const barejid, const char *const resource, const char *const status);

/**
Called when a presence notification is received from a contact
@param barejid Jabber ID of the contact
@param resource the resource being disconnected
@param presence presence of the contact, one of "chat", "online", "away", "xa" or "dnd"
@param status the status message received with the presence, or NULL
@param priority the priority associated with the resource
*/
void prof_on_contact_presence(const char *const barejid, const char *const resource, const char *const presence, const char *const status, const int priority);

/**
Called when a chat window is focussed
@param barejid Jabber ID of the chat window recipient
*/
void prof_on_chat_win_focus(const char *const barejid);

/**
Called when a chat room window is focussed
@param barejid Jabber ID of the room
*/
void prof_on_room_win_focus(const char *const barejid);
