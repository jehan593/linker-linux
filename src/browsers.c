#include "browsers.h"
#include "app_identity.h"
#include "json_min.h"
#include "util.h"

#include <string.h>
#include <time.h>

#define CACHE_TTL_SECONDS (5 * 60)

static void browser_list_item_init_defaults(BrowserListItem *item) {
    item->hidden = FALSE;
    item->order_index = 0;
    item->is_custom = FALSE;
}

void browser_list_item_free(BrowserListItem *item) {
    if (!item) return;
    g_free(item->id);
    g_free(item->system_label);
    g_free(item->display_label);
    g_free(item->system_exec_cmdline);
    g_free(item->system_icon);
    g_free(item->custom_executable_path);
    g_free(item->custom_icon_path);
    g_free(item->extra_arguments);
    g_free(item);
}

void browsers_free_list(GPtrArray *list) {
    if (!list) return;
    g_ptr_array_free(list, TRUE);
}

const gchar *browser_list_item_icon_field(const BrowserListItem *item) {
    if (item->custom_icon_path && *item->custom_icon_path) return item->custom_icon_path;
    return item->system_icon;
}

const gchar *browser_list_item_effective_cmdline(const BrowserListItem *item) {
    if (item->custom_executable_path && *item->custom_executable_path) return item->custom_executable_path;
    return item->system_exec_cmdline;
}

/* Strips freedesktop Exec= field codes (%f %F %u %U %i %c %k, %% -> %) leaving the
 * base command (possibly still multi-token, e.g. "flatpak run org.mozilla.firefox"). */
static gchar *strip_exec_field_codes(const gchar *exec) {
    if (!exec) return g_strdup("");
    GString *out = g_string_new(NULL);
    for (const gchar *p = exec; *p; p++) {
        if (*p == '%' && p[1] != '\0') {
            gchar code = p[1];
            if (code == '%') {
                g_string_append_c(out, '%');
                p++;
                continue;
            }
            if (strchr("fFuUick", code)) {
                p++; /* skip the field code; the leading/trailing space around it collapses naturally */
                continue;
            }
            g_string_append_c(out, *p);
        } else {
            g_string_append_c(out, *p);
        }
    }
    gchar *result = g_strdup(g_strstrip(out->str));
    g_string_free(out, TRUE);
    return result;
}

static BrowserListItem *parse_desktop_browser(const gchar *path, const gchar *id) {
    GKeyFile *kf = g_key_file_new();
    GError *error = NULL;
    if (!g_key_file_load_from_file(kf, path, G_KEY_FILE_NONE, &error)) {
        g_clear_error(&error);
        g_key_file_free(kf);
        return NULL;
    }

    const gchar *group = "Desktop Entry";
    gchar *type = g_key_file_get_string(kf, group, "Type", NULL);
    gboolean is_app = !type || g_strcmp0(type, "Application") == 0;
    g_free(type);

    gboolean no_display = g_key_file_get_boolean(kf, group, "NoDisplay", NULL);

    gchar *categories = g_key_file_get_string(kf, group, "Categories", NULL);
    gboolean is_browser = FALSE;
    if (categories) {
        gchar **parts = g_strsplit(categories, ";", -1);
        for (int i = 0; parts[i]; i++) {
            if (g_strcmp0(parts[i], "WebBrowser") == 0) {
                is_browser = TRUE;
                break;
            }
        }
        g_strfreev(parts);
        g_free(categories);
    }

    if (!is_app || no_display || !is_browser) {
        g_key_file_free(kf);
        return NULL;
    }

    gchar *name = g_key_file_get_locale_string(kf, group, "Name", NULL, NULL);
    gchar *exec = g_key_file_get_string(kf, group, "Exec", NULL);
    gchar *icon = g_key_file_get_string(kf, group, "Icon", NULL);
    g_key_file_free(kf);

    if (!name || !exec) {
        g_free(name);
        g_free(exec);
        g_free(icon);
        return NULL;
    }

    BrowserListItem *item = g_new0(BrowserListItem, 1);
    browser_list_item_init_defaults(item);
    item->id = g_strdup(id);
    item->system_label = name;
    item->display_label = g_strdup(name);
    item->system_exec_cmdline = strip_exec_field_codes(exec);
    item->system_icon = icon ? icon : g_strdup("");
    g_free(exec);
    return item;
}

