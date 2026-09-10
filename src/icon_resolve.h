/* Resolves a browser's icon (themed name or path) to a GdkPixbuf. */
#ifndef LINKER_ICON_RESOLVE_H
#define LINKER_ICON_RESOLVE_H

#include <gdk-pixbuf/gdk-pixbuf.h>

/* Resolves a desktop-entry Icon= value. Returns a placeholder on failure (never NULL). */
GdkPixbuf *icon_resolve_browser_icon(const gchar *icon_field, gint size);

/* Loads an arbitrary icon file. Returns NULL on failure. */
GdkPixbuf *icon_resolve_from_file(const gchar *path, gint size);

/* A generic placeholder glyph drawn with Cairo (circle + globe). */
GdkPixbuf *icon_resolve_placeholder(gint size);

#endif
