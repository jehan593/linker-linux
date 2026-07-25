#include "toast.h"

static gboolean hide_cb(gpointer user_data) {
    GtkWidget *revealer = GTK_WIDGET(user_data);
    gtk_revealer_set_reveal_child(GTK_REVEALER(revealer), FALSE);
    g_object_set_data(G_OBJECT(revealer), "linker-toast-timeout-id", GUINT_TO_POINTER(0));
    return G_SOURCE_REMOVE;
}

GtkWidget *toast_host_new(void) {
    GtkWidget *revealer = gtk_revealer_new();
    gtk_revealer_set_transition_type(GTK_REVEALER(revealer), GTK_REVEALER_TRANSITION_TYPE_CROSSFADE);
    gtk_revealer_set_transition_duration(GTK_REVEALER(revealer), 180);
    gtk_widget_set_halign(revealer, GTK_ALIGN_CENTER);
    gtk_widget_set_valign(revealer, GTK_ALIGN_END);
    gtk_widget_set_margin_bottom(revealer, 12);
    gtk_widget_set_can_focus(revealer, FALSE);

    GtkWidget *bubble = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_style_context_add_class(gtk_widget_get_style_context(bubble), "toast-bubble");

    GtkWidget *label = gtk_label_new("");
    gtk_style_context_add_class(gtk_widget_get_style_context(label), "toast-text");
    gtk_box_pack_start(GTK_BOX(bubble), label, FALSE, FALSE, 0);

    gtk_container_add(GTK_CONTAINER(revealer), bubble);
    g_object_set_data(G_OBJECT(revealer), "linker-toast-label", label);

    gtk_widget_show_all(bubble);
    return revealer;
}

void toast_host_show(GtkWidget *host, const gchar *message) {
    GtkWidget *label = g_object_get_data(G_OBJECT(host), "linker-toast-label");
    gtk_label_set_text(GTK_LABEL(label), message);
    gtk_revealer_set_reveal_child(GTK_REVEALER(host), TRUE);

    guint existing = GPOINTER_TO_UINT(g_object_get_data(G_OBJECT(host), "linker-toast-timeout-id"));
    if (existing) g_source_remove(existing);
    guint id = g_timeout_add(2200, hide_cb, host);
    g_object_set_data(G_OBJECT(host), "linker-toast-timeout-id", GUINT_TO_POINTER(id));
}
