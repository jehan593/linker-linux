#include "data_store.h"
#include "json_min.h"

#include <errno.h>
#include <fcntl.h>
#include <glib/gstdio.h>
#include <string.h>
#include <sys/file.h>
#include <unistd.h>

BrowserPrefEntity *browser_pref_entity_new(const gchar *id) {
    BrowserPrefEntity *e = g_new0(BrowserPrefEntity, 1);
    e->id = g_strdup(id);
    e->hidden = FALSE;
    e->order_index = 0;
    e->is_custom = FALSE;
    return e;
}

void browser_pref_entity_free(BrowserPrefEntity *entity) {
    if (!entity) return;
    g_free(entity->id);
    g_free(entity->custom_label);
    g_free(entity->custom_executable_path);
    g_free(entity->custom_icon_path);
    g_free(entity->extra_arguments);
    g_free(entity);
}

SavedLinkEntity *saved_link_entity_new(gint64 id, const gchar *url, gint64 saved_at_millis) {
    SavedLinkEntity *e = g_new0(SavedLinkEntity, 1);
    e->id = id;
    e->url = g_strdup(url);
    e->saved_at_millis = saved_at_millis;
    return e;
}

void saved_link_entity_free(SavedLinkEntity *entity) {
    if (!entity) return;
    g_free(entity->url);
    g_free(entity);
}

LinkerData *linker_data_new(void) {
    LinkerData *d = g_new0(LinkerData, 1);
    d->browser_prefs = g_ptr_array_new_with_free_func((GDestroyNotify) browser_pref_entity_free);
    d->saved_links = g_ptr_array_new_with_free_func((GDestroyNotify) saved_link_entity_free);
    d->next_saved_link_id = 1;
    return d;
}

void linker_data_free(LinkerData *data) {
    if (!data) return;
    g_ptr_array_free(data->browser_prefs, TRUE);
    g_ptr_array_free(data->saved_links, TRUE);
    g_free(data);
}

BrowserPrefEntity *linker_data_find_browser_pref(const LinkerData *data, const gchar *id) {
    if (!data || !id) return NULL;
    for (guint i = 0; i < data->browser_prefs->len; i++) {
        BrowserPrefEntity *e = g_ptr_array_index(data->browser_prefs, i);
        if (g_strcmp0(e->id, id) == 0) return e;
    }
    return NULL;
}

SavedLinkEntity *linker_data_find_saved_link_by_url(const LinkerData *data, const gchar *url) {
    if (!data || !url) return NULL;
    gchar *trimmed = g_strdup(url);
    g_strstrip(trimmed);
    SavedLinkEntity *found = NULL;
    for (guint i = 0; i < data->saved_links->len; i++) {
        SavedLinkEntity *e = g_ptr_array_index(data->saved_links, i);
        if (g_strcmp0(e->url, trimmed) == 0) {
            found = e;
            break;
        }
    }
    g_free(trimmed);
    return found;
}

SavedLinkEntity *linker_data_find_saved_link_by_id(const LinkerData *data, gint64 id) {
    if (!data) return NULL;
    for (guint i = 0; i < data->saved_links->len; i++) {
        SavedLinkEntity *e = g_ptr_array_index(data->saved_links, i);
        if (e->id == id) return e;
    }
    return NULL;
}

const gchar *linker_data_dir(void) {
    static gchar *dir = NULL;
    if (!dir) {
        dir = g_build_filename(g_get_user_data_dir(), "linker", NULL);
        g_mkdir_with_parents(dir, 0700);
    }
    return dir;
}

static gchar *data_file_path(void) {
    return g_build_filename(linker_data_dir(), "linker-data.json", NULL);
}

const gchar *linker_data_file_path(void) {
    static gchar *path = NULL;
    if (!path) path = data_file_path();
    return path;
}

static gchar *lock_file_path(void) {
    return g_build_filename(linker_data_dir(), "linker-data.lock", NULL);
}

static int acquire_lock(void) {
    gchar *path = lock_file_path();
    int fd = open(path, O_CREAT | O_RDWR, 0600);
    g_free(path);
    if (fd < 0) {
        g_warning("could not open lock file: %s", g_strerror(errno));
        return -1;
    }
    if (flock(fd, LOCK_EX) != 0) {
        g_warning("could not acquire data store lock: %s", g_strerror(errno));
        close(fd);
        return -1;
    }
    return fd;
}

static void release_lock(int fd) {
    if (fd < 0) return;
    flock(fd, LOCK_UN);
    close(fd);
}

/* ---------- (de)serialization ---------- */

static JsonValue *browser_pref_to_json(const BrowserPrefEntity *e) {
    JsonValue *o = json_new_object();
    json_object_set(o, "id", json_new_string(e->id));
    json_object_set(o, "customLabel", json_new_string(e->custom_label));
    json_object_set(o, "hidden", json_new_bool(e->hidden));
    json_object_set(o, "orderIndex", json_new_number(e->order_index));
    json_object_set(o, "customExecutablePath", json_new_string(e->custom_executable_path));
    json_object_set(o, "customIconPath", json_new_string(e->custom_icon_path));
    json_object_set(o, "extraArguments", json_new_string(e->extra_arguments));
    json_object_set(o, "isCustom", json_new_bool(e->is_custom));
    return o;
}

