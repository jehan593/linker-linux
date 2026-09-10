/* Browser enumeration and preference merge. Scans .desktop files for WebBrowser
 * categories and merges with saved preferences. */
#ifndef LINKER_BROWSERS_H
#define LINKER_BROWSERS_H

#include <glib.h>
#include "data_store.h"

typedef struct {
    gchar *id;
    gchar *system_label;
    gchar *display_label;
    gchar *system_exec_cmdline;
    gchar *system_icon;
    gchar *custom_executable_path;
    gchar *custom_icon_path;
    gchar *extra_arguments;
    gboolean hidden;
    gint order_index;
    gboolean is_custom;
} BrowserListItem;

void browser_list_item_free(BrowserListItem *item);
void browsers_free_list(GPtrArray *list);

/* Force-rescan, merge with prefs, append custom browsers, sort by order_index.
 * Also materializes any newly-synthesized order_index values. */
GPtrArray *browsers_get_manage_list(LinkerData *data, guint *materialized_out);

/* Uses a 5-minute TTL cache for a fast chooser path, filtered to non-hidden. */
GPtrArray *browsers_get_visible_list(LinkerData *data);

/* Effective icon field for display: custom override if set, else system_icon. */
const gchar *browser_list_item_icon_field(const BrowserListItem *item);

/* Effective base command: custom override if set, else system_exec_cmdline. */
const gchar *browser_list_item_effective_cmdline(const BrowserListItem *item);

#endif
