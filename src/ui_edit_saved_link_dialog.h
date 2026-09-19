/* Edit Link dialog. Handles its own persistence and refreshes saved_links_view. */
#ifndef LINKER_UI_EDIT_SAVED_LINK_DIALOG_H
#define LINKER_UI_EDIT_SAVED_LINK_DIALOG_H

#include <gtk/gtk.h>
#include "app_context.h"

void ui_edit_saved_link_dialog_run(GtkWindow *parent, AppState *state,
                                    gint64 link_id, const gchar *current_url, GtkWidget *saved_links_view);

#endif
