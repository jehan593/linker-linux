/* Shared, explicitly-passed application state — the data store, the bundled asset
 * directory (fonts/icons), and a handle back to the main window (so the chooser
 * popup's "Manage browsers" button can raise it). Passed by pointer through GTK
 * callback user_data rather than kept as hidden globals. */
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

/* Locates the bundled data/ dir whether running from the source tree or an installed
 * prefix ($XDG_DATA_HOME/linker). Returns NULL if neither is found. */
gchar *app_find_asset_dir(void);

AppState *app_state_new(void);
void app_state_free(AppState *state);

/* Saves app->data to disk, showing `error_toast_host` a toast on failure (pass NULL
 * to skip the toast, e.g. from non-UI contexts). */
void app_state_save(AppState *state, GtkWidget *error_toast_host);

#endif
