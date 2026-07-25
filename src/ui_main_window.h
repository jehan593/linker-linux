/* Main launcher window — the Linux analogue of MainWindow.axaml: top app bar,
 * dismissable-while-true default-browser banner, Browsers/Saved Links tabs. */
#ifndef LINKER_UI_MAIN_WINDOW_H
#define LINKER_UI_MAIN_WINDOW_H

#include <gtk/gtk.h>
#include "app_context.h"

/* Creates (or, if one already exists in `state`, raises) the main window. */
GtkWidget *ui_main_window_show(GtkApplication *app, AppState *state);

#endif
