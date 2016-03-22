/** @file
C plugin API.
*/

/** \mainpage C Plugins API and Hooks
List of all API function available to plugins: {@link profapi.h}

List of all hooks which plugins may implement: {@link profhooks.h}
 */

/** Type representing a window, used for referencing windows created by the plugin */
typedef char* PROF_WIN_TAG;

/** Type representing a function pointer to a command callback
@param args The arguments passed to the callback
*/
typedef void(*CMD_CB)(char **args);

/** Type representing a function pointer to a timed callback
*/
typedef void(*TIMED_CB)(void);

/** Type representing a function pointer to a window callback
*/
typedef void(*WINDOW_CB)(PROF_WIN_TAG win, char *line);

/**	Highlights the console window in the status bar. */
void prof_cons_alert(void);

/**	
Show a message in the console.
@param message the message to print 
@return 1 on success, 0 on failure
*/
int prof_cons_show(const char * const message);

/**	
Show a message in the console, using the specified theme.
Themes can be must be specified in ~/.local/share/profanity/plugin_themes
@param group The group name in the themes file, or NULL
@param item The item name within the group, or NULL
@param def A default colour if the theme cannot be found, or NULL
@param message the message to print 
@return 1 on success, 0 on failure
*/
int prof_cons_show_themed(const char *const group, const char *const item, const char *const def, const char *const message);

/**	
Show a message in the console when invalid arguments have been passed to a command.
@param cmd The name of the command, including the leading / 
@return 1 on success, 0 on failure
*/
int prof_cons_bad_cmd_usage(const char *const cmd);

/**
Register a plugin command.
@param command_name The name of the command, including the leading /
@param min_args Minimum arguments valid for the command
@param max_args Maximum arguments valid for the command
@param synopsis Overview of the command usage
@param description A text description of the command
@param arguments Description of all arguments
@param examples Example command usages
@param callback The {@link CMD_CB} function to execute when the command is invoked
*/
void prof_register_command(const char *command_name, int min_args, int max_args,
    const char **synopsis, const char *description, const char *arguments[][2], const char **examples,
    CMD_CB callback);

/**
Register a timed callback function
@param callback The {@link TIMED_CB} function to execute
@param interval_seconds The interval for calling the function
*/
void prof_register_timed(TIMED_CB callback, int interval_seconds);

/**
Register an autocompleter.
Note that calling this function more than once will add the items to the autocompleter
@param key The string used to initiate autocompletion
@param items The to autocomplete with
*/
void prof_register_ac(const char *key, char **items);

/**
Send a desktop notification.
@param message The message to show in the notification
@param timeout_ms Time in milliseconds until the notification disappears
@param category Used as a notification category of the system supports it
*/
void prof_notify(const char *message, int timeout_ms, const char *category);

/**
Send input to Profanity, equivalent to if the user had entered it.
@param line The input to send.
*/
void prof_send_line(char *line);

/**
Get the current recipient if in a chat window.
@return The JID of the recipient, or NULL if not in a chat window
*/
char* prof_get_current_recipient(void);

/**
Get the current room if in a MUC window.
@return The JID of the room, or NULL if not in a MUC window
*/
char* prof_get_current_muc(void);

/**
Determines if the current window is the console.
@return 1 if in the console, 0 otherwise
*/
int prof_current_win_is_console(void);

/**
Log a DEBUG message to the profanity log
@param message The message to log
*/
void prof_log_debug(const char *message);

/**
Log a INFO message to the profanity log
@param message The message to log
*/
void prof_log_info(const char *message);

/**
Log a WARNING message to the profanity log
@param message The message to log
*/
void prof_log_warning(const char *message);

/**
Log a ERROR message to the profanity log
@param message The message to log
*/
void prof_log_error(const char *message);

/**
Determine if window exist with PROF_WIN_TAG
@param win The {@link PROF_WIN_TAG} to check
@return 1 if the window exists, 0 otherwise
*/
int prof_win_exists(PROF_WIN_TAG win);

/**
Create a new window
@param win The {@link PROF_WIN_TAG} for the window
@param input_handler The WINDOW_CB function to call when input is received for that window
*/
void prof_win_create(PROF_WIN_TAG win, WINDOW_CB input_handler);

/**
Focus a window
@param win The {@link PROF_WIN_TAG} of the window to focus
@return 1 on success, 0 on failure
*/
int prof_win_focus(PROF_WIN_TAG win);

/**
Show a line in a window
@param win The {@link PROF_WIN_TAG} of the window
@param line The line to print
@return 1 on success, 0 on failure
*/
int prof_win_show(PROF_WIN_TAG win, char *line);

/**	
Show a message in a window, using the specified theme.
Themes can be must be specified in ~/.local/share/profanity/plugin_themes
@param win The {@link PROF_WIN_TAG} to print to
@param group The group name in the themes file, or NULL
@param item The item name within the group, or NULL
@param def A default colour if the theme cannot be found, or NULL
@param message the message to print 
@return 1 on success, 0 on failure
*/
int prof_win_show_themed(PROF_WIN_TAG tag, char *group, char *key, char *def, char *line);
