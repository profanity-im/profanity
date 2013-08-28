import prof
import getpass

def prof_init(version, status):
    prof.register_command("/whoami", 0, 0, "/whoami", "Call shell whoami command.", "Call shell whoami command.", cmd_whoami)

def cmd_whoami():
    me = getpass.getuser()
    prof.cons_show(me)
