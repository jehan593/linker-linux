/* Shared fade-in/out toast bubble — the Linux analogue of
 * Ui/Components/ToastService.cs + ToastHost.axaml. Bottom-center over whatever window
 * content it's overlaid on, 0.18s crossfade, visible for 2.2s. */
#ifndef LINKER_TOAST_H
#define LINKER_TOAST_H

#include <gtk/gtk.h>

/* Returns a widget meant to be added to a GtkOverlay via gtk_overlay_add_overlay() —
 * already positioned bottom-center and hidden until the first toast_host_show(). */
GtkWidget *toast_host_new(void);

void toast_host_show(GtkWidget *host, const gchar *message);

#endif
