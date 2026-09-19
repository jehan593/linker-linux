/* Regenerates data/icons/linker-*.png: two interlocking rings (Nord9 on Nord0),
 * an approximation of the original Android ic_launcher_foreground mark (that vector
 * source isn't available in this environment). Re-run if the palette or motif changes;
 * don't hand-edit the PNGs directly. Usage: ./generate_icon <out_dir>
 */
#define _USE_MATH_DEFINES
#include <math.h>
#include <cairo/cairo.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define NORD0_R 0x2E / 255.0
#define NORD0_G 0x34 / 255.0
#define NORD0_B 0x40 / 255.0

#define NORD9_R 0x81 / 255.0
#define NORD9_G 0xA1 / 255.0
#define NORD9_B 0xC1 / 255.0

static void draw_icon(cairo_t *cr, double size) {
    double corner = size * 0.22;

    /* Background: rounded square, Nord0 */
    cairo_new_sub_path(cr);
    cairo_arc(cr, size - corner, corner, corner, -M_PI / 2, 0);
    cairo_arc(cr, size - corner, size - corner, corner, 0, M_PI / 2);
    cairo_arc(cr, corner, size - corner, corner, M_PI / 2, M_PI);
    cairo_arc(cr, corner, corner, corner, M_PI, 3 * M_PI / 2);
    cairo_close_path(cr);
    cairo_set_source_rgb(cr, NORD0_R, NORD0_G, NORD0_B);
    cairo_fill(cr);

    /* Foreground: two interlocking rings, Nord9 */
    double ring_r = size * 0.165;
    double stroke_w = size * 0.075;
    double offset = ring_r * 0.85;
    double cy = size * 0.5;
    double cx1 = size * 0.5 - offset;
    double cx2 = size * 0.5 + offset;

    cairo_set_source_rgb(cr, NORD9_R, NORD9_G, NORD9_B);
    cairo_set_line_width(cr, stroke_w);
    cairo_set_line_cap(cr, CAIRO_LINE_CAP_ROUND);

    cairo_new_sub_path(cr);
    cairo_arc(cr, cx1, cy, ring_r, 0, 2 * M_PI);
    cairo_stroke(cr);

    cairo_new_sub_path(cr);
    cairo_arc(cr, cx2, cy, ring_r, 0, 2 * M_PI);
    cairo_stroke(cr);
}

static int render(const char *out_dir, int size) {
    cairo_surface_t *surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, size, size);
    cairo_t *cr = cairo_create(surface);
    draw_icon(cr, (double) size);

    char path[512];
    snprintf(path, sizeof(path), "%s/linker-%d.png", out_dir, size);
    cairo_status_t status = cairo_surface_write_to_png(surface, path);

    cairo_destroy(cr);
    cairo_surface_destroy(surface);

    if (status != CAIRO_STATUS_SUCCESS) {
        fprintf(stderr, "failed to write %s: %s\n", path, cairo_status_to_string(status));
        return 1;
    }
    printf("wrote %s\n", path);
    return 0;
}

int main(int argc, char **argv) {
    const char *out_dir = argc > 1 ? argv[1] : ".";
    int sizes[] = {16, 32, 48, 256};
    int rc = 0;
    for (size_t i = 0; i < sizeof(sizes) / sizeof(sizes[0]); i++) {
        rc |= render(out_dir, sizes[i]);
    }
    return rc;
}
