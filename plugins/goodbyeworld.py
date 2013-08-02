import prof

package_version = None
package_status = None

# hooks

def prof_init(version, status):
    global package_version
    global package_status
    package_version = version
    package_status = status

def prof_on_start():
    goodbyeworld()

# local functions

def goodbyeworld():
    global package_version
    global package_status
    prof.cons_show("Goodbye world! (" + package_version + " - " + package_status + ")")
