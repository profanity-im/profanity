import prof

user = "jabber.org"

def prof_on_start():
    global user
    prof.cons_show("")
    prof.cons_show("Enter password for " + user)
    prof.send_line("/connect " + user)
