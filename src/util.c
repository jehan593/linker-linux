#include "util.h"
#include <string.h>

gchar *linker_url_to_host(const gchar *url) {
    if (!url || !*url) return NULL;
    GError *error = NULL;
    GUri *uri = g_uri_parse(url, G_URI_FLAGS_PARSE_RELAXED, &error);
    if (!uri) {
        g_clear_error(&error);
        return NULL;
    }
    const gchar *host = g_uri_get_host(uri);
    gchar *result = (host && *host) ? g_strdup(host) : NULL;
    g_uri_unref(uri);
    return result;
}

gchar **linker_split_args(const gchar *args_str, gint *out_argc) {
    if (out_argc) *out_argc = 0;
    if (!args_str || !*args_str) return NULL;

    gchar *trimmed = g_strdup(args_str);
    g_strstrip(trimmed);
    if (!*trimmed) {
        g_free(trimmed);
        return NULL;
    }

    gint argc = 0;
    gchar **argv = NULL;
    GError *error = NULL;
    if (!g_shell_parse_argv(trimmed, &argc, &argv, &error)) {
        g_warning("failed to split argument string '%s': %s", trimmed, error ? error->message : "unknown error");
        g_clear_error(&error);
        g_free(trimmed);
        return NULL;
    }
    g_free(trimmed);
    if (out_argc) *out_argc = argc;
    return argv;
}

gint64 linker_now_millis(void) {
    return g_get_real_time() / 1000;
}

gchar *linker_format_time_of_day(gint64 millis) {
    GDateTime *utc = g_date_time_new_from_unix_utc(millis / 1000);
    GDateTime *local = g_date_time_to_local(utc);
    gchar *result = g_date_time_format(local, "%l:%M %p");
    gchar *trimmed = g_strdup(g_strstrip(result));
    g_free(result);
    g_date_time_unref(local);
    g_date_time_unref(utc);
    return trimmed;
}

gchar *linker_format_day_header(gint64 millis) {
    GDateTime *utc = g_date_time_new_from_unix_utc(millis / 1000);
    GDateTime *local = g_date_time_to_local(utc);
    GDateTime *now = g_date_time_new_now_local();

    gint y1, m1, d1, y2, m2, d2;
    g_date_time_get_ymd(local, &y1, &m1, &d1);
    g_date_time_get_ymd(now, &y2, &m2, &d2);

    gchar *result;
    if (y1 == y2 && m1 == m2 && d1 == d2) {
        result = g_strdup("Today");
    } else {
        GDateTime *yesterday = g_date_time_add_days(now, -1);
        gint y3, m3, d3;
        g_date_time_get_ymd(yesterday, &y3, &m3, &d3);
        if (y1 == y3 && m1 == m3 && d1 == d3) {
            result = g_strdup("Yesterday");
        } else {
            result = g_date_time_format(local, "%b %-d, %Y");
        }
        g_date_time_unref(yesterday);
    }

    g_date_time_unref(now);
    g_date_time_unref(local);
    g_date_time_unref(utc);
    return result;
}
