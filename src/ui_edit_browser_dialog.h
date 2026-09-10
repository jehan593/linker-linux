/* Add/Edit browser dialog. item == NULL means Add mode. Handles its own persistence
 * and refreshes browsers_view before closing. */
#ifndef LINKER_UI_EDIT_BROWSER_DIALOG_H
#define LINKER_UI_EDIT_BROWSER_DIALOG_H

#include <gtk/gtk.h>
#include "app_context.h"
#include "browsers.h"

void ui_edit_browser_dialog_run(GtkWindow *parent, AppState *state, GtkWidget *toast_host,
                                 const BrowserListItem *item, GtkWidget *browsers_view);

#endif