static BrowserPrefEntity *browser_pref_from_json(const JsonValue *o) {
    const gchar *id = json_get_string(o, "id", NULL);
    if (!id) return NULL;
    BrowserPrefEntity *e = browser_pref_entity_new(id);
    e->custom_label = g_strdup(json_get_string(o, "customLabel", NULL));
    e->hidden = json_get_bool(o, "hidden", FALSE);
    e->order_index = (gint) json_get_int(o, "orderIndex", 0);
    e->custom_executable_path = g_strdup(json_get_string(o, "customExecutablePath", NULL));
    e->custom_icon_path = g_strdup(json_get_string(o, "customIconPath", NULL));
    e->extra_arguments = g_strdup(json_get_string(o, "extraArguments", NULL));
    e->is_custom = json_get_bool(o, "isCustom", FALSE);
    return e;
}

static JsonValue *saved_link_to_json(const SavedLinkEntity *e) {
    JsonValue *o = json_new_object();
    json_object_set(o, "id", json_new_number(e->id));
    json_object_set(o, "url", json_new_string(e->url));
    json_object_set(o, "savedAtMillis", json_new_number(e->saved_at_millis));
    return o;
}

static SavedLinkEntity *saved_link_from_json(const JsonValue *o) {
    const gchar *url = json_get_string(o, "url", NULL);
    if (!url) return NULL;
    return saved_link_entity_new(json_get_int(o, "id", 0), url, json_get_int(o, "savedAtMillis", 0));
}

LinkerData *linker_data_load(void) {
    LinkerData *data = linker_data_new();

    int lock_fd = acquire_lock();
    gchar *path = data_file_path();
    gchar *contents = NULL;
    gsize length = 0;
    GError *error = NULL;
    gboolean ok = g_file_get_contents(path, &contents, &length, &error);
    g_free(path);
    release_lock(lock_fd);

    if (!ok) {
        if (!g_error_matches(error, G_FILE_ERROR, G_FILE_ERROR_NOENT)) {
            g_warning("could not read data store: %s", error->message);
        }
        g_clear_error(&error);
        return data;
    }

    JsonValue *root = json_parse(contents, &error);
    g_free(contents);
    if (!root) {
        g_warning("could not parse data store, starting fresh: %s", error ? error->message : "unknown error");
        g_clear_error(&error);
        return data;
    }

    JsonValue *prefs = json_object_get(root, "browserPrefs");
    if (prefs && prefs->type == JSON_ARRAY) {
        for (guint i = 0; i < prefs->v.arr->len; i++) {
            BrowserPrefEntity *e = browser_pref_from_json(g_ptr_array_index(prefs->v.arr, i));
            if (e) g_ptr_array_add(data->browser_prefs, e);
        }
    }

    JsonValue *links = json_object_get(root, "savedLinks");
    if (links && links->type == JSON_ARRAY) {
        for (guint i = 0; i < links->v.arr->len; i++) {
            SavedLinkEntity *e = saved_link_from_json(g_ptr_array_index(links->v.arr, i));
            if (e) g_ptr_array_add(data->saved_links, e);
        }
    }

    data->next_saved_link_id = json_get_int(root, "nextSavedLinkId", 1);
    json_value_free(root);
    return data;
}

gboolean linker_data_save(const LinkerData *data, GError **error) {
    JsonValue *root = json_new_object();

    JsonValue *prefs = json_new_array();
    for (guint i = 0; i < data->browser_prefs->len; i++) {
        json_array_append(prefs, browser_pref_to_json(g_ptr_array_index(data->browser_prefs, i)));
    }
    json_object_set(root, "browserPrefs", prefs);

    JsonValue *links = json_new_array();
    for (guint i = 0; i < data->saved_links->len; i++) {
        json_array_append(links, saved_link_to_json(g_ptr_array_index(data->saved_links, i)));
    }
    json_object_set(root, "savedLinks", links);
    json_object_set(root, "nextSavedLinkId", json_new_number(data->next_saved_link_id));

    gchar *text = json_to_string(root);
    json_value_free(root);

    int lock_fd = acquire_lock();
    gchar *path = data_file_path();
    gchar *tmp_path = g_strdup_printf("%s.tmp", path);
    gboolean ok = g_file_set_contents(tmp_path, text, -1, error);
    if (ok) {
        if (g_rename(tmp_path, path) != 0) {
            g_set_error(error, G_FILE_ERROR, g_file_error_from_errno(errno), "rename failed: %s", g_strerror(errno));
            ok = FALSE;
        }
    }
    g_free(tmp_path);
    g_free(path);
    g_free(text);
    release_lock(lock_fd);
    return ok;
}
