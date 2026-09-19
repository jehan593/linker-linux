#include "icon_resolve.h"
#include <gtk/gtk.h>
#include <math.h>
#include <string.h>

GdkPixbuf *icon_resolve_from_file(const gchar *path, gint size) {
    if (!path || !*path) return NULL;
    GError *error = NULL;
    GdkPixbuf *pixbuf = gdk_pixbuf_new_from_file_at_scale(path, size, size, TRUE, &error);
    if (!pixbuf) {
        g_warning("could not load icon file '%s': %s", path, error ? error->message : "unknown error");
        g_clear_error(&error);
        return NULL;
    }
    return pixbuf;
}

GdkPixbuf *icon_resolve_placeholder(gint size) {
    cairo_surface_t *surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, size, size);
    cairo_t *cr = cairo_create(surface);

    double cx = size / 2.0, cy = size / 2.0, r = size / 2.0 - 1.0;
    cairo_arc(cr, cx, cy, r, 0, 2 * M_PI);
    cairo_set_source_rgba(cr, 0x4C / 255.0, 0x56 / 255.0, 0x6A / 255.0, 1.0); /* Nord3 */
    cairo_fill_preserve(cr);
    cairo_set_source_rgba(cr, 0x81 / 255.0, 0xA1 / 255.0, 0xC1 / 255.0, 1.0); /* Nord9 */
    cairo_set_line_width(cr, size * 0.06);
    cairo_stroke(cr);

    /* Simple globe glyph: horizontal + vertical meridian arcs. */
    cairo_set_line_width(cr, size * 0.045);
    cairo_move_to(cr, cx - r * 0.55, cy);
    cairo_line_to(cr, cx + r * 0.55, cy);
    cairo_stroke(cr);
    cairo_save(cr);
    cairo_translate(cr, cx, cy);
    cairo_scale(cr, 0.42, 1.0);
    cairo_arc(cr, 0, 0, r * 0.85, 0, 2 * M_PI);
    cairo_restore(cr);
    cairo_stroke(cr);

    GdkPixbuf *pixbuf = gdk_pixbuf_get_from_surface(surface, 0, 0, size, size);
    cairo_destroy(cr);
    cairo_surface_destroy(surface);
    return pixbuf;
}

GdkPixbuf *icon_resolve_browser_icon(const gchar *icon_field, gint size) {
    if (icon_field && *icon_field) {
        if (g_path_is_absolute(icon_field) && g_file_test(icon_field, G_FILE_TEST_EXISTS)) {
            GdkPixbuf *pixbuf = icon_resolve_from_file(icon_field, size);
            if (pixbuf) return pixbuf;
        } else {
            GtkIconTheme *theme = gtk_icon_theme_get_default();
            GError *error = NULL;
            GdkPixbuf *pixbuf = gtk_icon_theme_load_icon(theme, icon_field, size, GTK_ICON_LOOKUP_FORCE_SIZE, &error);
            if (pixbuf) return pixbuf;
            g_clear_error(&error);

            /* Try stripping a possible extension as a last resort. */
            gchar *base = g_path_get_basename(icon_field);
            gchar *dot = strrchr(base, '.');
            if (dot) *dot = '\0';
            pixbuf = gtk_icon_theme_load_icon(theme, base, size, GTK_ICON_LOOKUP_FORCE_SIZE, NULL);
            g_free(base);
            if (pixbuf) return pixbuf;
        }
    }
    return icon_resolve_placeholder(size);
}
