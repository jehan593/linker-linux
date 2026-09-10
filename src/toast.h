/* Shared fade-in/out toast bubble. Bottom-center, 0.18s crossfade, 2.2s visible. */
#ifndef LINKER_TOAST_H
#define LINKER_TOAST_H

#include <gtk/gtk.h>

/* Returns a widget for use in a GtkOverlay, hidden until toast_host_show(). */
GtkWidget *toast_host_new(void);

void toast_host_show(GtkWidget *host, const gchar *message);

#endif
