/* Resolves a browser's icon (a themed icon name or absolute path, as found in a
 * .desktop file's Icon= key, or a user-picked file) to a GdkPixbuf — the Linux
 * analogue of Util/IconExtractor.cs, which rasterized Windows PE icon resources. */
#ifndef LINKER_ICON_RESOLVE_H
#define LINKER_ICON_RESOLVE_H

#include <gdk-pixbuf/gdk-pixbuf.h>

/* Resolves a desktop-entry Icon= value: an absolute path is loaded directly, anything
 * else is looked up in the current icon theme. Returns a placeholder (never NULL) on
 * failure to resolve. */
GdkPixbuf *icon_resolve_browser_icon(const gchar *icon_field, gint size);

/* Loads an arbitrary icon/image file (user-picked via a file chooser). Returns NULL
 * (not a placeholder) on failure so callers can show a distinct "couldn't load" toast. */
GdkPixbuf *icon_resolve_from_file(const gchar *path, gint size);

/* A generic broken-icon glyph, drawn with Cairo (a filled circle with a question-mark-
 * style stroke), used when a browser's icon can't be resolved and for custom browsers
 * with no icon override. */
GdkPixbuf *icon_resolve_placeholder(gint size);

#endif
