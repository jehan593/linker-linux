/* Nord dark/light palette, GTK3 CSS stylesheet, and OS dark/light detection
 * with live watching. */
#ifndef LINKER_THEME_H
#define LINKER_THEME_H

#include <gtk/gtk.h>

typedef struct {
    const char *primary, *on_primary, *primary_container, *on_primary_container;
    const char *secondary, *on_secondary, *secondary_container, *on_secondary_container;
    const char *tertiary, *on_tertiary, *tertiary_container, *on_tertiary_container;
    const char *background, *on_background;
    const char *surface, *on_surface, *surface_variant, *on_surface_variant;
    const char *surface_dim, *surface_bright;
    const char *surface_container_lowest, *surface_container_low, *surface_container;
    const char *surface_container_high, *surface_container_highest;
    const char *error, *on_error, *error_container, *on_error_container;
    const char *outline, *outline_variant;
    const char *inverse_surface, *inverse_on_surface, *inverse_primary;
    const char *row_hover;
} NordPalette;

/* Search-highlight colors (fixed, not theme-swapped). */
#define LINKER_HIGHLIGHT_BG "#81A1C1"
#define LINKER_HIGHLIGHT_FG "#2E3440"

/* Call once at startup after gtk_init. */
void linker_theme_init(void);

gboolean linker_theme_is_dark(void);

const NordPalette *linker_theme_palette(void);

#endif
