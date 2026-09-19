#include "ui_widgets.h"

static void add_class(GtkWidget *widget, const gchar *class_name) {
    gtk_style_context_add_class(gtk_widget_get_style_context(widget), class_name);
}

GtkWidget *ui_pill_button_new(const gchar *label_text) {
    GtkWidget *button = gtk_button_new_with_label(label_text);
    gtk_widget_set_can_focus(button, FALSE);
    add_class(button, "flat");
    add_class(button, "pill-button");
    return button;
}

GtkWidget *ui_text_button_new(const gchar *label_text, const gchar *color_class) {
    GtkWidget *button = gtk_button_new_with_label(label_text);
    gtk_widget_set_can_focus(button, FALSE);
    add_class(button, "flat");
    add_class(button, "linker-text-button");
    add_class(button, color_class);
    if (g_strcmp0(label_text, "Cancel") == 0) add_class(button, "dismiss-button");
    return button;
}

GtkWidget *ui_icon_button_new(const gchar *icon_name, const gchar *tooltip, gboolean small) {
    GtkWidget *image = gtk_image_new_from_icon_name(icon_name, small ? GTK_ICON_SIZE_MENU : GTK_ICON_SIZE_BUTTON);
    GtkWidget *button = gtk_button_new();
    gtk_widget_set_can_focus(button, FALSE);
    gtk_button_set_image(GTK_BUTTON(button), image);
    gtk_button_set_relief(GTK_BUTTON(button), GTK_RELIEF_NONE);
    add_class(button, "flat");
    add_class(button, "icon-button");
    if (small) add_class(button, "icon-button-small");
    if (tooltip) {
        gtk_widget_set_tooltip_text(button, tooltip);
        atk_object_set_name(gtk_widget_get_accessible(button), tooltip);
    }
    return button;
}

GtkWidget *ui_title_label_new(const gchar *text) {
    GtkWidget *label = gtk_label_new(text);
    add_class(label, "linker-title");
    gtk_label_set_xalign(GTK_LABEL(label), 0.0);
    return label;
}

GtkWidget *ui_label_label_new(const gchar *text) {
    GtkWidget *label = gtk_label_new(text);
    add_class(label, "linker-label");
    gtk_label_set_xalign(GTK_LABEL(label), 0.0);
    return label;
}

GtkWidget *ui_body_small_label_new(const gchar *text) {
    GtkWidget *label = gtk_label_new(text);
    add_class(label, "linker-body-small");
    gtk_label_set_xalign(GTK_LABEL(label), 0.0);
    return label;
}

GtkWidget *ui_outlined_entry_new(void) {
    GtkWidget *entry = gtk_entry_new();
    add_class(entry, "outlined");
    return entry;
}

static gboolean on_textview_focus(GtkWidget *widget, GdkEventFocus *event, gpointer scroller) {
    (void) widget;
    GtkStyleContext *style = gtk_widget_get_style_context(GTK_WIDGET(scroller));
    if (event->in) gtk_style_context_add_class(style, "focused");
    else gtk_style_context_remove_class(style, "focused");
    return FALSE;
}

GtkWidget *ui_outlined_textview_new(GtkWidget **out_textview) {
    GtkWidget *scroller = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroller), GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
    add_class(scroller, "outlined");

    GtkWidget *textview = gtk_text_view_new();
    gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(textview), GTK_WRAP_WORD_CHAR);
    gtk_text_view_set_accepts_tab(GTK_TEXT_VIEW(textview), FALSE);
    /* CSS padding doesn't position GtkTextView's text — the margin props do. */
    gtk_text_view_set_left_margin(GTK_TEXT_VIEW(textview), 12);
    gtk_text_view_set_right_margin(GTK_TEXT_VIEW(textview), 12);
    gtk_text_view_set_top_margin(GTK_TEXT_VIEW(textview), 10);
    gtk_text_view_set_bottom_margin(GTK_TEXT_VIEW(textview), 10);
    add_class(textview, "outlined");
    g_signal_connect(textview, "focus-in-event", G_CALLBACK(on_textview_focus), scroller);
    g_signal_connect(textview, "focus-out-event", G_CALLBACK(on_textview_focus), scroller);

    gtk_container_add(GTK_CONTAINER(scroller), textview);
    if (out_textview) *out_textview = textview;
    return scroller;
}

