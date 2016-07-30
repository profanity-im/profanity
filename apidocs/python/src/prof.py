"""
Profanity plugin API
"""


def cons_alert(): 
    """
    Highlights the console window in the status bar.
    """
    pass


def cons_show(message): 
    """Show a message in the console window.

    :param message: the message to print
    :type message: str or unicode

    Example:
    ::
        prof.cons_show("This will appear in the console window")
    """
    pass


def cons_show_themed(group, key, default, message): 
    """Show a message in the console, using the specified theme.\n
    Themes can be must be specified in ``~/.local/share/profanity/plugin_themes``

    :param group: the group name in the themes file
    :param key: the item name within the group
    :param default: default colour if the theme cannot be found
    :param message: the message to print
    :type group: str, unicode or None
    :type key: str, unicode or None
    :type default: str, unicode or None
    :type message: str or unicode

    Example:
    ::
        prof.cons_show_themed("myplugin", "text", None, "Plugin themed message")
    """
    pass


def cons_bad_cmd_usage(command): 
    """Show a message indicating the command has been called incorrectly.

    :param command: the command name with leading slash, e.g. ``"/say"``
    :type command: str or unicode

    Example:
    ::
        prof.cons_bad_cmd_usage("/mycommand")
    """
    pass


def register_command(name, min_args, max_args, synopsis, description, arguments, examples, callback): 
    """Register a new command, with help information, and callback for command execution.\n
    Profanity will do some basic validation when the command is called using the argument range.

    :param name: the command name with leading slash, e.g. ``"/say"``
    :param min_args: minimum number or arguments that the command considers to be a valid call
    :param max_args: maximum number or arguments that the command considers to be a valid call
    :param synopsis: command usages
    :param description: a short description of the command
    :param arguments: argument descriptions, each element should be of the form [arguments, description]
    :param examples: example usages
    :param callback: function to call when the command is executed
    :type name: str or unicode
    :type min_args: int
    :type max_args: int
    :type synopsis: list of str or unicode
    :type description: str or unicode
    :type arguments: list of list of str or unicode
    :type examples: list of str or unicode
    :type callback: function
    
    Example:
    ::
        synopsis = [
            "/newcommand action1|action2",
            "/newcommand print <argument>",
            "/newcommand dosomething [<optional_argument>]"
        ]
        description = "An example for the documentation"
        args = [
            [ "action1|action2", "Perform action1 or action2" ],
            [ "print <argument>", "Print argument" ],
            [ "dosomething [<optional_argument>]", "Do something, optionally with the argument" ]
        ]
        examples = [
            "/newcommand action1",
            "/newcommand print \\"Test debug message\\"",
            "/newcommand dosomething"
        ]        

        prof.register_command("/newcommand", 1, 2, synopsis, description, args, examples, my_function)
    """
    pass


def register_timed(callback, interval): 
    """Register a function that Profanity will call periodically.

    :param callback: the funciton to call
    :param interval: the time between each call to the function, in seconds
    :type callback: function
    :type interval: int

    Example:
    ::
        prof.register_timed(some_function, 30)
    """
    pass


def completer_add(key, items):
    """Add values to be autocompleted by Profanity for a command, or command argument. If the key already exists, Profanity will add the items to the existing autocomplete items for that key.

    :param key: the prefix to trigger autocompletion
    :param items: the items to return on autocompletion
    :type key: str or unicode
    :type items: list of str or unicode

    Examples:
    ::
        prof.completer_add("/mycommand", [ 
            "action1",
            "action2", 
            "dosomething" 
        ])

        prof.completer_add("/mycommand dosomething", [
            "thing1",
            "thing2" 
        ])
    """
    pass


def completer_remove(key, items):
    """Remove values from autocompletion for a command, or command argument.

    :param key: the prefix from which to remove the autocompletion items
    :param items: the items to remove
    :type key: str or unicode
    :type items: list of str or unicode

    Examples:
    ::
        prof.completer_remove("/mycommand", [ 
            "action1",
            "action2"
        ])

        prof.completer_add("/mycommand dosomething", [
            "thing1"
        ])
    """
    pass


