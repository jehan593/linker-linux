/* Flat-JSON persistence for browser prefs and saved links. Stored at
 * $XDG_DATA_HOME/linker/linker-data.json, guarded by flock(). */
#ifndef LINKER_DATA_STORE_H
#define LINKER_DATA_STORE_H

#include <glib.h>

typedef struct {
    gchar *id;
    gchar *custom_label;
    gboolean hidden;
    gint order_index;
    gchar *custom_executable_path;
    gchar *custom_icon_path;
    gchar *extra_arguments;
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

/* Loads from disk under flock. Returns empty data if file doesn't exist. */
LinkerData *linker_data_load(void);

/* Serializes and atomically replaces the data file under flock. */
gboolean linker_data_save(const LinkerData *data, GError **error);

BrowserPrefEntity *linker_data_find_browser_pref(const LinkerData *data, const gchar *id);

/* Exact URL match (trimmed). */
SavedLinkEntity *linker_data_find_saved_link_by_url(const LinkerData *data, const gchar *url);
SavedLinkEntity *linker_data_find_saved_link_by_id(const LinkerData *data, gint64 id);

const gchar *linker_data_dir(void);
const gchar *linker_data_file_path(void);

#endif
