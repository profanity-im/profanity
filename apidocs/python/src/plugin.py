"""
This page describes functions that plugins may implement to be called from Profanity on certain events. All functions are optional.

Examples:
::
    def prof_on_start():
        prof.cons_show("Profanity has started...")

    def prof_pre_room_message_display(room, nick, message):
        prof.cons_show("Manipulating chat room message before display...")
        new_message = message + " (added by plugin)"
        return new_message

    def prof_on_contact_presence(barejid, resource, presence, status, priority):
        notify_message = barejid + " is " + presence
        prof.notify(notify_message, 5, "Presence")
"""

def prof_init(version, status, account_name, fulljid):
    """Called when a plugin is loaded, either when profanity is started, or when the ``/plugins load`` or ``/plugins install`` commands are called

    :param version: the version of Profanity
    :param status: the package status of Profanity, ``"development"`` or ``"release"``
    :param account_name: account name of the currently logged in account, or ``None`` if not logged in
    :param fulljid: the users full Jabber ID (barejid and resource) if logged in, ``None`` otherwise
    :type version: str or unicode
    :type status: str or unicode
    :type account_name: str, unicode or None
    :type fulljid: str, unicode or None
    """
    pass


def prof_on_start():
    """Called when Profanity is started
    """
    pass


def prof_on_shutdown():
    """Called when the user quits Profanity
    """
    pass


def prof_on_unload():
    """Called when a plugin is unloaded with the ``/plugins unload`` command
    """
    pass


def prof_on_connect(account_name, fulljid):
    """Called when the user connects with an account

    :param account_name: account name of the account used for logging in
    :param fulljid: the full Jabber ID (barejid and resource) of the account
    :type account_name: str or unicode
    :type fulljid: str or unicode
    """
    pass


def prof_on_disconnect(account_name, fulljid):
    """Called when the user disconnects an account

    :param account_name: account name of the account being disconnected
    :param fulljid: the full Jabber ID (barejid and resource) of the account
    :type account_name: str or unicode
    :type fulljid: str or unicode
    """
    pass


def prof_pre_chat_message_display(barejid, resource, message):
    """Called before a chat message is displayed

    :param barejid: Jabber ID of the message sender
    :param resource: resource of the message sender
    :param message: the received message
    :type barejid: str or unicode
    :type resource: str or unicode
    :type message: str or unicode
    :return: the new message to display, or ``None`` to preserve the original message 
    :rtype: str or unicode
    """
    pass


def prof_post_chat_message_display(barejid, resource, message):
    """Called after a chat message is displayed

    :param barejid: Jabber ID of the message sender
    :param resource: resource of the message sender
    :param message: the received message
    :type barejid: str or unicode
    :type resource: str or unicode
    :type message: str or unicode
    """
    pass


def prof_pre_chat_message_send(barejid, message):
    """Called before a chat message is sent

    :param barejid: Jabber ID of the message recipient
    :param message: the message to be sent
    :type barejid: str or unicode
    :type message: str or unicode
    :return: the modified or original message to send, or ``None`` to cancel sending of the message
    :rtype: str or unicode
    """
    pass


def prof_post_chat_message_send(barejid, message):
    """Called after a chat message has been sent

    :param barejid: Jabber ID of the message recipient
    :param message: the sent message
    :type barejid: str or unicode
    :type message: str or unicode
    """
    pass


def prof_pre_room_message_display(barejid, nick, message):
    """Called before a chat room message is displayed

    :param barejid: Jabber ID of the room
    :param nick: nickname of message sender
    :param message: the received message
    :type barejid: str or unicode
    :type nick: str or unicode
    :type message: str or unicode
    :return: the new message to display, or ``None`` to preserve the original message 
    :rtype: str or unicode
    """
    pass


def prof_post_room_message_display(barejid, nick, message):
    """Called after a chat room message is displayed

    :param barejid: Jabber ID of the room
    :param nick: nickname of the message sender 
    :param message: the received message
    :type barejid: str or unicode
    :type nick: str or unicode
    :type message: str or unicode
    """
    pass


def prof_pre_room_message_send(barejid, message):
    """Called before a chat room message is sent

    :param barejid: Jabber ID of the room
    :param message: the message to be sent
    :type barejid: str or unicode
    :type message: str or unicode
    :return: the modified or original message to send, or ``None`` to cancel sending of the message
    :rtype: str or unicode
    """
    pass


def prof_post_room_message_send(barejid, message):
    """Called after a chat room message has been sent

    :param barejid: Jabber ID of the room
    :param message: the sent message
    :type barejid: str or unicode
    :type message: str or unicode
    """
    pass


