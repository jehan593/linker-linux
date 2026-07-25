/* Flat-JSON persistence for browser preferences and saved links — the Linux analogue
 * of the Windows app's JsonDataStore (named-Mutex + atomic write). Stored at
 * $XDG_DATA_HOME/linker/linker-data.json, guarded per-call by flock() on a sibling
 * lock file so two Linker processes launched at once (every chooser invocation is a
 * fresh process) can't interleave writes and corrupt the file. */
#ifndef LINKER_DATA_STORE_H
#define LINKER_DATA_STORE_H

#include <glib.h>

typedef struct {
    gchar *id;                       /* desktop-file basename (no .desktop) or "custom:<uuid>" */
    gchar *custom_label;             /* nullable */
    gboolean hidden;
    gint order_index;
    gchar *custom_executable_path;   /* nullable; for custom browsers this IS the exe path */
    gchar *custom_icon_path;         /* nullable */
    gchar *extra_arguments;          /* nullable, space-separated/quote-escaped */
    gboolean is_custom;
} BrowserPrefEntity;

typedef struct {
    gint64 id;
    gchar *url;
    gint64 saved_at_millis;
} SavedLinkEntity;

typedef struct {
    GPtrArray *browser_prefs; /* of BrowserPrefEntity* */
    GPtrArray *saved_links;   /* of SavedLinkEntity*, insertion order */
    gint64 next_saved_link_id;
} LinkerData;

BrowserPrefEntity *browser_pref_entity_new(const gchar *id);
void browser_pref_entity_free(BrowserPrefEntity *entity);

SavedLinkEntity *saved_link_entity_new(gint64 id, const gchar *url, gint64 saved_at_millis);
void saved_link_entity_free(SavedLinkEntity *entity);

LinkerData *linker_data_new(void);
void linker_data_free(LinkerData *data);

/* Loads from disk under an exclusive lock; returns a fresh empty LinkerData (never
 * NULL) if the file doesn't exist yet or fails to parse (logs a warning in that case). */
LinkerData *linker_data_load(void);

/* Serializes and atomically replaces the data file under an exclusive lock.
 * Returns FALSE and sets *error on failure. */
gboolean linker_data_save(const LinkerData *data, GError **error);

BrowserPrefEntity *linker_data_find_browser_pref(const LinkerData *data, const gchar *id);

/* Exact (trimmed) URL match, matching the Windows dedupe-on-save behavior. */
SavedLinkEntity *linker_data_find_saved_link_by_url(const LinkerData *data, const gchar *url);
SavedLinkEntity *linker_data_find_saved_link_by_id(const LinkerData *data, gint64 id);

const gchar *linker_data_dir(void); /* $XDG_DATA_HOME/linker, created if missing */

#endif
