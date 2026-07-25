/* The link-interceptor popup — the Linux analogue of Interceptor/LinkChooserWindow.axaml.
 * Shown instead of the main window when argv contains a URL. Every exit path (pick a
 * browser, Cancel, jump to Manage Browsers) destroys the window; since the chooser
 * process exists solely to show this popup, that ends the process. */
#ifndef LINKER_UI_CHOOSER_WINDOW_H
#define LINKER_UI_CHOOSER_WINDOW_H

#include <gtk/gtk.h>
#include "app_context.h"

GtkWidget *ui_chooser_window_new(GtkApplication *app, AppState *state, const gchar *url);

#endif