static void scan_dir_into(const gchar *dir_path, GHashTable *seen_ids, GPtrArray *out) {
    if (!g_file_test(dir_path, G_FILE_TEST_IS_DIR)) return;
    GDir *dir = g_dir_open(dir_path, 0, NULL);
    if (!dir) return;

    const gchar *entry;
    while ((entry = g_dir_read_name(dir))) {
        gchar *full = g_build_filename(dir_path, entry, NULL);
        if (g_file_test(full, G_FILE_TEST_IS_DIR)) {
            /* one level of vendor subdirectories (e.g. applications/kde4/) */
            GDir *sub = g_dir_open(full, 0, NULL);
            if (sub) {
                const gchar *sub_entry;
                while ((sub_entry = g_dir_read_name(sub))) {
                    if (!g_str_has_suffix(sub_entry, ".desktop")) continue;
                    gchar *sub_full = g_build_filename(full, sub_entry, NULL);
                    gchar *id = g_strndup(sub_entry, strlen(sub_entry) - strlen(".desktop"));
                    if (!g_hash_table_contains(seen_ids, id) && g_strcmp0(id, "linker") != 0) {
                        BrowserListItem *item = parse_desktop_browser(sub_full, id);
                        g_hash_table_add(seen_ids, g_strdup(id));
                        if (item) g_ptr_array_add(out, item);
                    }
                    g_free(id);
                    g_free(sub_full);
                }
                g_dir_close(sub);
            }
        } else if (g_str_has_suffix(entry, ".desktop")) {
            gchar *id = g_strndup(entry, strlen(entry) - strlen(".desktop"));
            if (!g_hash_table_contains(seen_ids, id) && g_strcmp0(id, "linker") != 0) {
                BrowserListItem *item = parse_desktop_browser(full, id);
                g_hash_table_add(seen_ids, g_strdup(id));
                if (item) g_ptr_array_add(out, item);
            }
            g_free(id);
        }
        g_free(full);
    }
    g_dir_close(dir);
}

static GPtrArray *scan_desktop_browsers(void) {
    GPtrArray *out = g_ptr_array_new_with_free_func((GDestroyNotify) browser_list_item_free);
    GHashTable *seen_ids = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, NULL);

    gchar *user_apps = g_build_filename(g_get_user_data_dir(), "applications", NULL);
    scan_dir_into(user_apps, seen_ids, out);
    g_free(user_apps);

    const gchar *const *system_dirs = g_get_system_data_dirs();
    for (int i = 0; system_dirs[i]; i++) {
        gchar *sys_apps = g_build_filename(system_dirs[i], "applications", NULL);
        scan_dir_into(sys_apps, seen_ids, out);
        g_free(sys_apps);
    }

    g_hash_table_destroy(seen_ids);
    return out;
}

static gint compare_by_order(gconstpointer a, gconstpointer b) {
    const BrowserListItem *ia = *(const BrowserListItem **) a;
    const BrowserListItem *ib = *(const BrowserListItem **) b;
    return ia->order_index - ib->order_index;
}

/* Merges raw scanned browsers with `data`'s prefs, appends custom browsers, and
 * synthesizes+materializes order_index for anything not yet persisted — mirrors
 * BrowserPrefsRepository.Merge / the "lock in order before any single mutation" rule. */
static GPtrArray *merge_with_prefs(GPtrArray *scanned, LinkerData *data) {
    GPtrArray *result = g_ptr_array_new_with_free_func((GDestroyNotify) browser_list_item_free);

    gint max_order = -1;
    for (guint i = 0; i < data->browser_prefs->len; i++) {
        BrowserPrefEntity *e = g_ptr_array_index(data->browser_prefs, i);
        if (e->order_index > max_order) max_order = e->order_index;
    }
    gint next_synthetic = max_order + 1;

    for (guint i = 0; i < scanned->len; i++) {
        BrowserListItem *item = g_ptr_array_index(scanned, i);
        BrowserPrefEntity *pref = linker_data_find_browser_pref(data, item->id);
        if (pref) {
            item->hidden = pref->hidden;
            item->order_index = pref->order_index;
            g_free(item->display_label);
            item->display_label = g_strdup(pref->custom_label && *pref->custom_label ? pref->custom_label : item->system_label);
            item->custom_executable_path = g_strdup(pref->custom_executable_path);
            item->custom_icon_path = g_strdup(pref->custom_icon_path);
            item->extra_arguments = g_strdup(pref->extra_arguments);
        } else {
            item->order_index = next_synthetic++;
            /* materialize so future scans/reorders see a stable, persisted order */
            BrowserPrefEntity *materialized = browser_pref_entity_new(item->id);
            materialized->order_index = item->order_index;
            g_ptr_array_add(data->browser_prefs, materialized);
        }
        g_ptr_array_add(result, item);
    }
    /* Items were moved into `result` by reference; free just the container, not the
     * elements (free_seg=FALSE skips invoking scanned's element_free_func). */
    gpointer *raw = g_ptr_array_free(scanned, FALSE);
    g_free(raw);

    for (guint i = 0; i < data->browser_prefs->len; i++) {
        BrowserPrefEntity *pref = g_ptr_array_index(data->browser_prefs, i);
        if (!pref->is_custom) continue;
        BrowserListItem *item = g_new0(BrowserListItem, 1);
        browser_list_item_init_defaults(item);
        item->id = g_strdup(pref->id);
        item->system_label = g_strdup("Custom browser");
        item->display_label = g_strdup(pref->custom_label && *pref->custom_label ? pref->custom_label : "Custom browser");
        item->system_exec_cmdline = g_strdup("");
        item->system_icon = g_strdup("");
        item->custom_executable_path = g_strdup(pref->custom_executable_path);
        item->custom_icon_path = g_strdup(pref->custom_icon_path);
        item->extra_arguments = g_strdup(pref->extra_arguments);
        item->hidden = pref->hidden;
        item->order_index = pref->order_index;
        item->is_custom = TRUE;
        g_ptr_array_add(result, item);
    }

    g_ptr_array_sort(result, compare_by_order);
    return result;
}

