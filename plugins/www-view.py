import prof
import os
import webbrowser


# hooks
def prof_init(version, status):
    prof.register_command("/browser", 1, 1, "/browser url", "View a URL in the browser.", "View a URL in the browser", cmd_browser)

# commands
def cmd_browser(url):
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
