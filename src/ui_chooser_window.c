#include "ui_chooser_window.h"
#include "ui_main_window.h"
#include "ui_widgets.h"
#include "browsers.h"
#include "launcher.h"
#include "icon_resolve.h"
#include "toast.h"
#include "util.h"
#include "window_geometry.h"

/* The URL box always shows exactly this many rows, scrolling past it. */
#define URL_TEXTVIEW_ROWS 5

/* The browser list shows this many rows without scrolling. */
#define BROWSER_ROWS_VISIBLE 5

/* Locks the URL scroller to a fixed pixel height for `rows` lines of rendered text.
 * Must run after realize: that's the first point the font is resolved. */
static void fixate_textview_rows(GtkTextView *view, GtkWidget *scroller, gint rows) {
    PangoFontDescription *font = NULL;
    gtk_style_context_get(gtk_widget_get_style_context(GTK_WIDGET(view)),
                          gtk_widget_get_state_flags(GTK_WIDGET(view)),
                          "font", &font, NULL);
    if (!font) return;

    PangoLayout *layout = pango_layout_new(gtk_widget_get_pango_context(GTK_WIDGET(view)));
    pango_layout_set_font_description(layout, font);
    pango_layout_set_text(layout, "Ag", -1);
    gint line_height = 0;
    pango_layout_get_pixel_size(layout, NULL, &line_height);
    g_object_unref(layout);
    pango_font_description_free(font);
    if (line_height <= 0) return;

    /* +4 covers the outlined 1px borders on both the scroller and the textview. */
    gint vinset = gtk_text_view_get_top_margin(view) + gtk_text_view_get_bottom_margin(view) + 4;
    gtk_widget_set_size_request(scroller, -1, line_height * rows + vinset);
}

static void on_url_textview_realized(GtkWidget *widget, gpointer user_data) {
    fixate_textview_rows(GTK_TEXT_VIEW(widget), GTK_WIDGET(user_data), URL_TEXTVIEW_ROWS);
}

static gchar *current_url_text(GtkWidget *window) {
    GtkWidget *url_textview = g_object_get_data(G_OBJECT(window), "url-textview");
    gchar *url = ui_textview_get_text(GTK_TEXT_VIEW(url_textview));
    g_strstrip(url);
    return url;
}

static void update_host_label(GtkWidget *window) {
    GtkWidget *host_label = g_object_get_data(G_OBJECT(window), "host-label");
    gchar *url = current_url_text(window);
    gchar *host = linker_url_to_host(url);
    gtk_label_set_text(GTK_LABEL(host_label), (host && *host) ? host : "Open link");
    g_free(host);
    g_free(url);
}

static void update_save_icon(GtkWidget *window) {
    AppState *state = g_object_get_data(G_OBJECT(window), "state");
    GtkWidget *save_btn = g_object_get_data(G_OBJECT(window), "save-btn");
    gchar *url = current_url_text(window);
    gboolean saved = *url && linker_data_find_saved_link_by_url(state->data, url) != NULL;
    GtkWidget *image = gtk_image_new_from_icon_name(saved ? "starred-symbolic" : "non-starred-symbolic", GTK_ICON_SIZE_BUTTON);
    gtk_button_set_image(GTK_BUTTON(save_btn), image);
    g_free(url);
}

static void on_url_buffer_changed(GtkTextBuffer *buffer, gpointer user_data) {
    (void) buffer;
    GtkWidget *window = GTK_WIDGET(user_data);
    update_host_label(window);
    update_save_icon(window);
}

static void on_copy_clicked(GtkButton *btn, gpointer user_data) {
    (void) btn;
    GtkWidget *window = GTK_WIDGET(user_data);
    GtkWidget *toast_host = g_object_get_data(G_OBJECT(window), "toast-host");
    gchar *url = current_url_text(window);
    gtk_clipboard_set_text(gtk_clipboard_get(GDK_SELECTION_CLIPBOARD), url, -1);
    toast_host_show(toast_host, "Copied to clipboard");
    g_free(url);
}

