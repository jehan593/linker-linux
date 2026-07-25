/* Add/Edit browser dialog — the Linux analogue of Ui/Browsers/EditBrowserDialog.axaml.
 * item == NULL means Add mode. Handles its own persistence (app_state_save) and
 * refreshes `browsers_view` on success before closing itself; the caller doesn't need
 * to do anything with the return path. */
#ifndef LINKER_UI_EDIT_BROWSER_DIALOG_H
#define LINKER_UI_EDIT_BROWSER_DIALOG_H

#include <gtk/gtk.h>
#include "app_context.h"
#include "browsers.h"

void ui_edit_browser_dialog_run(GtkWindow *parent, AppState *state, GtkWidget *toast_host,
                                 const BrowserListItem *item, GtkWidget *browsers_view);

#endif
