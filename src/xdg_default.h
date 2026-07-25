/* Default-browser registration/query — the Linux analogue of
 * Util/DefaultBrowserRegistration.cs. Windows registers a StartMenuInternet +
 * RegisteredApplications entry (eligible-but-not-forced) and deep-links into
 * ms-settings:defaultapps for the user to pick by hand, since no third-party API can
 * force the choice. Linux has no such restriction: installing our own .desktop file
 * with the right MimeType makes us an eligible candidate (same "eligible regardless
 * of held" idea), and GIO's GAppInfo can both query *and directly set* the default
 * handler by writing mimeapps.list — no OS settings roundtrip required. */
#ifndef LINKER_XDG_DEFAULT_H
#define LINKER_XDG_DEFAULT_H

#include <glib.h>

/* Installs/updates ~/.local/share/applications/linker.desktop (Exec pointing at the
 * running executable with %u, MimeType for http/https) and best-effort runs
 * update-desktop-database. Idempotent; call on every startup. */
void xdg_default_ensure_registered(const gchar *icon_path);

/* True if Linker is the registered default handler for both http and https. */
gboolean xdg_default_is_default(void);

/* Sets Linker as the default handler for http and https via GIO. Returns FALSE on
 * failure (e.g. no writable mimeapps.list). */
gboolean xdg_default_set_as_default(void);

#endif
