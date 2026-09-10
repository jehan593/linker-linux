/* Launches the chosen browser. Shell-splits the command, appends the URL, spawns. */
#ifndef LINKER_LAUNCHER_H
#define LINKER_LAUNCHER_H

#include <glib.h>
#include "browsers.h"

/* Shell-splits the effective command and extra_arguments, appends the URL, and
 * spawns asynchronously. Returns FALSE on failure. */
gboolean launcher_open_browser(const BrowserListItem *item, const gchar *url, GError **error);

#endif