def completer_clear(key):
    """Remove all values from autocompletion for a command, or command argument.

    :param key: the prefix from which to clear the autocompletion items
    :param items: the items to remove

    Examples:
    ::
        prof.completer_clear("/mycommand")

        prof.completer_add("/mycommand dosomething")
    """
    pass


def send_line(line): 
    """Send a line of input to Profanity to execute.

    :param line: the line to send
    :type line: str or unicode

    Example:
    ::
        prof.send_line("/who online")
    """
    pass


def notify(message, timeout, category): 
    """Send a desktop notification.

    :param message: the message to display in the notification
    :param timeout: the length of time before the notification disappears in milliseconds
    :param category: the category of the notification, also displayed
    :type message: str or unicode
    :type timeout: int
    :type category: str or unicode

    Example:
    ::
        prof.notify("Example notification for 5 seconds", 5000, "Example plugin")
    """
    pass


def get_current_recipient(): 
    """Retrieve the Jabber ID of the current chat recipient, when in a chat window.

    :return: The Jabber ID of the current chat recipient e.g. ``"buddy@chat.org"``, or ``None`` if not in a chat window.
    :rtype: str
    """
    pass


def get_current_muc(): 
    """Retrieve the Jabber ID of the current room, when in a chat room window.

    :return: The Jabber ID of the current chat room e.g. ``"metalchat@conference.chat.org"``, or ``None`` if not in a chat room window.
    :rtype: str
    """
    pass


def get_current_nick(): 
    """Retrieve the users nickname in a chat room, when in a chat room window.

    :return: The users nickname in the current chat room e.g. ``"eddie"``, or ``None`` if not in a chat room window.
    :rtype: str
    """
    pass


def get_current_occupants(): 
    """Retrieve nicknames of all occupants in a chat room, when in a chat room window.

    :return: Nicknames of all occupants in the current room or an empty list if not in a chat room window.
    :rtype: list of str
    """
    pass


def current_win_is_console():
    """Determine whether or not the Console window is currently focussed.

    :return: ``True`` if the user is currently in the Console window, ``False`` otherwise.
    :rtype: boolean
    """
    pass


def log_debug(message):
    """Write to the Profanity log at level ``DEBUG``.

    :param message: the message to log
    :type message: str or unicode
    """
    pass


def log_info(): 
    """Write to the Profanity log at level ``INFO``.

    :param message: the message to log
    :type message: str or unicode
    """
    pass


def log_warning(): 
    """Write to the Profanity log at level ``WARNING``.

    :param message: the message to log
    :type message: str or unicode
    """
    pass


def log_error(): 
    """Write to the Profanity log at level ``ERROR``.

    :param message: the message to log
    :type message: str or unicode
    """
    pass


def win_exists(tag):
    """Determine whether or not a plugin window currently exists for the tag.

    :param tag: The tag used when creating the plugin window 
    :type tag: str or unicode
    :return: ``True`` if the window exists, ``False`` otherwise.
    :rtype: boolean

    Example:
    ::
        prof.win_exists("My Plugin")
    """
    pass


def win_create(tag, callback): 
    """Create a plugin window.

    :param tag: The tag used to refer to the window 
    :type tag: str or unicode
    :param callback: function to call when the window receives input 
    :type callback: function

    Example:
    ::
        prof.win_create("My Plugin", window_handler)
    """
    pass


def win_focus(tag): 
    """Focus a plugin window.

    :param tag: The tag of the window to focus 
    :type tag: str or unicode

    Example:
    ::
        prof.win_focus("My Plugin")
    """
    pass


def win_show(tag, message): 
    """Show a message in the plugin window.

    :param tag: The tag of the window to focus
    :type tag: str or unicode
    :param message: the message to print
    :type message: str or unicode

    Example:
    ::
        prof.win_show("My Plugin", "This will appear in the plugin window")
    """
    pass