static void on_save_clicked(GtkButton *btn, gpointer user_data) {
    (void) btn;
    GtkWidget *window = GTK_WIDGET(user_data);
    AppState *state = g_object_get_data(G_OBJECT(window), "state");
    GtkWidget *toast_host = g_object_get_data(G_OBJECT(window), "toast-host");
    gchar *url = current_url_text(window);
    if (*url) {
        SavedLinkEntity *existing = linker_data_find_saved_link_by_url(state->data, url);
        if (existing) {
            /* saved_links is a free-func array, so removal already frees the entity */
            g_ptr_array_remove(state->data->saved_links, existing);
            toast_host_show(toast_host, "Removed from saved links");
        } else {
            SavedLinkEntity *entity = saved_link_entity_new(state->data->next_saved_link_id++, url, linker_now_millis());
            g_ptr_array_add(state->data->saved_links, entity);
            toast_host_show(toast_host, "Saved");
        }
        app_state_save(state, toast_host);
        update_save_icon(window);
    }
    g_free(url);
}

static gboolean on_browser_row_click(GtkWidget *event_box, GdkEventButton *event, gpointer user_data) {
    (void) event;
    GtkWidget *window = GTK_WIDGET(user_data);
    BrowserListItem *item = g_object_get_data(G_OBJECT(event_box), "browser-item");
    GtkWidget *toast_host = g_object_get_data(G_OBJECT(window), "toast-host");
    gchar *url = current_url_text(window);

    GError *error = NULL;
    if (!launcher_open_browser(item, url, &error)) {
        g_warning("could not open browser: %s", error ? error->message : "unknown error");
        toast_host_show(toast_host, "Couldn't open that link");
        g_clear_error(&error);
    }
    g_free(url);
    gtk_widget_destroy(window);
    return TRUE;
}

static gboolean on_browser_row_enter(GtkWidget *event_box, GdkEventCrossing *event, gpointer user_data) {
    (void) event;
    (void) user_data;
    gtk_widget_set_state_flags(event_box, GTK_STATE_FLAG_PRELIGHT, FALSE);
    return FALSE;
}

static gboolean on_browser_row_leave(GtkWidget *event_box, GdkEventCrossing *event, gpointer user_data) {
    (void) event;
    (void) user_data;
    gtk_widget_unset_state_flags(event_box, GTK_STATE_FLAG_PRELIGHT);
    return FALSE;
}

static void on_manage_clicked(GtkButton *btn, gpointer user_data) {
    (void) btn;
    GtkWidget *window = GTK_WIDGET(user_data);
    AppState *state = g_object_get_data(G_OBJECT(window), "state");
    GtkApplication *app = g_object_get_data(G_OBJECT(window), "app");
    ui_main_window_show(app, state);
    gtk_widget_destroy(window);
}

static void on_cancel_clicked(GtkButton *btn, gpointer user_data) {
    (void) btn;
    gtk_widget_destroy(GTK_WIDGET(user_data));
}

static GtkWidget *build_browser_row(GtkWidget *window, const BrowserListItem *item) {
    GtkWidget *event_box = gtk_event_box_new();
    gtk_style_context_add_class(gtk_widget_get_style_context(event_box), "chooser-row");
    gtk_widget_add_events(event_box, GDK_ENTER_NOTIFY_MASK | GDK_LEAVE_NOTIFY_MASK);
    g_signal_connect(event_box, "enter-notify-event", G_CALLBACK(on_browser_row_enter), NULL);
    g_signal_connect(event_box, "leave-notify-event", G_CALLBACK(on_browser_row_leave), NULL);

    GtkWidget *row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    gtk_widget_set_margin_start(row, 12);
    gtk_widget_set_margin_end(row, 12);
    gtk_widget_set_margin_top(row, 8);
    gtk_widget_set_margin_bottom(row, 8);

    GdkPixbuf *pixbuf = icon_resolve_browser_icon(browser_list_item_icon_field(item), 28);
    GtkWidget *icon_img = gtk_image_new_from_pixbuf(pixbuf);
    g_object_unref(pixbuf);

    GtkWidget *label = gtk_label_new(item->display_label);
    gtk_label_set_xalign(GTK_LABEL(label), 0.0);

    gtk_box_pack_start(GTK_BOX(row), icon_img, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(row), label, TRUE, TRUE, 0);
    gtk_container_add(GTK_CONTAINER(event_box), row);

    g_object_set_data(G_OBJECT(event_box), "browser-item", (gpointer) item);
    g_signal_connect(event_box, "button-release-event", G_CALLBACK(on_browser_row_click), window);

    return event_box;
}

