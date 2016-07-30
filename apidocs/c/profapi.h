/** @file
C plugin API.
*/

/** \mainpage C Plugins API and Hooks
List of all API function available to plugins: {@link profapi.h}

List of all hooks which plugins may implement: {@link profhooks.h}
*/

/** Type representing a window, used for referencing windows created by the plugin */
typedef char* PROF_WIN_TAG;

/** Type representing a function pointer to a command callback */
typedef void(*CMD_CB)(char **args);

/** Type representing a function pointer to a timed callback */
typedef void(*TIMED_CB)(void);

/** Type representing a function pointer to a window callback */
typedef void(*WINDOW_CB)(PROF_WIN_TAG win, char *line);

/**	Highlights the console window in the status bar. */
void prof_cons_alert(void);

/**	
Show a message in the console window.
@param message the message to print
@return 1 on success, 0 on failure
*/
int prof_cons_show(const char * const message);

/**	
Show a message in the console, using the specified theme.
Themes can be must be specified in ~/.local/share/profanity/plugin_themes
@param group the group name in the themes file
@param item the item name within the group
@param def default colour if the theme cannot be found
@param message the message to print 
@return 1 on success, 0 on failure
*/
int prof_cons_show_themed(const char *const group, const char *const item, const char *const def, const char *const message);

/**	
Show a message indicating the command has been called incorrectly.
@param cmd the command name with leading slash, e.g. "/say" 
@return 1 on success, 0 on failure
*/
int prof_cons_bad_cmd_usage(const char *const cmd);

/**
Register a new command, with help information, and callback for command execution.
Profanity will do some basic validation when the command is called using the argument range.
@param command_name the command name with leading slash, e.g. "/say"
@param min_args minimum number or arguments that the command considers to be a valid call
@param max_args maximum number or arguments that the command considers to be a valid call
@param synopsis command usages
@param description a short description of the command
@param arguments argument descriptions
@param examples example usages
@param callback The {@link CMD_CB} function to execute when the command is invoked
*/
void prof_register_command(const char *command_name, int min_args, int max_args,
    char **synopsis, const char *description, char *arguments[][2], char **examples,
    CMD_CB callback);

/**
Register a function that Profanity will call periodically.
@param callback The {@link TIMED_CB} function to execute
@param interval_seconds the time between each call to the function, in seconds
*/
void prof_register_timed(TIMED_CB callback, int interval_seconds);

/**
Add values to be autocompleted by Profanity for a command, or command argument. If the key already exists, Profanity will add the items to the existing autocomplete items for that key.
@param key the prefix to trigger autocompletion
@param items the items to return on autocompletion
*/
void prof_completer_add(const char *key, char **items);

/**
Remove values from autocompletion for a command, or command argument.

@param key the prefix from which to remove the autocompletion items
@param items the items to remove
*/
void prof_completer_remove(const char *key, char **items);

/**
Remove all values from autocompletion for a command, or command argument.

@param key the prefix from which to clear the autocompletion items
@param items the items to remove
*/
void prof_completer_clear(const char *key);

/**
Send a desktop notification.
@param message the message to display in the notification
@param timeout_ms the length of time before the notification disappears in milliseconds
@param category the category of the notification, also displayed
*/
void prof_notify(const char *message, int timeout_ms, const char *category);

/**
Send a line of input to Profanity to execute.
@param line the line to send
*/
void prof_send_line(char *line);

/**
Retrieve the Jabber ID of the current chat recipient, when in a chat window.
@return the Jabber ID of the current chat recipient e.g. "buddy@chat.org", or NULL if not in a chat window.
*/
char* prof_get_current_recipient(void);

/**
Retrieve the Jabber ID of the current room, when in a chat room window.
@return the Jabber ID of the current chat room e.g. "metalchat@conference.chat.org", or NULL if not in a chat room window.
*/
char* prof_get_current_muc(void);

/**
Determine whether or not the Console window is currently focussed.
@return 1 if the user is currently in the Console window, 0 otherwise.
*/
int prof_current_win_is_console(void);

/**
Retrieve the users nickname in a chat room, when in a chat room window.
@return the users nickname in the current chat room e.g. "eddie", or NULLL if not in a chat room window.
*/
char* prof_get_current_nick(void);

