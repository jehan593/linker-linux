/* Minimal hand-rolled JSON reader/writer for Linker's own small schema.
 * No json-glib dependency — keeps the footprint to GTK3 + GLib/GIO + libcurl. */
#ifndef LINKER_JSON_MIN_H
#define LINKER_JSON_MIN_H

#include <glib.h>

typedef enum {
    JSON_NULL,
    JSON_BOOL,
    JSON_NUMBER,
    JSON_STRING,
    JSON_ARRAY,
    JSON_OBJECT
} JsonValueType;

typedef struct JsonValue JsonValue;

struct JsonValue {
    JsonValueType type;
    union {
        gboolean b;
        gint64 n;
        gchar *s;
        GPtrArray *arr;  /* of JsonValue* */
        GPtrArray *obj;  /* of JsonMember* */
    } v;
};

typedef struct {
    gchar *key;
    JsonValue *value;
} JsonMember;

JsonValue *json_new_null(void);
JsonValue *json_new_bool(gboolean b);
JsonValue *json_new_number(gint64 n);
JsonValue *json_new_string(const gchar *s);
JsonValue *json_new_array(void);
JsonValue *json_new_object(void);

void json_array_append(JsonValue *array, JsonValue *item);
void json_object_set(JsonValue *object, const gchar *key, JsonValue *value);

JsonValue *json_object_get(const JsonValue *object, const gchar *key);

/* Accessors with safe defaults when the value is missing or wrong type. */
const gchar *json_get_string(const JsonValue *object, const gchar *key, const gchar *default_value);
gint64 json_get_int(const JsonValue *object, const gchar *key, gint64 default_value);
gboolean json_get_bool(const JsonValue *object, const gchar *key, gboolean default_value);

void json_value_free(JsonValue *value);

JsonValue *json_parse(const gchar *text, GError **error);

/* Serializes with 2-space pretty printing. */
gchar *json_to_string(const JsonValue *value);

#endif
