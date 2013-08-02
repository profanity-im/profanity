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
    helloworld()

def prof_on_connect():
    helloworld();

# local functions

def helloworld():
    global package_version
    global package_status
    prof.cons_show("Hello world! (" + package_version + " - " + package_status + ")")
