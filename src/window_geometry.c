/* Stores a flat key -> {width, height} map as JSON, flock-guarded atomic writes. */
#include <gtk/gtk.h>
#include <glib/gstdio.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/file.h>
#include <unistd.h>

#include "window_geometry.h"
#include "json_min.h"
#include "data_store.h"

typedef struct {
    guint width;
    guint height;
} WinGeom;

typedef struct {
    gint width;
    gint height;
} LastSize;

static GHashTable *geom_cache = NULL; /* key -> WinGeom* */

static gchar *geometry_file_path(void) {
    return g_build_filename(linker_data_dir(), "settings.json", NULL);
}

static gchar *geometry_lock_path(void) {
    return g_build_filename(linker_data_dir(), "settings.lock", NULL);
}

static int acquire_geometry_lock(void) {
    gchar *path = geometry_lock_path();
    int fd = open(path, O_CREAT | O_RDWR, 0600);
    g_free(path);
    if (fd < 0) {
        g_warning("could not open window-size lock file: %s", g_strerror(errno));
        return -1;
    }
    if (flock(fd, LOCK_EX) != 0) {
        g_warning("could not acquire window-size lock: %s", g_strerror(errno));
        close(fd);
        return -1;
    }
    return fd;
}

static void release_geometry_lock(int fd) {
    if (fd < 0) return;
    flock(fd, LOCK_UN);
    close(fd);
}

static void load_geometry(void) {
    if (geom_cache) return;
    geom_cache = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, g_free);

    int lock_fd = acquire_geometry_lock();
    gchar *path = geometry_file_path();
    gchar *contents = NULL;
    GError *error = NULL;
    gboolean ok = g_file_get_contents(path, &contents, NULL, &error);
    g_free(path);
    release_geometry_lock(lock_fd);

    if (!ok) {
        if (!g_error_matches(error, G_FILE_ERROR, G_FILE_ERROR_NOENT)) {
            g_warning("could not read window-size store: %s", error->message);
        }
        g_clear_error(&error);
        return;
    }

    JsonValue *root = json_parse(contents, &error);
    g_free(contents);
    if (!root) {
        if (error) g_warning("could not parse window-size store: %s", error->message);
        g_clear_error(&error);
        return;
    }

    JsonValue *wins = json_object_get(root, "windows");
    if (wins && wins->type == JSON_OBJECT) {
        for (guint i = 0; i < wins->v.obj->len; i++) {
            JsonMember *member = g_ptr_array_index(wins->v.obj, i);
            if (member->value->type != JSON_OBJECT) continue;
            guint w = (guint) json_get_int(member->value, "width", 0);
            guint h = (guint) json_get_int(member->value, "height", 0);
            if (w < 10 || h < 10) continue;
            WinGeom *geom = g_new(WinGeom, 1);
            geom->width = w;
            geom->height = h;
            g_hash_table_insert(geom_cache, g_strdup(member->key), geom);
        }
    }
    json_value_free(root);
}

static void save_geometry(void) {
    if (!geom_cache) return;

    JsonValue *root = json_new_object();
    JsonValue *wins = json_new_object();
    GHashTableIter iter;
    gpointer key_buf, val_buf;
    g_hash_table_iter_init(&iter, geom_cache);
    while (g_hash_table_iter_next(&iter, &key_buf, &val_buf)) {
        WinGeom *geom = val_buf;
        JsonValue *entry = json_new_object();
        json_object_set(entry, "width", json_new_number(geom->width));
        json_object_set(entry, "height", json_new_number(geom->height));
        json_object_set(wins, key_buf, entry);
    }
    json_object_set(root, "windows", wins);

    gchar *text = json_to_string(root);
    json_value_free(root);

    int lock_fd = acquire_geometry_lock();
    if (lock_fd >= 0) {
        gchar *path = geometry_file_path();
        gchar *tmp_path = g_strdup_printf("%s.tmp", path);
        if (!g_file_set_contents(tmp_path, text, -1, NULL)) {
            g_warning("could not write window-size store");
        } else if (g_rename(tmp_path, path) != 0) {
            g_warning("could not rename window-size store: %s", g_strerror(errno));
        }
        g_free(tmp_path);
        g_free(path);
        release_geometry_lock(lock_fd);
    }
    g_free(text);
}

static void window_geometry_set(const gchar *key, guint width, guint height) {
    if (key == NULL || width == 0 || height == 0) return;
    load_geometry();
    WinGeom *geom = g_hash_table_lookup(geom_cache, key);
    if (!geom) {
        geom = g_new(WinGeom, 1);
        g_hash_table_insert(geom_cache, g_strdup(key), geom);
    }
    geom->width = width;
    geom->height = height;
    save_geometry();
}

/* Capture the live size on configure (gtk_window_get_size() returns the last
 * requested default, not the real allocation); persist once at destroy time. */
static gboolean on_window_configure(GtkWidget *window, GdkEventConfigure *event, gpointer user_data) {
    (void) user_data;
    if (event->width <= 0 || event->height <= 0) return FALSE;
    LastSize *last = g_object_get_data(G_OBJECT(window), "linker-geometry-size");
    if (!last) {
        last = g_new0(LastSize, 1);
        g_object_set_data_full(G_OBJECT(window), "linker-geometry-size", last, g_free);
    }
    last->width = event->width;
    last->height = event->height;
    return FALSE;
}

static void on_window_destroy(GtkWidget *window, gpointer user_data) {
    (void) user_data;
    const gchar *key = g_object_get_data(G_OBJECT(window), "linker-geometry-key");
    if (!key) return;
    LastSize *last = g_object_get_data(G_OBJECT(window), "linker-geometry-size");
    if (last && last->width > 0 && last->height > 0) {
        window_geometry_set(key, (guint) last->width, (guint) last->height);
    }
}

void window_geometry_apply(GtkWindow *window, const gchar *key) {
    load_geometry();
    WinGeom *geom = g_hash_table_lookup(geom_cache, key);
    if (geom) {
        gtk_window_set_default_size(window, (gint) geom->width, (gint) geom->height);
    }
    g_object_set_data_full(G_OBJECT(window), "linker-geometry-key", g_strdup(key), g_free);
    g_signal_connect(G_OBJECT(window), "configure-event", G_CALLBACK(on_window_configure), NULL);
    g_signal_connect(G_OBJECT(window), "destroy", G_CALLBACK(on_window_destroy), NULL);
}