GtkWidget *ui_chooser_window_new(GtkApplication *app, AppState *state, const gchar *url) {
    GtkWidget *window = gtk_application_window_new(app);
    gtk_window_set_title(GTK_WINDOW(window), "Linker");
    gtk_widget_set_size_request(window, 380, -1);
    gtk_window_set_default_size(GTK_WINDOW(window), 440, -1);
    window_geometry_apply(GTK_WINDOW(window), "chooser");
    gtk_window_set_position(GTK_WINDOW(window), GTK_WIN_POS_CENTER);
    gtk_window_set_keep_above(GTK_WINDOW(window), TRUE);
    gtk_window_set_skip_taskbar_hint(GTK_WINDOW(window), TRUE);
    gtk_window_set_type_hint(GTK_WINDOW(window), GDK_WINDOW_TYPE_HINT_DIALOG);
    if (state->icon_256_path && g_file_test(state->icon_256_path, G_FILE_TEST_EXISTS)) {
        gtk_window_set_icon_from_file(GTK_WINDOW(window), state->icon_256_path, NULL);
    }

    GtkWidget *root = gtk_box_new(GTK_ORIENTATION_VERTICAL, 12);
    gtk_style_context_add_class(gtk_widget_get_style_context(root), "content-pad-16");
    gtk_container_add(GTK_CONTAINER(window), root);

    /* domain header row */
    GtkWidget *header_row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
    GtkWidget *globe_icon = gtk_image_new_from_icon_name("web-browser-symbolic", GTK_ICON_SIZE_BUTTON);
    gtk_style_context_add_class(gtk_widget_get_style_context(globe_icon), "icon-tint-primary");
    GtkWidget *host_label = gtk_label_new("Open link");
    gtk_style_context_add_class(gtk_widget_get_style_context(host_label), "linker-title");
    gtk_label_set_ellipsize(GTK_LABEL(host_label), PANGO_ELLIPSIZE_END);
    gtk_label_set_xalign(GTK_LABEL(host_label), 0.0);
    gtk_widget_set_hexpand(host_label, TRUE);

    GtkWidget *copy_btn = ui_icon_button_new("edit-copy-symbolic", "Copy link", FALSE);
    GtkWidget *save_btn = ui_icon_button_new("non-starred-symbolic", "Save link", FALSE);

    gtk_box_pack_start(GTK_BOX(header_row), globe_icon, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(header_row), host_label, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(header_row), copy_btn, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(header_row), save_btn, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(root), header_row, FALSE, FALSE, 0);

    /* editable URL box: fixed at URL_TEXTVIEW_ROWS once realized; longer content
     * scrolls inside that fixed height instead of growing the window. */
    GtkWidget *url_textview_widget = NULL;
    GtkWidget *url_scroller = ui_outlined_textview_new(&url_textview_widget);
    gtk_box_pack_start(GTK_BOX(root), url_scroller, FALSE, FALSE, 0);

    gtk_box_pack_start(GTK_BOX(root), ui_hairline_new(), FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(root), ui_label_label_new("Open with"), FALSE, FALSE, 0);

    /* browser list area */
    GtkWidget *list_area = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_widget_set_size_request(list_area, -1, 120);
    gtk_widget_set_vexpand(list_area, TRUE);

    GtkWidget *empty_label = ui_body_small_label_new(
        "No browsers to show — check Manage Browsers.");
    gtk_label_set_line_wrap(GTK_LABEL(empty_label), TRUE);
    gtk_label_set_max_width_chars(GTK_LABEL(empty_label), 46);
    gtk_label_set_justify(GTK_LABEL(empty_label), GTK_JUSTIFY_CENTER);
    gtk_widget_set_halign(empty_label, GTK_ALIGN_CENTER);
    gtk_widget_set_valign(empty_label, GTK_ALIGN_CENTER);
    gtk_widget_set_vexpand(empty_label, TRUE);

    GtkWidget *list_scroller = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(list_scroller), GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
    gtk_scrolled_window_set_propagate_natural_height(GTK_SCROLLED_WINDOW(list_scroller), TRUE);
    GtkWidget *browsers_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_container_add(GTK_CONTAINER(list_scroller), browsers_box);

    gtk_box_pack_start(GTK_BOX(list_area), empty_label, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(list_area), list_scroller, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(root), list_area, TRUE, TRUE, 0);

    /* toast host: a fixed-height row here (not an overlay), unlike MainWindow */
    GtkWidget *toast_host = toast_host_new();
    gtk_widget_set_size_request(toast_host, -1, 34);
    gtk_box_pack_start(GTK_BOX(root), toast_host, FALSE, FALSE, 0);

    /* bottom row: Manage browsers left, Cancel right */
    GtkWidget *bottom_row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);
    GtkWidget *manage_btn = ui_text_button_new("Manage browsers", "text-button-neutral");
    GtkWidget *cancel_btn = ui_text_button_new("Cancel", "text-button-neutral");
    GtkWidget *bottom_spacer = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_widget_set_hexpand(bottom_spacer, TRUE);
    gtk_box_pack_start(GTK_BOX(bottom_row), manage_btn, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(bottom_row), bottom_spacer, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(bottom_row), cancel_btn, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(root), bottom_row, FALSE, FALSE, 0);

    g_object_set_data(G_OBJECT(window), "app", app);
    g_object_set_data(G_OBJECT(window), "state", state);
    g_object_set_data(G_OBJECT(window), "toast-host", toast_host);
    g_object_set_data(G_OBJECT(window), "host-label", host_label);
    g_object_set_data(G_OBJECT(window), "save-btn", save_btn);
    g_object_set_data(G_OBJECT(window), "url-textview", url_textview_widget);

    g_signal_connect(gtk_text_view_get_buffer(GTK_TEXT_VIEW(url_textview_widget)), "changed",
                      G_CALLBACK(on_url_buffer_changed), window);
    g_signal_connect(url_textview_widget, "realize", G_CALLBACK(on_url_textview_realized), url_scroller);
    g_signal_connect(copy_btn, "clicked", G_CALLBACK(on_copy_clicked), window);
    g_signal_connect(save_btn, "clicked", G_CALLBACK(on_save_clicked), window);
    g_signal_connect(manage_btn, "clicked", G_CALLBACK(on_manage_clicked), window);
    g_signal_connect(cancel_btn, "clicked", G_CALLBACK(on_cancel_clicked), window);

    /* setting the initial text fires the buffer's "changed" signal above, which
     * populates the host label and save icon from the real starting URL */
    ui_textview_set_text(GTK_TEXT_VIEW(url_textview_widget), url);

    GPtrArray *visible = browsers_get_visible_list(state->data);
    app_state_save(state, NULL);
    g_object_set_data_full(G_OBJECT(window), "browsers-list", visible, (GDestroyNotify) browsers_free_list);

    if (visible->len == 0) {
        gtk_widget_show(empty_label);
        gtk_widget_hide(list_scroller);
    } else {
        gtk_widget_hide(empty_label);
        gtk_widget_show(list_scroller);
        GtkWidget *first_row = NULL;
        for (guint i = 0; i < visible->len; i++) {
            BrowserListItem *item = g_ptr_array_index(visible, i);
            GtkWidget *row = build_browser_row(window, item);
            if (!first_row) first_row = row;
            gtk_box_pack_start(GTK_BOX(browsers_box), row, FALSE, FALSE, 0);
        }
        /* Fit exactly BROWSER_ROWS_VISIBLE rows with no scrollbar: measure the first
         * real row instead of guessing. +2 slack avoids off-by-one scroll. */
        gint row_min = 0, row_nat = 0;
        gtk_widget_get_preferred_height(first_row, &row_min, &row_nat);
        gint row_h = MAX(row_min, row_nat);
        /* +2 slack: GTK treats content exactly at max_content_height as scrolling. */
        gtk_scrolled_window_set_max_content_height(GTK_SCROLLED_WINDOW(list_scroller),
                                                  row_h * BROWSER_ROWS_VISIBLE + 2);
    }

    gtk_widget_show_all(window);
    gtk_widget_set_visible(empty_label, visible->len == 0);
    gtk_widget_set_visible(list_scroller, visible->len != 0);
    gtk_widget_grab_focus(url_textview_widget);

    return window;
}
