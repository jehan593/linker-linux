/* Linker for Linux — entry point.
 *
 * Every launch is a fresh process (no background service). If argv contains a URL,
 * show the chooser popup; otherwise show the main window. Both share the same
 * on-disk data store; no cross-process IPC.
 */
#include <gtk/gtk.h>
#include <fontconfig/fontconfig.h>
#include <curl/curl.h>
#include <stdio.h>

#include "app_context.h"
#include "app_identity.h"
#include "theme.h"
#include "xdg_default.h"
#include "ui_main_window.h"
#include "ui_chooser_window.h"

typedef struct {
    AppState *state;
    const gchar *url_arg; /* NULL => show MainWindow instead of the chooser */
} LaunchContext;

static void load_bundled_fonts(const gchar *data_dir) {
    if (!data_dir) return;
    const char *files[] = {"martian_mono_regular.ttf", "martian_mono_medium.ttf", "martian_mono_bold.ttf"};
    for (size_t i = 0; i < G_N_ELEMENTS(files); i++) {
        gchar *path = g_build_filename(data_dir, "fonts", files[i], NULL);
        if (g_file_test(path, G_FILE_TEST_EXISTS)) {
            if (!FcConfigAppFontAddFile(FcConfigGetCurrent(), (const FcChar8 *) path)) {
                g_warning("failed to register bundled font: %s", path);
            }
        }
        g_free(path);
    }
}

static void activate(GtkApplication *app, gpointer user_data) {
    LaunchContext *ctx = user_data;
    linker_theme_init();

    /* Idempotent — keeps Linker listed as an eligible browser candidate. */
    xdg_default_ensure_registered(ctx->state->icon_256_path);

    if (ctx->url_arg) {
        ui_chooser_window_new(app, ctx->state, ctx->url_arg);
    } else {
        ui_main_window_show(app, ctx->state);
    }
}

int main(int argc, char **argv) {
    curl_global_init(CURL_GLOBAL_DEFAULT);

    AppState *state = app_state_new();
    if (!state->asset_dir) {
        fprintf(stderr, "warning: could not locate data dir (fonts/icons); falling back to system resources\n");
    }
    load_bundled_fonts(state->asset_dir);

    LaunchContext ctx = {.state = state, .url_arg = NULL};
    if (argc > 1 && g_uri_is_valid(argv[1], G_URI_FLAGS_PARSE_RELAXED, NULL)) {
        ctx.url_arg = argv[1];
    }

    GtkApplication *app = gtk_application_new(LINKER_APP_ID, G_APPLICATION_NON_UNIQUE);
    g_signal_connect(app, "activate", G_CALLBACK(activate), &ctx);

    /* Run with just argv[0]: we already extracted the URL above. */
    int status = g_application_run(G_APPLICATION(app), 1, argv);

    g_object_unref(app);
    app_state_free(state);
    curl_global_cleanup();
    return status;
}
