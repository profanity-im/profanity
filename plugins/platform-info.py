import prof
import platform

# hooks

def prof_init(version, status):
    prof.register_command("/platform", 0, 0, "/platform", "Output system information.", "Output system information", cmd_platform)

# local functions

def cmd_platform():
    result_summary = platform.platform()
#    result_machine = plaform.machine()
#    result_node = platform,node()
#    result_processor = platform.processor()
#    result_release = platform.release()
#    result_system = platform.system()
#    result_version = platform.version()

#    prof.cons_show(result_machine + " " + result_node + " " + result_processor + " " + result_release + " " + result_release + " " + result_system + " " + result_version)
    prof.cons_show(result_summary)
