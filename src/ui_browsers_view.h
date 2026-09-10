/* Manage Browsers tab. */
#ifndef LINKER_UI_BROWSERS_VIEW_H
#define LINKER_UI_BROWSERS_VIEW_H

#include <gtk/gtk.h>
#include "app_context.h"

GtkWidget *ui_browsers_view_new(AppState *state, GtkWidget *toast_host);

/* Force-rescan and rebuild the row list. Call after any mutation or focus change. */
void ui_browsers_view_refresh(GtkWidget *view);

/* Schedules a refresh on the next idle — safe from within a row's signal handler. */
void ui_browsers_view_refresh_deferred(GtkWidget *view);

#endif
