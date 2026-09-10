/* Chooser-window size persistence. Only the chooser popup saves/restores its size;
 * other windows keep fixed defaults. Stored in
 * $XDG_DATA_HOME/linker/settings.json under "windows", flock-guarded. */
#ifndef LINKER_WINDOW_GEOMETRY_H
#define LINKER_WINDOW_GEOMETRY_H

#include <gtk/gtk.h>

/* Restores the last saved size for `key` (if any), tracks live size, and persists
 * on destroy. Call after setting the window's default size. */
void window_geometry_apply(GtkWindow *window, const gchar *key);

#endif