def prof_on_room_history_message(barejid, nick, message, timestamp):
    """Called when the server sends a chat room history message

    :param barejid: Jabber ID of the room
    :param nick: nickname of the message sender
    :param message: the message to be sent
    :param timestamp: time the message was originally sent to the room, in ISO8601 format
    :type barejid: str or unicode
    :type nick: str or unicode
    :type message: str or unicode
    :type timestamp: str or unicode
    """
    pass


def prof_pre_priv_message_display(barejid, nick, message):
    """Called before a private chat room message is displayed

    :param barejid: Jabber ID of the room
    :param nick: nickname of message sender
    :param message: the received message
    :type barejid: str or unicode
    :type nick: str or unicode
    :type message: str or unicode
    :return: the new message to display, or ``None`` to preserve the original message 
    :rtype: str or unicode
    """
    pass


def prof_post_priv_message_display(barejid, nick, message):
    """Called after a private chat room message is displayed

    :param barejid: Jabber ID of the room
    :param nick: nickname of the message sender 
    :param message: the received message
    :type barejid: str or unicode
    :type nick: str or unicode
    :type message: str or unicode
    """
    pass


def prof_pre_priv_message_send(barejid, nick, message):
    """Called before a private chat room message is sent

    :param barejid: Jabber ID of the room
    :param nick: nickname of message recipient
    :param message: the message to be sent
    :type barejid: str or unicode
    :type nick: str or unicode
    :type message: str or unicode
    :return: the modified or original message to send, or ``None`` to cancel sending of the message
    :rtype: str or unicode
    """
    pass


def prof_post_priv_message_send(barejid, nick, message):
    """Called after a private chat room message has been sent

    :param barejid: Jabber ID of the room
    :param nick: nickname of the message recipient
    :param message: the sent message
    :type barejid: str or unicode
    :type nick: str or unicode
    :type message: str or unicode
    """
    pass


def prof_on_message_stanza_send(stanza):
    """Called before an XMPP message stanza is sent

    :param stanza: The stanza to send
    :type stanza: str or unicode
    :return: The new stanza to send, or ``None`` to preserve the original stanza
    :rtype: str or unicode
    """
    pass


def prof_on_message_stanza_receive(stanza):
    """Called when an XMPP message stanza is received

    :param stanza: The stanza received
    :type stanza: str or unicode
    :return: ``True`` if Profanity should continue to process the message stanza, ``False`` otherwise
    :rtype: boolean
    """
    pass


def prof_on_presence_stanza_send(stanza):
    """Called before an XMPP presence stanza is sent

    :param stanza: The stanza to send
    :type stanza: str or unicode
    :return: The new stanza to send, or ``None`` to preserve the original stanza
    :rtype: str or unicode
    """
    pass


def prof_on_presence_stanza_receive(stanza):
    """Called when an XMPP presence stanza is received

    :param stanza: The stanza received
    :type stanza: str or unicode
    :return: ``True`` if Profanity should continue to process the presence stanza, ``False`` otherwise
    :rtype: boolean
    """
    pass


def prof_on_iq_stanza_send(stanza):
    """Called before an XMPP iq stanza is sent

    :param stanza: The stanza to send
    :type stanza: str or unicode
    :return: The new stanza to send, or ``None`` to preserve the original stanza
    :rtype: str or unicode
    """
    pass


def prof_on_iq_stanza_receive(stanza):
    """Called when an XMPP iq stanza is received

    :param stanza: The stanza received
    :type stanza: str or unicode
    :return: ``True`` if Profanity should continue to process the iq stanza, ``False`` otherwise
    :rtype: boolean
    """
    pass


def prof_on_contact_offline(barejid, resource, status):
    """Called when a contact goes offline

    :param barejid: Jabber ID of the contact
    :param resource: the resource being disconnected
    :param status: the status message received with the offline presence, or ``None``
    :type barejid: str or unicode
    :type resource: str or unicode
    :type status: str or unicode
    """
    pass


def prof_on_contact_presence(barejid, resource, presence, status, priority):
    """Called when a presence notification is received from a contact

    :param barejid: Jabber ID of the contact
    :param resource: the resource being disconnected
    :param presence: presence of the contact, one of ``"chat"``, ``"online"``, ``"away"``, ``"xa"`` or ``"dnd"``
    :param status: the status message received with the presence, or ``None``
    :param priority: the priority associated with the resource
    :type barejid: str or unicode
    :type resource: str or unicode
    :type presence: str or unicode
    :type status: str or unicode
    :type priority: int
    """
    pass


def prof_on_chat_win_focus(barejid):
    """Called when a chat window is focussed

    :param barejid: Jabber ID of the chat window recipient
    :type barejid: str or unicode
    """
    pass


def prof_on_room_win_focus(barejid):
    """Called when a chat room window is focussed

    :param barejid: Jabber ID of the room
    :type barejid: str or unicode
    """
    pass
