// FIXME  on windows this might be a different header
#include <dlfcn.h>
#include <stdlib.h>
#include <assert.h>

#include <glib.h>

#include "config/preferences.h"
#include "log.h"
#include "plugins/api.h"
#include "plugins/callbacks.h"
#include "plugins/plugins.h"
#include "plugins/c_plugins.h"
#include "ui/ui.h"

ProfPlugin *
c_plugin_create(const char * const filename)
{
    ProfPlugin *plugin;
    void *handle = NULL;

    // TODO use XDG for path
    GString *path = g_string_new("./plugins/");
    g_string_append(path, filename);

    handle = dlopen (path->str, RTLD_NOW | RTLD_GLOBAL);

    if (!handle) {
        log_warning ("dlopen failed to open `%s', %s", filename, dlerror ());
        g_string_free(path, TRUE);
        return NULL;
    }

    plugin = malloc(sizeof(ProfPlugin));
    plugin->name = g_strdup(filename);
    plugin->lang = LANG_C;
    plugin->module = handle;
    plugin->init_func = c_init_hook;
    plugin->on_start_func = c_on_start_hook;
    plugin->on_connect_func = c_on_connect_hook;
    plugin->on_message_received_func = c_on_message_received_hook;

    g_string_free(path, TRUE);

    return plugin;
}

void
c_init_hook(ProfPlugin *plugin, const char *  const version, const char *  const status)
{
    void *  f = NULL;
    void (*func)(const char * const __version, const char * const __status);

    assert (plugin && plugin->module);

    if (NULL == (f = dlsym (plugin->module, "prof_init"))) {
        log_warning ("warning: %s does not have init function", plugin->name);
        return ;
    }

    func = (void (*)(const char * const, const char * const))f;

    // FIXME maybe we want to make it boolean to see if it succeeded or not?
    func (version, status);
}


void
c_on_start_hook (ProfPlugin *plugin)
{
    void * f = NULL;
    void (*func)(void);
    assert (plugin && plugin->module);

    if (NULL == (f = dlsym (plugin->module, "prof_on_start")))
        return ;

    func = (void (*)(void)) f;
    func ();
}


void
c_on_connect_hook (ProfPlugin *plugin, const char * const account_name, const char * const fulljid)
{
    void * f = NULL;
    void (*func)(const char * const __account_name, const char * const __fulljid);
    assert (plugin && plugin->module);

    if (NULL == (f = dlsym (plugin->module, "prof_on_connect")))
        return ;

    func = (void (*)(const char * const, const char * const)) f;
    func (account_name, fulljid);
}


// FIXME wooow, if the message is cosntant, then how could I possibly fiddle around with it?
void
c_on_message_received_hook(ProfPlugin *plugin, const char * const jid, const char * const message)
{
    void * f = NULL;
    void (*func)(const char * const __jid, const char * const __message);
    assert (plugin && plugin->module);

    if (NULL == (f = dlsym (plugin->module, "prof_on_message_received")))
        return;

    func = (void (*)(const char * const, const char * const)) f;
    func (jid, message);

}

void
c_close_library (ProfPlugin * plugin)
{
    assert (plugin && plugin->module);

    if (dlclose (plugin->module))
        log_warning ("dlclose failed to close `%s' with `%s'", plugin->name, dlerror ());
}