def win_show_themed(tag, group, key, default, message): 
    """Show a message in the plugin window, using the specified theme.\n
    Themes can be must be specified in ``~/.local/share/profanity/plugin_themes``

    :param tag: The tag of the window to focus
    :type tag: str or unicode
    :param group: the group name in the themes file
    :param key: the item name within the group
    :param default: default colour if the theme cannot be found
    :param message: the message to print
    :type group: str, unicode or None
    :type key: str, unicode or None
    :type default: str, unicode or None
    :type message: str or unicode

    Example:
    ::
        prof.win_show_themed("My Plugin", "myplugin", "text", None, "Plugin themed message")
    """
    pass


def send_stanza(stanza):
    """Send an XMPP stanza

    :param stanza: An XMPP stanza
    :type stanza: str or unicode
    :return: ``True`` if the stanza was sent successfully, ``False`` otherwise
    :rtype boolean:

    Example:
    ::
        prof.send_stanza("<iq to='juliet@capulet.lit/balcony' id='s2c1' type='get'><ping xmlns='urn:xmpp:ping'/></iq>")
    """
    pass


def settings_get_boolean(group, key, default):
    """Get a boolean setting\n
    Settings must be specified in ``~/.local/share/profanity/plugin_settings``

    :param group: the group name in the settings file
    :param key: the item name within the group
    :param default: default value if setting not found
    :type group: str or unicode
    :type key: str or unicode
    :type default: boolean

    Example:
    ::
        prof.settings_get_boolean("myplugin", "notify", False)
    """
    pass


def settings_set_boolean(group, key, value):
    """Set a boolean setting\n
    Settings must be specified in ``~/.local/share/profanity/plugin_settings``

    :param group: the group name in the settings file
    :param key: the item name within the group
    :param value: value to set
    :type group: str or unicode
    :type key: str or unicode
    :type value: boolean

    Example:
    ::
        prof.settings_set_boolean("myplugin", "activate", True)
    """
    pass


def settings_get_string(group, key, default):
    """Get a string setting\n
    Settings must be specified in ``~/.local/share/profanity/plugin_settings``

    :param group: the group name in the settings file
    :param key: the item name within the group
    :param default: default value if setting not found
    :type group: str or unicode
    :type key: str or unicode
    :type default: str

    Example:
    ::
        prof.settings_get_string("myplugin", "prefix", "prefix-->")
    """
    pass


def settings_set_string(group, key, value):
    """Set a string setting\n
    Settings must be specified in ``~/.local/share/profanity/plugin_settings``

    :param group: the group name in the settings file
    :param key: the item name within the group
    :param value: value to set
    :type group: str or unicode
    :type key: str or unicode
    :type value: str

    Example:
    ::
        prof.settings_set_string("myplugin", "prefix", "myplugin, ")
    """
    pass


def settings_get_int(group, key, default):
    """Get an integer setting\n
    Settings must be specified in ``~/.local/share/profanity/plugin_settings``

    :param group: the group name in the settings file
    :param key: the item name within the group
    :param default: default value if setting not found
    :type group: str or unicode
    :type key: str or unicode
    :type default: int

    Example:
    ::
        prof.settings_get_int("myplugin", "timeout", 10)
    """
    pass


def settings_set_int(group, key, value):
    """Set an integer setting\n
    Settings must be specified in ``~/.local/share/profanity/plugin_settings``

    :param group: the group name in the settings file
    :param key: the item name within the group
    :param value: value to set
    :type group: str or unicode
    :type key: str or unicode
    :type value: int

    Example:
    ::
        prof.settings_set_int("myplugin", "timeout", 100)
    """
    pass


def incoming_message(barejid, resource, message):
    """Trigger incoming message handling, this plugin will make profanity act as if the message has been received

    :param barejid: Jabber ID of the sender of the message
    :param resource: resource of the sender of the message
    :param message: the message text
    :type barejid: str or unicode
    :type resource: str or unicode
    :type message: str or unicode

    Example:
    ::
        prof.incoming_message("bob@server.org", "laptop", "Hello there")
    """

def disco_add_feature(feature):
    """Add a service discovery feature the list supported by Profanity.\n
    If a session is already connected, a presence update will be sent to allow any client/server caches to update their feature list for Profanity

    :param feature: the service discovery feature to be added
    :type feature: str

    Example:
    ::
        prof.disco_add_feature("urn:xmpp:omemo:0:devicelist+notify")
    """
    pass