/* Small shared helpers: URL parsing, shell-style argument splitting, time/date
 * formatting for the saved-links list, and HTML escaping for the Notesnook body. */
#ifndef LINKER_UTIL_H
#define LINKER_UTIL_H

#include <glib.h>

/* Returns the host of `url` (e.g. "example.com"), or NULL if it doesn't parse as an
 * absolute URI — caller falls back to showing "Open link". Free with g_free. */
gchar *linker_url_to_host(const gchar *url);

/* Splits a space-separated, double-quote-aware argument string (extra launch args or
 * a desktop Exec= command line) into an argv array via g_shell_parse_argv. Returns
 * NULL and sets *out_argc = 0 for an empty/NULL input; caller frees with g_strfreev. */
gchar **linker_split_args(const gchar *args_str, gint *out_argc);

gint64 linker_now_millis(void);

/* "2:45 PM" style, local time. Free with g_free. */
gchar *linker_format_time_of_day(gint64 millis);

/* "Today" / "Yesterday" / "Jul 20, 2026" depending on how `millis` compares to the
 * current local date. Free with g_free. */
gchar *linker_format_day_header(gint64 millis);

/* Escapes &, <, >, " for embedding in the Notesnook HTML note body. Free with g_free. */
gchar *linker_html_escape(const gchar *s);

#endif
