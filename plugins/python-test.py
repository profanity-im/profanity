import prof

def prof_init(version, status):
    prof.cons_show("python-test: init, " + version + ", " + status)
    prof.register_command("/python", 0, 1, "/python", "python-test", "python-test", cmd_python)
    prof.register_timed(timer_test, 10)

def prof_on_start():
    prof.cons_show("python-test: on_start")

def prof_on_connect(account_name, fulljid):
    prof.cons_show("python-test: on_connect, " + account_name + ", " + fulljid)

def prof_on_message_received(jid, message):
    prof.cons_show("python-test: on_message_received, " + jid + ", " + message)
    prof.cons_alert()
    return message + "[PYTHON]"

def prof_on_message_send(jid, message):
    prof.cons_show("python-test: on_message_send, " + jid + ", " + message)
    prof.cons_alert()
    return message + "[PYTHON]"

def cmd_python(msg):
    if msg:
        prof.cons_show("python-test: /python command called, arg = " + msg)
    else:
        prof.cons_show("python-test: /python command called with no arg")
    prof.cons_alert()
    prof.notify("python-test: notify", 2000, "Plugins")
    prof.send_line("/vercheck")
    prof.cons_show("python-test: sent \"/vercheck\" command")

def timer_test():
    prof.cons_show("python-test: timer fired.")
    recipient = prof.get_current_recipient()
    if recipient:
        prof.cons_show("  current recipient = " + recipient)
    prof.cons_alert()

