import prof
import os
import webbrowser
import re

lastlink = None

# hooks
def prof_init(version, status):
    prof.register_command("/browser", 0, 1, "/browser url", "View a URL in the browser.", "View a URL in the browser", cmd_browser)

def prof_on_message(jid, message):
    global lastlink
    links = re.findall(r'(https?://\S+)', message)
    if (len(links) > 0):
        lastlink = links[len(links)-1]

# commands
def cmd_browser(url):
    global lastlink

    link = None
    if (url != None):
        link = url
    elif (lastlink != None):
        link = lastlink

    if (link != None):
        prof.cons_show("Opening " + link + " in browser.")
        open_browser(link)
    else:
        prof.cons_show("No link supplied.")

def open_browser(url):
    savout = os.dup(1)
    saverr = os.dup(2)
    os.close(1)
    os.close(2)
    os.open(os.devnull, os.O_RDWR)
    try:
        webbrowser.open(url, new=2)
    finally:
        os.dup2(savout, 1)
        os.dup2(saverr, 2)
