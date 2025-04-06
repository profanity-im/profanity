# 📚 Profanity
---

## File: `profanity.c`

| Function | Description |
|---------|-------------|
| `prof_run()` | Starts the Profanity main loop, initializing subsystems and handling runtime lifecycle. |
| `prof_set_quit()` | Sets the global quit flag to signal shutdown, returns the previous state. |
| `_connect_default()` | Attempts to connect using the default or provided XMPP account. |
| `_init()` | Initializes key Profanity subsystems including preferences, logging, UI, and plugins. |
| `prof_add_shutdown_routine()` | Registers a function to be executed during Profanity shutdown. |
| `_call_and_free_shutdown_routine()` | Invokes and frees a shutdown routine struct. |
| `prof_shutdown()` | Runs all registered shutdown routines. |
| `_shutdown()` | Executes all shutdown processes including disconnection, logs, UI, etc. |

---

## File: `chatlog.c`

| Function | Description |
|---------|-------------|
| `_chatlog_close()` | Frees chat log hash tables during shutdown. |
| `chatlog_init()` | Initializes personal and group chat logs and hooks into shutdown routine. |
| `chat_log_msg_out()` | Logs outgoing plaintext messages to a contact. |
| `chat_log_otr_msg_out()` | Logs outgoing OTR messages with optional redaction. |
| `chat_log_pgp_msg_out()` | Logs outgoing PGP messages with optional redaction. |
| `chat_log_omemo_msg_out()` | Logs outgoing OMEMO messages with optional redaction. |
| `chat_log_otr_msg_in()` | Logs incoming OTR messages, considering log settings. |
| `chat_log_pgp_msg_in()` | Logs incoming PGP messages, considering log settings. |
| `chat_log_omemo_msg_in()` | Logs incoming OMEMO messages, considering log settings. |
| `chat_log_msg_in()` | Logs incoming plaintext messages. |
| `groupchat_log_msg_out()` | Logs outgoing groupchat messages. |
| `groupchat_log_msg_in()` | Logs incoming groupchat messages. |
| `groupchat_log_omemo_msg_out()` | Logs outgoing OMEMO-encrypted groupchat messages. |
| `groupchat_log_omemo_msg_in()` | Logs incoming OMEMO-encrypted groupchat messages. |

**Internal Functions:**

| Function | Description |
|---------|-------------|
| `_chat_log_chat()` | Internal helper to write one-to-one chat messages to file, handles redaction and formatting. |
| `_groupchat_log_chat()` | Internal helper to write group chat messages to file. |
| `_get_log_filename()` | Generates the log file path based on contact/room name and login. |
| `_create_chatlog()` | Creates and returns a new `dated_chat_log` struct for personal chats. |
| `_create_groupchat_log()` | Creates and returns a new `dated_chat_log` struct for group chats. |
| `_log_roll_needed()` | Checks if the log should be rolled (rotated daily). |
| `_free_chat_log()` | Frees memory for a `dated_chat_log` struct. |
| `_key_equals()` | Compares two hash table keys for equality. |

---

## File: `common.c`

| Function | Description |
|---------|-------------|
| `auto_free_gchar()` / `auto_free_guchar()` / `auto_free_gcharv()` / `auto_free_char()` | Auto-cleanup helpers for memory deallocation. |
| `auto_close_gfd()` / `auto_close_FILE()` | Auto-cleanup helpers for file descriptors and `FILE*`. |
| `load_data_keyfile()` / `load_config_keyfile()` / `load_custom_keyfile()` | Load `GKeyFile` from data or config paths. |
| `save_keyfile()` | Saves a keyfile to disk. |
| `free_keyfile()` | Releases memory for a keyfile. |
| `string_to_verbosity()` | Converts a string to log level (0–3). |
| `create_dir()` | Creates directory and parent paths if needed. |
| `copy_file()` | Copies a file between paths. |
| `str_replace()` | Replaces all occurrences of a substring. |
| `strtoi_range()` | Safely converts a string to integer within range. |
| `utf8_display_len()` | Returns visual width of a UTF-8 string. |
| `release_get_latest()` | Retrieves latest release version from website. |
| `release_is_new()` | Compares current version to latest release. |
| `strip_arg_quotes()` | Removes quotes around a string. |
| `is_notify_enabled()` | Returns whether system supports notifications. |
| `prof_occurrences()` | Finds character positions of matches in string. |
| `is_regular_file()` / `is_dir()` | File/directory existence and type check. |
| `get_file_paths_recursive()` | Recursively finds all file paths. |
| `get_random_string()` | Generates random alphanumeric string. |
| `get_mentions()` | Finds mention positions of nickname in message. |
| `call_external()` | Spawns an external process. |
| `format_call_external_argv()` | Prepares `argv` for external command using format string. |
| `basename_from_url()` | Extracts a filename from a URL. |
| `get_expanded_path()` | Expands `~` and `file://` paths. |
| `unique_filename_from_url()` | Generates a unique filename based on a URL. |
| `prof_get_version()` | Returns the current Profanity version string. |

---

## File: `database.c`

| Function | Description |
|---------|-------------|
| `log_database_init()` | Initializes and, if needed, migrates the SQLite chat log database. |
| `log_database_close()` | Closes and shuts down the SQLite connection. |
| `log_database_add_incoming()` | Inserts an incoming message into the chat DB. |
| `log_database_add_outgoing_chat()` | Inserts an outgoing chat message into the DB. |
| `log_database_add_outgoing_muc()` | Inserts an outgoing MUC (group) message. |
| `log_database_add_outgoing_muc_pm()` | Inserts an outgoing private message in MUC. |
| `log_database_get_limits_info()` | Returns first/last message metadata for contact. |
| `log_database_get_previous_chat()` | Returns history messages for a contact with filtering options. |

**Internal Helpers:**

| Function | Description |
|---------|-------------|
| `_add_to_db()` | Core function to insert a message into the DB with metadata. |
| `_migrate_to_v2()` | Handles schema migration to version 2. |
| `_check_available_space_for_db_migration()` | Validates disk space for safe migration. |
| `_get_message_type_type()` / `_get_message_type_str()` | Maps message type enum <-> string. |
| `_get_message_enc_type()` / `_get_message_enc_str()` | Maps encryption enum <-> string. |

---

