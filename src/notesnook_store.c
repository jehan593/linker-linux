#include "notesnook_store.h"
#include "data_store.h" /* linker_data_dir() */
#include "json_min.h"

static gchar *settings_file_path(void) {
    return g_build_filename(linker_data_dir(), "settings.json", NULL);
}

NotesnookSettings *notesnook_settings_load(void) {
    NotesnookSettings *s = g_new0(NotesnookSettings, 1);

    gchar *path = settings_file_path();
    gchar *contents = NULL;
    GError *error = NULL;
    gboolean ok = g_file_get_contents(path, &contents, NULL, &error);
    g_free(path);

    if (!ok) {
        g_clear_error(&error);
        return s;
    }

    JsonValue *root = json_parse(contents, &error);
    g_free(contents);
    if (!root) {
        g_warning("could not parse notesnook settings: %s", error ? error->message : "unknown error");
        g_clear_error(&error);
        return s;
    }

    s->api_key = g_strdup(json_get_string(root, "notesnookApiKey", NULL));
    s->tag_id = g_strdup(json_get_string(root, "notesnookTagId", NULL));
    json_value_free(root);
    return s;
}

gboolean notesnook_settings_save(const NotesnookSettings *settings, GError **error) {
    JsonValue *root = json_new_object();
    json_object_set(root, "notesnookApiKey", json_new_string(settings->api_key));
    json_object_set(root, "notesnookTagId", json_new_string(settings->tag_id));

    gchar *text = json_to_string(root);
    json_value_free(root);

    gchar *path = settings_file_path();
    gboolean ok = g_file_set_contents(path, text, -1, error);
    g_free(path);
    g_free(text);
    return ok;
}

void notesnook_settings_free(NotesnookSettings *settings) {
    if (!settings) return;
    g_free(settings->api_key);
    g_free(settings->tag_id);
    g_free(settings);
}
