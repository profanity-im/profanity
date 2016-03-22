/** @file
C Hooks.
*/

/**
Called when profanity loads the plugin
@param version The version of profanity as a string e.g. "0.5.0"
@param status The build status, either "development" or "release"
*/
void prof_init(const char * const version, const char * const status);

/**
Called when profanity starts, after {@link prof_init}
*/
void prof_on_start(void);

/**
Called when profanity shuts down
*/
void prof_on_shutdown(void);

/**
Called when an account is connected
@param account_name The name of the account
@param fulljid The full JID of the account
*/
void prof_on_connect(const char * const account_name, const char * const fulljid);

/**
Called when an account is disconnected
@param account_name The name of the account
@param fulljid The full JID of the account
*/
void prof_on_disconnect(const char * const account_name, const char * const fulljid);

/**
Called before a regular chat message is displayed
@param jid The JID of the sender
@param message The message received
@return The new message, or NULL if no change made to the message
*/
char* prof_pre_chat_message_display(const char * const jid, const char *message);

/**
Called after a regular chat message is displayed
@param jid The JID of the sender
@param message The message received
*/
void prof_post_chat_message_display(const char * const jid, const char *message);

/**
Called before a regular chat message is sent
@param jid The JID of the recipient
@param message The message to send
@return The new message, or NULL if no change made to the message
*/
char* prof_pre_chat_message_send(const char * const jid, const char *message);

/**
Called after a regular chat message is sent
@param jid The JID of the recipient
@param message The message sent
*/
void prof_post_chat_message_send(const char * const jid, const char *message);

/**
Called before a MUC message is displayed
@param room The JID of the room
@param nick The nickname of the sender
@param message The message received
@return The new message, or NULL if no change made to the message
*/
char* prof_pre_room_message_display(const char * const room, const char * const nick, const char *message);

/**
Called after a MUC message is displayed
@param room The JID of the room
@param nick The nickname of the sender
@param message The message received
*/
void prof_post_room_message_display(const char * const room, const char * const nick, const char *message);

/**
Called before a MUC message is sent
@param room The JID of the room
@param message The message to send
@return The new message, or NULL if no change made to the message
*/
char* prof_pre_room_message_send(const char * const room, const char *message);

/**
Called after a MUC message is sent
@param room The JID of the room
@param message The message sent
*/
void prof_post_room_message_send(const char * const room, const char *message);

/**
Called before a MUC private message is displayed
@param room The JID of the room
@param nick The nickname of the sender
@param message The message received
@return The new message, or NULL if no change made to the message
*/
char* prof_pre_priv_message_display(const char * const room, const char * const nick, const char *message);

/**
Called after a MUC private message is displayed
@param room The JID of the room
@param nick The nickname of the sender
@param message The message received
*/
void prof_post_priv_message_display(const char * const room, const char * const nick, const char *message);

/**
Called before a MUC private message is sent
@param room The JID of the room
@param nick The nickname of the recipient
@param message The message to send
@return The new message, or NULL if no change made to the message
*/
char* prof_pre_priv_message_send(const char * const room, const char * const nick, const char *message);

/**
Called after a MUC private message is sent
@param room The JID of the room
@param nick The nickname of the recipient
@param message The message sent
*/
void prof_post_priv_message_send(const char * const room, const char * const nick, const char *message);
