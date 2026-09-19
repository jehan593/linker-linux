/* Shared widget factories applying Nord control classes from theme.c. */
#ifndef LINKER_UI_WIDGETS_H
#define LINKER_UI_WIDGETS_H

#include <gtk/gtk.h>

/* Confirm an irreversible deletion with a red primary action. Enter confirms. */
gboolean ui_confirm_delete(GtkWindow *parent, const gchar *title, const gchar *consequence);

/* Rounded filled button (primary background). */
GtkWidget *ui_pill_button_new(const gchar *label_text);

/* Outlined secondary button. Cancel, error and warning actions are text-only. */
GtkWidget *ui_text_button_new(const gchar *label_text, const gchar *color_class);

/* Quiet icon button. `small` selects the 32x32 variant. */
GtkWidget *ui_icon_button_new(const gchar *icon_name, const gchar *tooltip, gboolean small);

GtkWidget *ui_title_label_new(const gchar *text);
GtkWidget *ui_label_label_new(const gchar *text);
GtkWidget *ui_body_small_label_new(const gchar *text);

/* Single-line entry, outlined style. */
GtkWidget *ui_outlined_entry_new(void);

/* Multiline text view in outlined style, wrapped in its own scroller.
 * *out_textview receives the inner GtkTextView. */
GtkWidget *ui_outlined_textview_new(GtkWidget **out_textview);

/* 1px divider. */
GtkWidget *ui_hairline_new(void);

/* Convenience: get/set plain text on a GtkTextView's buffer. Free the result with g_free. */
gchar *ui_textview_get_text(GtkTextView *view);
void ui_textview_set_text(GtkTextView *view, const gchar *text);

/* Bare Enter (no Shift/Ctrl/Alt) answers `dialog` with `response`. */
void ui_bind_enter(GtkWidget *widget, GtkWidget *dialog, gint response);

#endif
