/* Saved Links tab — the Linux analogue of Ui/SavedLinks/SavedLinksView.axaml. */
#ifndef LINKER_UI_SAVED_LINKS_VIEW_H
#define LINKER_UI_SAVED_LINKS_VIEW_H

#include <gtk/gtk.h>
#include "app_context.h"

GtkWidget *ui_saved_links_view_new(AppState *state, GtkWidget *toast_host);

/* Reloads from state->data and rebuilds the (search-filtered, day-grouped) row list —
 * call after any mutation and when the main window regains focus. */
void ui_saved_links_view_refresh(GtkWidget *view);

/* Schedules a refresh on the next idle iteration — safe to call from within a row's
 * own signal handler. */
void ui_saved_links_view_refresh_deferred(GtkWidget *view);

#endif
