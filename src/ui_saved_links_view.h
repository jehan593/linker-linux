/* Saved Links tab. */
#ifndef LINKER_UI_SAVED_LINKS_VIEW_H
#define LINKER_UI_SAVED_LINKS_VIEW_H

#include <gtk/gtk.h>
#include "app_context.h"

GtkWidget *ui_saved_links_view_new(AppState *state, GtkWidget *toast_host);

/* Reloads and rebuilds the (search-filtered, day-grouped) row list.
 * Call after any mutation or focus change. */
void ui_saved_links_view_refresh(GtkWidget *view);

/* Schedules a refresh on the next idle — safe from within a row's signal handler. */
void ui_saved_links_view_refresh_deferred(GtkWidget *view);

#endif
