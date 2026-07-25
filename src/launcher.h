/* Launches the chosen browser — the Linux analogue of Util/BrowserLauncher.cs. */
#ifndef LINKER_LAUNCHER_H
#define LINKER_LAUNCHER_H

#include <glib.h>
#include "browsers.h"

/* Shell-splits the effective command (custom override or system base command) and any
 * extra_arguments, appends `url` as the final argument, and g_spawn_asyncs it.
 * Returns FALSE and sets *error on failure (nothing to launch, bad quoting, or spawn
 * failure) — caller shows a toast, matching "Couldn't open that link". */
gboolean launcher_open_browser(const BrowserListItem *item, const gchar *url, GError **error);

#endif