GtkWidget *ui_hairline_new(void) {
    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_widget_set_size_request(box, -1, 1);
    add_class(box, "hairline");
    return box;
}

gchar *ui_textview_get_text(GtkTextView *view) {
    GtkTextBuffer *buffer = gtk_text_view_get_buffer(view);
    GtkTextIter start, end;
    gtk_text_buffer_get_bounds(buffer, &start, &end);
    return gtk_text_buffer_get_text(buffer, &start, &end, FALSE);
}

static void on_confirmation_action(GtkButton *button, gpointer dialog) {
    gtk_dialog_response(GTK_DIALOG(dialog),
        GPOINTER_TO_INT(g_object_get_data(G_OBJECT(button), "response")));
}

gboolean ui_confirm_delete(GtkWindow *parent, const gchar *title, const gchar *consequence) {
    GtkWidget *dialog = gtk_dialog_new_with_buttons(title, parent,
        GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT, NULL, NULL);
    gtk_window_set_default_size(GTK_WINDOW(dialog), 400, -1);
    GtkWidget *content = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    add_class(content, "content-pad-16");
    gtk_box_set_spacing(GTK_BOX(content), 12);
    gtk_box_pack_start(GTK_BOX(content), ui_title_label_new(title), FALSE, FALSE, 0);
    GtkWidget *body = ui_body_small_label_new(consequence);
    gtk_label_set_line_wrap(GTK_LABEL(body), TRUE);
    gtk_label_set_max_width_chars(GTK_LABEL(body), 44);
    gtk_box_pack_start(GTK_BOX(content), body, FALSE, FALSE, 0);
    GtkWidget *actions = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
    gtk_widget_set_halign(actions, GTK_ALIGN_END);
    GtkWidget *cancel = ui_text_button_new("Cancel", "text-button-neutral");
    GtkWidget *confirm = ui_pill_button_new("Delete");
    add_class(confirm, "destructive-button");
    g_object_set_data(G_OBJECT(cancel), "response", GINT_TO_POINTER(GTK_RESPONSE_CANCEL));
    g_object_set_data(G_OBJECT(confirm), "response", GINT_TO_POINTER(GTK_RESPONSE_ACCEPT));
    g_signal_connect(cancel, "clicked", G_CALLBACK(on_confirmation_action), dialog);
    g_signal_connect(confirm, "clicked", G_CALLBACK(on_confirmation_action), dialog);
    gtk_box_pack_start(GTK_BOX(actions), cancel, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(actions), confirm, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(content), actions, FALSE, FALSE, 0);
    gtk_widget_set_can_default(confirm, TRUE);
    gtk_window_set_default(GTK_WINDOW(dialog), confirm);
    gtk_widget_show_all(dialog);
    gboolean accepted = gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT;
    gtk_widget_destroy(dialog);
    return accepted;
}

void ui_textview_set_text(GtkTextView *view, const gchar *text) {
    GtkTextBuffer *buffer = gtk_text_view_get_buffer(view);
    gtk_text_buffer_set_text(buffer, text ? text : "", -1);
}

typedef struct {
    GtkWidget *dialog;
    gint response;
} EnterBinding;

static gboolean on_enter_key(GtkWidget *widget, GdkEventKey *event, gpointer data) {
    (void) widget;
    EnterBinding *binding = data;
    if (event->keyval == GDK_KEY_Return &&
        !(event->state & (GDK_SHIFT_MASK | GDK_CONTROL_MASK | GDK_MOD1_MASK))) {
        gtk_dialog_response(GTK_DIALOG(binding->dialog), binding->response);
        return TRUE;
    }
    return FALSE;
}

/* Pressing Enter (without Shift/Ctrl/Alt) answers the dialog. Shift/Ctrl+Enter
 * still insert a line break in multi-line fields. */
static void enter_binding_free(gpointer data, GClosure *closure) {
    (void) closure;
    g_free(data);
}

void ui_bind_enter(GtkWidget *widget, GtkWidget *dialog, gint response) {
    EnterBinding *binding = g_new0(EnterBinding, 1);
    binding->dialog = dialog;
    binding->response = response;
    g_signal_connect_data(widget, "key-press-event", G_CALLBACK(on_enter_key),
                          binding, enter_binding_free, 0);
}
