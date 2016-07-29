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
    :return: ``True`` on success, ``False`` on failure
    """
    pass

def cons_show_themed(group, item, default, message): 
    """Show a message in the console, using the specified theme.\n
    Themes can be must be specified in ``~/.local/share/profanity/plugin_themes``

    :param group: the group name in the themes file, or None
    :param item: the item name within the group, or None
    :param default: default colour if the theme cannot be found, or None
    :param message: the message to print
    :return: ``True`` on success, ``False`` on failure
    """
    pass


def cons_bad_cmd_usage(command): 
    """Show a message indicating the command has been called incorrectly.

    :param command: the command name with leading slash, e.g. ``"/say"``
    """
    pass


def register_command(): 
	pass

def register_timed(): 
	pass

def register_ac(): 
	pass

def send_line(): 
	pass

def notify(): 
	pass

def get_current_recipient(): 
	pass

def get_current_muc(): 
	pass

def current_win_is_console(): 
	pass

def log_debug(): 
	pass

def log_info(): 
	pass

def log_warning(): 
	pass

def log_error(): 
	pass

def win_exists(): 
	pass

def win_create(): 
	pass

def win_focus(): 
	pass

def win_show(): 
	pass

def win_show_themed(): 
	pass