GPtrArray *browsers_get_manage_list(LinkerData *data) {
    GPtrArray *scanned = scan_desktop_browsers();
    return merge_with_prefs(scanned, data);
}

/* ---------- 5-minute TTL cache of the raw scan, for the chooser's fast path ---------- */

static gchar *cache_file_path(void) {
    return g_build_filename(linker_data_dir(), "browsers-cache.json", NULL);
}

static GPtrArray *load_cached_scan(void) {
    gchar *path = cache_file_path();
    gchar *contents = NULL;
    gboolean ok = g_file_get_contents(path, &contents, NULL, NULL);
    g_free(path);
    if (!ok) return NULL;

    JsonValue *root = json_parse(contents, NULL);
    g_free(contents);
    if (!root) return NULL;

    gint64 cached_at = json_get_int(root, "cachedAtMillis", 0);
    if (linker_now_millis() - cached_at > CACHE_TTL_SECONDS * 1000) {
        json_value_free(root);
        return NULL;
    }

    JsonValue *items = json_object_get(root, "items");
    if (!items || items->type != JSON_ARRAY) {
        json_value_free(root);
        return NULL;
    }

    GPtrArray *out = g_ptr_array_new_with_free_func((GDestroyNotify) browser_list_item_free);
    for (guint i = 0; i < items->v.arr->len; i++) {
        JsonValue *o = g_ptr_array_index(items->v.arr, i);
        BrowserListItem *item = g_new0(BrowserListItem, 1);
        browser_list_item_init_defaults(item);
        item->id = g_strdup(json_get_string(o, "id", ""));
        item->system_label = g_strdup(json_get_string(o, "label", ""));
        item->display_label = g_strdup(item->system_label);
        item->system_exec_cmdline = g_strdup(json_get_string(o, "exec", ""));
        item->system_icon = g_strdup(json_get_string(o, "icon", ""));
        g_ptr_array_add(out, item);
    }
    json_value_free(root);
    return out;
}

static void save_cached_scan(GPtrArray *scanned) {
    JsonValue *root = json_new_object();
    json_object_set(root, "cachedAtMillis", json_new_number(linker_now_millis()));
    JsonValue *items = json_new_array();
    for (guint i = 0; i < scanned->len; i++) {
        BrowserListItem *item = g_ptr_array_index(scanned, i);
        JsonValue *o = json_new_object();
        json_object_set(o, "id", json_new_string(item->id));
        json_object_set(o, "label", json_new_string(item->system_label));
        json_object_set(o, "exec", json_new_string(item->system_exec_cmdline));
        json_object_set(o, "icon", json_new_string(item->system_icon));
        json_array_append(items, o);
    }
    json_object_set(root, "items", items);

    gchar *text = json_to_string(root);
    json_value_free(root);
    gchar *path = cache_file_path();
    g_file_set_contents(path, text, -1, NULL);
    g_free(path);
    g_free(text);
}

GPtrArray *browsers_get_visible_list(LinkerData *data) {
    GPtrArray *scanned = load_cached_scan();
    if (!scanned) {
        scanned = scan_desktop_browsers();
        save_cached_scan(scanned);
    }

    GPtrArray *merged = merge_with_prefs(scanned, data);

    GPtrArray *visible = g_ptr_array_new_with_free_func((GDestroyNotify) browser_list_item_free);
    for (guint i = 0; i < merged->len; i++) {
        BrowserListItem *item = g_ptr_array_index(merged, i);
        if (!item->hidden) {
            g_ptr_array_add(visible, item);
            merged->pdata[i] = NULL;
        }
    }
    g_ptr_array_free(merged, TRUE);
    return visible;
}
