#include "app_context.h"
#include "toast.h"

gchar *app_find_asset_dir(void) {
    const gchar *candidates_relative_to_exe[] = {"data", "../data", "../share/linker", NULL};

    gchar *exe_path = g_file_read_link("/proc/self/exe", NULL);
    if (exe_path) {
        gchar *exe_dir = g_path_get_dirname(exe_path);
        g_free(exe_path);
        for (int i = 0; candidates_relative_to_exe[i]; i++) {
            gchar *candidate = g_build_filename(exe_dir, candidates_relative_to_exe[i], NULL);
            if (g_file_test(candidate, G_FILE_TEST_IS_DIR)) {
                g_free(exe_dir);
                return candidate;
            }
            g_free(candidate);
        }
        g_free(exe_dir);
    }

    gchar *installed = g_build_filename(g_get_user_data_dir(), "linker", NULL);
    if (g_file_test(installed, G_FILE_TEST_IS_DIR)) return installed;
    g_free(installed);
    return NULL;
}

AppState *app_state_new(void) {
    AppState *state = g_new0(AppState, 1);
    state->data = linker_data_load();
    state->asset_dir = app_find_asset_dir();
    state->icon_256_path = state->asset_dir ? g_build_filename(state->asset_dir, "icons", "linker-256.png", NULL) : NULL;
    state->main_window = NULL;
    return state;
}

void app_state_free(AppState *state) {
    if (!state) return;
    linker_data_free(state->data);
    g_free(state->asset_dir);
    g_free(state->icon_256_path);
    g_free(state);
}

void app_state_save(AppState *state, GtkWidget *error_toast_host) {
    GError *error = NULL;
    if (!linker_data_save(state->data, &error)) {
        g_warning("could not save data store: %s", error ? error->message : "unknown error");
        g_clear_error(&error);
        if (error_toast_host) toast_host_show(error_toast_host, "Couldn't save changes");
    }
}

void app_state_reload(AppState *state) {
    LinkerData *fresh = linker_data_load();
    linker_data_free(state->data);
    state->data = fresh;
}
