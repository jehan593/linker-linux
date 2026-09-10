/* Link chooser popup — shown when argv contains a URL instead of the main window.
 * Every exit path destroys the window, ending the process. */
#ifndef LINKER_UI_CHOOSER_WINDOW_H
#define LINKER_UI_CHOOSER_WINDOW_H

#include <gtk/gtk.h>
#include "app_context.h"

GtkWidget *ui_chooser_window_new(GtkApplication *app, AppState *state, const gchar *url);

#endif
