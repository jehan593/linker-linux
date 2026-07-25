#include "launcher.h"
#include "util.h"

gboolean launcher_open_browser(const BrowserListItem *item, const gchar *url, GError **error) {
    const gchar *base_cmdline = browser_list_item_effective_cmdline(item);
    if (!base_cmdline || !*base_cmdline) {
        g_set_error(error, G_SPAWN_ERROR, G_SPAWN_ERROR_NOENT, "no executable configured for this browser");
        return FALSE;
    }

    gint base_argc = 0;
    gchar **base_argv = linker_split_args(base_cmdline, &base_argc);
    if (!base_argv || base_argc == 0) {
        g_set_error(error, G_SPAWN_ERROR, G_SPAWN_ERROR_INVAL, "could not parse executable command");
        g_strfreev(base_argv);
        return FALSE;
    }

    gint extra_argc = 0;
    gchar **extra_argv = linker_split_args(item->extra_arguments, &extra_argc);

    GPtrArray *final_argv = g_ptr_array_new();
    for (gint i = 0; i < base_argc; i++) g_ptr_array_add(final_argv, base_argv[i]);
    for (gint i = 0; i < extra_argc; i++) g_ptr_array_add(final_argv, extra_argv[i]);
    g_ptr_array_add(final_argv, (gpointer) url);
    g_ptr_array_add(final_argv, NULL);

    gboolean ok = g_spawn_async(
        NULL, (gchar **) final_argv->pdata, NULL,
        G_SPAWN_SEARCH_PATH | G_SPAWN_STDOUT_TO_DEV_NULL | G_SPAWN_STDERR_TO_DEV_NULL,
        NULL, NULL, NULL, error);

    g_ptr_array_free(final_argv, TRUE);
    g_strfreev(base_argv);
    g_strfreev(extra_argv);
    return ok;
}
