/* Manage Browsers tab — the Linux analogue of Ui/Browsers/ManageBrowsersView.axaml. */
#ifndef LINKER_UI_BROWSERS_VIEW_H
#define LINKER_UI_BROWSERS_VIEW_H

#include <gtk/gtk.h>
#include "app_context.h"

GtkWidget *ui_browsers_view_new(AppState *state, GtkWidget *toast_host);

/* Force-rescans and rebuilds the row list — call after any mutation, and when the
 * main window regains focus (matches Windows re-querying on Window.Activated). */
void ui_browsers_view_refresh(GtkWidget *view);

/* Schedules a refresh on the next idle iteration — safe to call from within a row's
 * own signal handler, where an immediate refresh would destroy the widget whose
 * handler is still on the call stack. */
void ui_browsers_view_refresh_deferred(GtkWidget *view);

#endif