/**
Retrieve nicknames of all occupants in a chat room, when in a chat room window.
@return nicknames of all occupants in the current room or an empty list if not in a chat room window.
*/
char** prof_get_current_occupants(void);

/**
Write to the Profanity log at level DEBUG.
@param message The message to log
*/
void prof_log_debug(const char *message);

/**
Write to the Profanity log at level INFO.
@param message The message to log
*/
void prof_log_info(const char *message);

/**
Write to the Profanity log at level WARNING.
@param message The message to log
*/
void prof_log_warning(const char *message);

/**
Write to the Profanity log at level ERROR.
@param message The message to log
*/
void prof_log_error(const char *message);

/**
Create a plugin window.
@param win The {@link PROF_WIN_TAG} used to refer to the window
@param input_handler The WINDOW_CB function to call when the window receives input
*/
void prof_win_create(PROF_WIN_TAG win, WINDOW_CB input_handler);

/**
Determine whether or not a plugin window currently exists for {@link PROF_WIN_TAG}.
@param win the {@link PROF_WIN_TAG} used when creating the plugin window
@return 1 if the window exists, 0 otherwise
*/
int prof_win_exists(PROF_WIN_TAG win);

/**
Focus plugin window.
@param win the {@link PROF_WIN_TAG} of the window to focus
@return 1 on success, 0 on failure
*/
int prof_win_focus(PROF_WIN_TAG win);

/**
Show a message in the plugin window.
@param win the {@link PROF_WIN_TAG} of the window to display the message
@param line The message to print
@return 1 on success, 0 on failure
*/
int prof_win_show(PROF_WIN_TAG win, char *line);

/**	
Show a message in the plugin window, using the specified theme.
Themes must be specified in ~/.local/share/profanity/plugin_themes
@param win The {@link PROF_WIN_TAG} of the window to display the message
@param group the group name in the themes file
@param key the item name within the group
@param def default colour if the theme cannot be found or NULL
@param message the message to print 
@return 1 on success, 0 on failure
*/
int prof_win_show_themed(PROF_WIN_TAG tag, char *group, char *key, char *def, char *line);

/**
Send an XMPP stanza
@param stanza an XMPP stanza
@return 1 if the stanza was sent successfully, 0 otherwise
*/
int prof_send_stanza(char *stanza);

/**
Get a boolean setting
Settings must be specified in ~/.local/share/profanity/plugin_settings
@param group the group name in the settings file
@param key the item name within the group
@param def default value if setting not found
@return the setting, or default value
*/
int prof_settings_get_boolean(char *group, char *key, int def);

/**
Set a boolean setting
Settings must be specified in ~/.local/share/profanity/plugin_settings
@param group the group name in the settings file
@param key the item name within the group
@param value value to set
*/
void prof_settings_set_boolean(char *group, char *key, int value);

/**
Get a string setting
Settings must be specified in ~/.local/share/profanity/plugin_settings
@param group the group name in the settings file
@param key the item name within the group
@param def default value if setting not found
@return the setting, or default value
*/
char* prof_settings_get_string(char *group, char *key, char *def);

/**
Set a string setting
Settings must be specified in ~/.local/share/profanity/plugin_settings
@param group the group name in the settings file
@param key the item name within the group
@param value value to set
*/
void prof_settings_set_string(char *group, char *key, char *value);

/**
Get an integer setting
Settings must be specified in ~/.local/share/profanity/plugin_settings
@param group the group name in the settings file
@param key the item name within the group
@param def default value if setting not found
@return the setting, or default value
*/
int prof_settings_get_int(char *group, char *key, int def);

/**
Set an integer setting
Settings must be specified in ~/.local/share/profanity/plugin_settings
@param group the group name in the settings file
@param key the item name within the group
@param value value to set
*/
void prof_settings_set_int(char *group, char *key, int value);

/**
Trigger incoming message handling, this plugin will make profanity act as if the message has been received
@param barejid Jabber ID of the sender of the message
@param resource resource of the sender of the message
@param message the message text
*/
void prof_incoming_message(char *barejid, char *resource, char *message);

/**
Add a service discovery feature the list supported by Profanity.
If a session is already connected, a presence update will be sent to allow any client/server caches to update their feature list for Profanity
@param feature the service discovery feature to be added
*/
void prof_disco_add_feature(char *feature);
