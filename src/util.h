/* URL parsing, shell-style argument splitting, time/date formatting. */
#ifndef LINKER_UTIL_H
#define LINKER_UTIL_H

#include <glib.h>

/* Returns the host of `url` (e.g. "example.com"), or NULL if not parseable. */
gchar *linker_url_to_host(const gchar *url);

/* Splits a space-separated, quote-aware argument string into an argv array.
 * Returns NULL for empty/NULL input. Caller frees with g_strfreev. */
gchar **linker_split_args(const gchar *args_str, gint *out_argc);

gint64 linker_now_millis(void);
gchar *linker_format_time_of_day(gint64 millis);  /* "2:45 PM" */
gchar *linker_format_day_header(gint64 millis);   /* "Today" / "Yesterday" / "Jul 20, 2026" */

#endif
