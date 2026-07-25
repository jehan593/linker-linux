#include "ui_edit_saved_link_dialog.h"
#include "ui_saved_links_view.h"
#include "ui_widgets.h"

static void response_ok(GtkButton *btn, gpointer dialog) {
    (void) btn;
    gtk_dialog_response(GTK_DIALOG(dialog), GTK_RESPONSE_OK);
}
static void response_cancel(GtkButton *btn, gpointer dialog) {
    (void) btn;
    gtk_dialog_response(GTK_DIALOG(dialog), GTK_RESPONSE_CANCEL);
}

void ui_edit_saved_link_dialog_run(GtkWindow *parent, AppState *state, GtkWidget *toast_host,
                                    gint64 link_id, const gchar *current_url, GtkWidget *saved_links_view) {
    GtkWidget *dialog = gtk_dialog_new_with_buttons(
        "Edit link", parent, GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT, NULL, NULL);
    gtk_window_set_default_size(GTK_WINDOW(dialog), 420, 260);
    gtk_widget_set_size_request(dialog, 340, 220);
    gtk_window_set_position(GTK_WINDOW(dialog), GTK_WIN_POS_CENTER_ON_PARENT);

    GtkWidget *content = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    gtk_style_context_add_class(gtk_widget_get_style_context(content), "content-pad-20");

    GtkWidget *title_label = ui_title_label_new("Edit link");
    gtk_widget_set_margin_bottom(title_label, 16);
    gtk_box_pack_start(GTK_BOX(content), title_label, FALSE, FALSE, 0);

    gtk_box_pack_start(GTK_BOX(content), ui_label_label_new("Link"), FALSE, FALSE, 0);
    GtkWidget *textview_widget = NULL;
    GtkWidget *textview_scroller = ui_outlined_textview_new(&textview_widget);
    gtk_widget_set_size_request(textview_scroller, -1, 60);
    gtk_widget_set_vexpand(textview_scroller, TRUE);
    ui_textview_set_text(GTK_TEXT_VIEW(textview_widget), current_url);
    gtk_box_pack_start(GTK_BOX(content), textview_scroller, TRUE, TRUE, 4);

    GtkWidget *button_row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_widget_set_margin_top(button_row, 16);
    GtkWidget *spacer = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_widget_set_hexpand(spacer, TRUE);
    GtkWidget *cancel_btn = ui_text_button_new("Cancel", "text-button-neutral");
    GtkWidget *save_btn = ui_text_button_new("Save", "text-button-primary");
    gtk_box_pack_start(GTK_BOX(button_row), spacer, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(button_row), cancel_btn, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(button_row), save_btn, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(content), button_row, FALSE, FALSE, 0);

    g_signal_connect(save_btn, "clicked", G_CALLBACK(response_ok), dialog);
    g_signal_connect(cancel_btn, "clicked", G_CALLBACK(response_cancel), dialog);
    gtk_widget_set_can_default(save_btn, TRUE);
    gtk_dialog_set_default_response(GTK_DIALOG(dialog), GTK_RESPONSE_OK);

    gtk_widget_show_all(dialog);
    gtk_widget_grab_focus(textview_widget);

    gint response = gtk_dialog_run(GTK_DIALOG(dialog));
    if (response == GTK_RESPONSE_OK) {
        gchar *new_url = ui_textview_get_text(GTK_TEXT_VIEW(textview_widget));
        g_strstrip(new_url);
        SavedLinkEntity *entity = linker_data_find_saved_link_by_id(state->data, link_id);
        if (entity && *new_url) {
            g_free(entity->url);
            entity->url = g_strdup(new_url);
            app_state_save(state, toast_host);
        }
        g_free(new_url);
    }

    gtk_widget_destroy(dialog);
    ui_saved_links_view_refresh(saved_links_view);
}
