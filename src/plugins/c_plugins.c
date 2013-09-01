// FIXME  on windows this might be a different header
#include <dlfcn.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>

#include <glib.h>

#include "config/preferences.h"
#include "log.h"
#include "plugins/api.h"
#include "plugins/callbacks.h"
#include "plugins/plugins.h"
#include "plugins/c_plugins.h"
#include "plugins/c_api.h"
#include "ui/ui.h"

void
c_env_init(void)
{
    c_api_init();
}

ProfPlugin *
c_plugin_create(const char * const filename)
{
    ProfPlugin *plugin;
    void *handle = NULL;

    gchar *plugins_dir = plugins_get_dir();
    GString *path = g_string_new(plugins_dir);
    g_free(plugins_dir);
    g_string_append(path, "/");
    g_string_append(path, filename);

    handle = dlopen (path->str, RTLD_NOW | RTLD_GLOBAL);

    if (!handle) {
        log_warning ("dlopen failed to open `%s', %s", filename, dlerror ());
        g_string_free(path, TRUE);
        return NULL;
    }

    gchar *module_name = g_strndup(filename, strlen(filename) - 3);

    plugin = malloc(sizeof(ProfPlugin));
    plugin->name = strdup(module_name);
    plugin->lang = LANG_C;
    plugin->module = handle;
    plugin->init_func = c_init_hook;
    plugin->on_start_func = c_on_start_hook;
    plugin->on_connect_func = c_on_connect_hook;
    plugin->on_message_received_func = c_on_message_received_hook;
    plugin->on_message_send_func = c_on_message_send_hook;

    g_string_free(path, TRUE);
    g_free(module_name);

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


char *
c_on_message_received_hook(ProfPlugin *plugin, const char * const jid, const char *message)
{
    void * f = NULL;
    char* (*func)(const char * const __jid, const char * __message);
    assert (plugin && plugin->module);

    if (NULL == (f = dlsym (plugin->module, "prof_on_message_received")))
        return NULL;

    func = (char* (*)(const char * const, const char *)) f;
    return func (jid, message);

}

char *
c_on_message_send_hook(ProfPlugin *plugin, const char * const jid, const char *message)
{
    void * f = NULL;
    char* (*func)(const char * const __jid, const char * __message);
    assert (plugin && plugin->module);

    if (NULL == (f = dlsym (plugin->module, "prof_on_message_send")))
        return NULL;

    func = (char* (*)(const char * const, const char *)) f;
    return func (jid, message);

}

void
c_plugin_destroy(ProfPlugin *plugin)
{
    assert (plugin && plugin->module);

    if (dlclose (plugin->module)) {
        log_warning ("dlclose failed to close `%s' with `%s'", plugin->name, dlerror ());
    }

    free(plugin->name);
    free(plugin);
}

void
c_shutdown(void)
{

}
