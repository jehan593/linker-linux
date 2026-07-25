#include "ui_settings_dialog.h"
#include "ui_widgets.h"
#include "notesnook_store.h"

static void response_ok(GtkButton *btn, gpointer dialog) {
    (void) btn;
    gtk_dialog_response(GTK_DIALOG(dialog), GTK_RESPONSE_OK);
}
static void response_cancel(GtkButton *btn, gpointer dialog) {
    (void) btn;
    gtk_dialog_response(GTK_DIALOG(dialog), GTK_RESPONSE_CANCEL);
}

void ui_settings_dialog_run(GtkWindow *parent) {
    GtkWidget *dialog = gtk_dialog_new_with_buttons(
        "Notesnook", parent, GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT, NULL, NULL);
    gtk_widget_set_size_request(dialog, 360, -1);
    gtk_window_set_default_size(GTK_WINDOW(dialog), 420, -1);
    gtk_window_set_resizable(GTK_WINDOW(dialog), FALSE);
    gtk_window_set_position(GTK_WINDOW(dialog), GTK_WIN_POS_CENTER_ON_PARENT);

    GtkWidget *content = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    gtk_style_context_add_class(gtk_widget_get_style_context(content), "content-pad-20");
    gtk_box_set_spacing(GTK_BOX(content), 8);

    GtkWidget *title_label = ui_title_label_new("Notesnook");
    gtk_widget_set_margin_bottom(title_label, 16);
    gtk_box_pack_start(GTK_BOX(content), title_label, FALSE, FALSE, 0);

    GtkWidget *intro = ui_body_small_label_new("From Notesnook: Settings > Inbox > Create Key.");
    gtk_label_set_line_wrap(GTK_LABEL(intro), TRUE);
    gtk_label_set_max_width_chars(GTK_LABEL(intro), 46);
    gtk_widget_set_margin_bottom(intro, 8);
    gtk_box_pack_start(GTK_BOX(content), intro, FALSE, FALSE, 0);

    gtk_box_pack_start(GTK_BOX(content), ui_label_label_new("Inbox API key"), FALSE, FALSE, 0);
    GtkWidget *api_key_entry = ui_outlined_entry_new();
    gtk_entry_set_visibility(GTK_ENTRY(api_key_entry), FALSE);
    gtk_entry_set_invisible_char(GTK_ENTRY(api_key_entry), 0x25CF); /* ● */
    gtk_box_pack_start(GTK_BOX(content), api_key_entry, FALSE, FALSE, 0);

    GtkWidget *tag_label = ui_label_label_new("Tag ID (optional)");
    gtk_widget_set_margin_top(tag_label, 8);
    gtk_box_pack_start(GTK_BOX(content), tag_label, FALSE, FALSE, 0);
    GtkWidget *tag_id_entry = ui_outlined_entry_new();
    gtk_box_pack_start(GTK_BOX(content), tag_id_entry, FALSE, FALSE, 0);

    GtkWidget *hint = ui_body_small_label_new(
        "Right-click a tag in Notesnook and choose Copy ID. Sent links are titled "
        "“Link: <url>”, with the send time and link in the note body.");
    gtk_label_set_line_wrap(GTK_LABEL(hint), TRUE);
    gtk_label_set_max_width_chars(GTK_LABEL(hint), 46);
    gtk_widget_set_margin_top(hint, 8);
    gtk_box_pack_start(GTK_BOX(content), hint, FALSE, FALSE, 0);

    GtkWidget *button_row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_widget_set_margin_top(button_row, 20);
    GtkWidget *spacer = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_widget_set_hexpand(spacer, TRUE);
    GtkWidget *cancel_btn = ui_text_button_new("Cancel", "text-button-neutral");
    GtkWidget *save_btn = ui_text_button_new("Save", "text-button-primary");
    gtk_box_pack_start(GTK_BOX(button_row), spacer, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(button_row), cancel_btn, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(button_row), save_btn, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(content), button_row, FALSE, FALSE, 0);

    NotesnookSettings *settings = notesnook_settings_load();
    gtk_entry_set_text(GTK_ENTRY(api_key_entry), settings->api_key ? settings->api_key : "");
    gtk_entry_set_text(GTK_ENTRY(tag_id_entry), settings->tag_id ? settings->tag_id : "");
    notesnook_settings_free(settings);

    gtk_entry_set_activates_default(GTK_ENTRY(api_key_entry), TRUE);
    gtk_entry_set_activates_default(GTK_ENTRY(tag_id_entry), TRUE);
    gtk_widget_set_can_default(save_btn, TRUE);
    gtk_dialog_set_default_response(GTK_DIALOG(dialog), GTK_RESPONSE_OK);

    g_signal_connect(save_btn, "clicked", G_CALLBACK(response_ok), dialog);
    g_signal_connect(cancel_btn, "clicked", G_CALLBACK(response_cancel), dialog);

    gtk_widget_show_all(dialog);
    gtk_widget_grab_focus(api_key_entry);

    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_OK) {
        NotesnookSettings updated = {0};
        updated.api_key = g_strstrip(g_strdup(gtk_entry_get_text(GTK_ENTRY(api_key_entry))));
        updated.tag_id = g_strstrip(g_strdup(gtk_entry_get_text(GTK_ENTRY(tag_id_entry))));
        if (!*updated.api_key) g_clear_pointer(&updated.api_key, g_free);
        if (!*updated.tag_id) g_clear_pointer(&updated.tag_id, g_free);
        GError *error = NULL;
        if (!notesnook_settings_save(&updated, &error)) {
            g_warning("could not save notesnook settings: %s", error ? error->message : "unknown error");
            g_clear_error(&error);
        }
        g_free(updated.api_key);
        g_free(updated.tag_id);
    }

    gtk_widget_destroy(dialog);
}
