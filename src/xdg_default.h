/* Default-browser registration and query. Installs a .desktop entry and uses
 * GIO's GAppInfo to query/set the default handler for http/https. */
#ifndef LINKER_XDG_DEFAULT_H
#define LINKER_XDG_DEFAULT_H

#include <glib.h>

/* Installs/updates ~/.local/share/applications/linker.desktop. Idempotent. */
void xdg_default_ensure_registered(const gchar *icon_path);

/* True if Linker is the default handler for both http and https. */
gboolean xdg_default_is_default(void);

/* Sets Linker as the default handler for http and https. Returns FALSE on failure. */
gboolean xdg_default_set_as_default(void);

#endif
