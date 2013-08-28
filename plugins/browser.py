import prof
import os
import webbrowser
import re

lastlink = {}

def prof_init(version, status):
    prof.register_command("/browser", 0, 1,
        "/browser [url]",
        "View a URL in the browser.",
        "View a URL in the browser, if no argument is supplied, " +
        "the last received URL will be used.",
        cmd_browser)

def prof_on_message_received(jid, message):
    global lastlink
    links = re.findall(r'(https?://\S+)', message)
    if (len(links) > 0):
        lastlink[jid] = links[len(links)-1]

def cmd_browser(url):
    global lastlink
    link = None

    # use arg if supplied
    if (url != None):
        link = url
    else:
        jid = prof.get_current_recipient();

        # check if in chat window
        if (jid != None):

            # check for link from recipient
            if jid in lastlink.keys():
                link = lastlink[jid]
            else:
                prof.cons_show("No links found from " + jid);

        # not in chat window
        else:
            prof.cons_show("You must supply a URL to the /browser command")

    # open the browser if link found
    if (link != None):
        prof.cons_show("Opening " + link + " in browser")
        open_browser(link)

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
