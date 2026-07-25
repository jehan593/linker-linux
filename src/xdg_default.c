#include "xdg_default.h"
#include "app_identity.h"

#include <gio/gdesktopappinfo.h>
#include <gio/gio.h>

void xdg_default_ensure_registered(const gchar *icon_path) {
    gchar *exe = g_file_read_link("/proc/self/exe", NULL);
    if (!exe) exe = g_strdup("linker");

    gchar *apps_dir = g_build_filename(g_get_user_data_dir(), "applications", NULL);
    g_mkdir_with_parents(apps_dir, 0755);
    gchar *desktop_path = g_build_filename(apps_dir, LINKER_DESKTOP_ID, NULL);

    GString *content = g_string_new(NULL);
    g_string_append(content, "[Desktop Entry]\n");
    g_string_append(content, "Type=Application\n");
    g_string_append(content, "Name=Linker\n");
    g_string_append(content, "Comment=Choose which browser opens a link\n");
    g_string_append_printf(content, "Exec=%s %%u\n", exe);
    g_string_append_printf(content, "Icon=%s\n", (icon_path && *icon_path) ? icon_path : "linker");
    g_string_append(content, "Terminal=false\n");
    g_string_append(content, "Categories=Network;\n");
    g_string_append_printf(content, "MimeType=%s;%s;\n", LINKER_MIME_HTTP, LINKER_MIME_HTTPS);
    g_string_append(content, "NoDisplay=false\n");

    GError *error = NULL;
    if (!g_file_set_contents(desktop_path, content->str, -1, &error)) {
        g_warning("could not write %s: %s", desktop_path, error->message);
        g_clear_error(&error);
    }
    g_string_free(content, TRUE);

    gchar *argv[] = {"update-desktop-database", apps_dir, NULL};
    g_spawn_sync(NULL, argv, NULL,
                 G_SPAWN_SEARCH_PATH | G_SPAWN_STDOUT_TO_DEV_NULL | G_SPAWN_STDERR_TO_DEV_NULL,
                 NULL, NULL, NULL, NULL, NULL, NULL);

    g_free(desktop_path);
    g_free(apps_dir);
    g_free(exe);
}

gboolean xdg_default_is_default(void) {
    GAppInfo *http_app = g_app_info_get_default_for_type(LINKER_MIME_HTTP, FALSE);
    GAppInfo *https_app = g_app_info_get_default_for_type(LINKER_MIME_HTTPS, FALSE);

    gboolean http_ok = http_app && g_strcmp0(g_app_info_get_id(http_app), LINKER_DESKTOP_ID) == 0;
    gboolean https_ok = https_app && g_strcmp0(g_app_info_get_id(https_app), LINKER_DESKTOP_ID) == 0;

    if (http_app) g_object_unref(http_app);
    if (https_app) g_object_unref(https_app);
    return http_ok && https_ok;
}

gboolean xdg_default_set_as_default(void) {
    GDesktopAppInfo *self_info = g_desktop_app_info_new(LINKER_DESKTOP_ID);
    if (!self_info) {
        g_warning("could not load our own desktop entry (%s) to set as default", LINKER_DESKTOP_ID);
        return FALSE;
    }

    GError *error = NULL;
    gboolean ok_http = g_app_info_set_as_default_for_type(G_APP_INFO(self_info), LINKER_MIME_HTTP, &error);
    if (!ok_http) {
        g_warning("failed to set default for %s: %s", LINKER_MIME_HTTP, error ? error->message : "unknown error");
        g_clear_error(&error);
    }
    gboolean ok_https = g_app_info_set_as_default_for_type(G_APP_INFO(self_info), LINKER_MIME_HTTPS, &error);
    if (!ok_https) {
        g_warning("failed to set default for %s: %s", LINKER_MIME_HTTPS, error ? error->message : "unknown error");
        g_clear_error(&error);
    }

    g_object_unref(self_info);
    return ok_http && ok_https;
}
