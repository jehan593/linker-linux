/* Browser enumeration + preference merge — the Linux analogue of
 * Data/Browser/InstalledBrowsersRepository.cs and Data/Repositories/BrowserPrefsRepository.cs.
 * Windows scanned the StartMenuInternet registry convention; Linux scans .desktop
 * files across XDG_DATA_HOME/XDG_DATA_DIRS for Categories=...WebBrowser... entries. */
#ifndef LINKER_BROWSERS_H
#define LINKER_BROWSERS_H

#include <glib.h>
#include "data_store.h"

typedef struct {
    gchar *id;                      /* desktop-id (basename minus .desktop) or "custom:<uuid>" */
    gchar *system_label;
    gchar *display_label;           /* custom_label if set, else system_label ("Custom browser" for unlabeled customs) */
    gchar *system_exec_cmdline;     /* base command, desktop Exec field codes stripped; may be multi-token
                                        (e.g. "flatpak run org.mozilla.firefox") — always shell-split before spawning */
    gchar *system_icon;             /* Icon= value: themed name or absolute path */
    gchar *custom_executable_path;  /* nullable override; same "cmdline" semantics as system_exec_cmdline */
    gchar *custom_icon_path;        /* nullable override: themed name or absolute path */
    gchar *extra_arguments;         /* nullable, shell-split and prepended before the URL */
    gboolean hidden;
    gint order_index;
    gboolean is_custom;
} BrowserListItem;

void browser_list_item_free(BrowserListItem *item);
void browsers_free_list(GPtrArray *list); /* frees a GPtrArray of BrowserListItem* */

/* Force-rescans .desktop files (no cache) and merges with `data`'s prefs, appending
 * custom browsers, sorted by order_index. Used by the Manage Browsers view, which
 * always wants a fresh list. Also materializes (persists) any newly-synthesized
 * order_index values into `data` (mutates in place; caller should save `data` after
 * if it wants that pinned) — mirrors the Windows "lock in order before any single
 * mutation" rule. */
GPtrArray *browsers_get_manage_list(LinkerData *data);

/* Uses a 5-minute on-disk TTL cache of the raw desktop-file scan (not the merge) for
 * a fast chooser-popup path, filtered to non-hidden browsers. Like the manage list,
 * this may materialize newly-synthesized order_index values into `data` in memory. */
GPtrArray *browsers_get_visible_list(LinkerData *data);

/* Effective icon field for display: custom_icon_path if set, else custom_executable_path's
 * own resolvable icon is not applicable on Linux (unlike Windows PE icon extraction) so
 * this simply falls back to system_icon (empty for a custom browser with no icon override,
 * letting the caller show the placeholder). */
const gchar *browser_list_item_icon_field(const BrowserListItem *item);

/* Effective base command to launch (custom_executable_path override, else system_exec_cmdline). */
const gchar *browser_list_item_effective_cmdline(const BrowserListItem *item);

#endif
