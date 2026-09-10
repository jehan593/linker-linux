/* Shared application state — data store, bundled assets, main window handle.
 * Passed explicitly through GTK callback user_data, not kept as globals. */
#ifndef LINKER_APP_CONTEXT_H
#define LINKER_APP_CONTEXT_H

#include <gtk/gtk.h>
#include "data_store.h"

typedef struct {
    LinkerData *data;
    gchar *asset_dir;    /* bundled data/ dir (fonts/, icons/), resolved once at startup */
    gchar *icon_256_path;
    GtkWidget *main_window; /* NULL until the main window exists */
} AppState;

/* Locates the bundled data/ dir (source tree or installed prefix). */
gchar *app_find_asset_dir(void);

AppState *app_state_new(void);
void app_state_free(AppState *state);

/* Saves data to disk, showing a toast on failure (pass NULL to skip). */
void app_state_save(AppState *state, GtkWidget *error_toast_host);

/* Reloads data from disk. Call after another process may have written to the file. */
void app_state_reload(AppState *state);

#endif
