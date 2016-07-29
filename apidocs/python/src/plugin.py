"""
Profanity plugin Hooks
"""

def prof_init(version, status, account_name, fulljid):
    pass


def prof_on_start():
    pass


def prof_on_shutdown():
    pass


def prof_on_unload():
    pass


def prof_on_connect(account_name, fulljid):
    pass


def prof_on_disconnect(account_name, fulljid):
    pass


def prof_pre_chat_message_display(jid, message):
    pass


def prof_post_chat_message_display(jid, message):
    pass


def prof_pre_chat_message_send(jid, message):
    pass


def prof_post_chat_message_send(jid, message):
    pass


def prof_pre_room_message_display(room, nick, message):
    pass


def prof_post_room_message_display(room, nick, message):
    pass


def prof_pre_room_message_send(room, message):
    pass


def prof_post_room_message_send(room, message):
    pass


def prof_on_room_history_message(room, nick, message, timestamp):
    pass


def prof_pre_priv_message_display(room, nick, message):
    pass


def prof_post_priv_message_display(room, nick, message):
    pass


def prof_pre_priv_message_send(room, nick, message):
    pass


def prof_post_priv_message_send(room, nick, message):
    pass


def prof_on_message_stanza_send(stanza):
    pass


def prof_on_message_stanza_receive(stanza):
    pass


def prof_on_presence_stanza_send(stanza):
    pass


def prof_on_presence_stanza_receive(stanza):
    pass


def prof_on_iq_stanza_send(stanza):
    pass


def prof_on_iq_stanza_receive(stanza):
    pass


def prof_on_contact_offline(barejid, resource, status):
    pass


def prof_on_contact_presence(barejid, resource, presence, status, priority):
    pass


def prof_on_chat_win_focus(barejid):
    pass


def prof_on_room_win_focus(roomjid):
    pass

