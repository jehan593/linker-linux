#include "ui_main_window.h"
#include "ui_browsers_view.h"
#include "ui_saved_links_view.h"
#include "ui_settings_dialog.h"
#include "ui_widgets.h"
#include "xdg_default.h"
#include "toast.h"

static void update_banner_visibility(GtkWidget *window) {
    GtkWidget *banner = g_object_get_data(G_OBJECT(window), "banner");
    gtk_widget_set_visible(banner, !xdg_default_is_default());
}

static void on_settings_clicked(GtkButton *btn, gpointer user_data) {
    (void) btn;
    ui_settings_dialog_run(GTK_WINDOW(user_data));
}

static void on_request_default_clicked(GtkButton *btn, gpointer user_data) {
    (void) btn;
    GtkWidget *window = GTK_WIDGET(user_data);
    GtkWidget *toast_host = g_object_get_data(G_OBJECT(window), "toast-host");
    if (xdg_default_set_as_default()) {
        toast_host_show(toast_host, "Linker is now your default browser");
    } else {
        toast_host_show(toast_host, "Couldn't set Linker as default browser");
    }
    update_banner_visibility(window);
}

static GtkWidget *build_banner(GtkWidget *window) {
    GtkWidget *banner = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
    gtk_style_context_add_class(gtk_widget_get_style_context(banner), "banner");
    gtk_widget_set_margin_start(banner, 16);
    gtk_widget_set_margin_end(banner, 16);
    gtk_widget_set_margin_top(banner, 16);
    gtk_widget_set_margin_bottom(banner, 16);

    GtkWidget *title = gtk_label_new("Linker isn't your default browser yet");
    gtk_style_context_add_class(gtk_widget_get_style_context(title), "banner-title");
    gtk_label_set_xalign(GTK_LABEL(title), 0.0);
    gtk_label_set_line_wrap(GTK_LABEL(title), TRUE);

    GtkWidget *body = gtk_label_new("Set it as default so links you open anywhere go through this chooser first.");
    gtk_style_context_add_class(gtk_widget_get_style_context(body), "banner-body");
    gtk_label_set_xalign(GTK_LABEL(body), 0.0);
    gtk_label_set_line_wrap(GTK_LABEL(body), TRUE);

    GtkWidget *btn = ui_pill_button_new("Set as default browser");
    gtk_widget_set_halign(btn, GTK_ALIGN_START);
    gtk_widget_set_margin_top(btn, 4);

    gtk_box_pack_start(GTK_BOX(banner), title, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(banner), body, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(banner), btn, FALSE, FALSE, 0);

    g_signal_connect(btn, "clicked", G_CALLBACK(on_request_default_clicked), window);
    return banner;
}

static void select_tab(GtkWidget *window, gboolean browsers_selected);

static void on_browsers_tab_toggled(GtkToggleButton *btn, gpointer user_data) {
    if (gtk_toggle_button_get_active(btn)) select_tab(GTK_WIDGET(user_data), TRUE);
    else gtk_toggle_button_set_active(btn, TRUE);
}

static void on_saved_tab_toggled(GtkToggleButton *btn, gpointer user_data) {
    if (gtk_toggle_button_get_active(btn)) select_tab(GTK_WIDGET(user_data), FALSE);
    else gtk_toggle_button_set_active(btn, TRUE);
}

static void select_tab(GtkWidget *window, gboolean browsers_selected) {
    GtkWidget *browsers_tab = g_object_get_data(G_OBJECT(window), "browsers-tab");
    GtkWidget *saved_tab = g_object_get_data(G_OBJECT(window), "saved-tab");
    GtkWidget *browsers_view = g_object_get_data(G_OBJECT(window), "browsers-view");
    GtkWidget *saved_view = g_object_get_data(G_OBJECT(window), "saved-view");

    g_signal_handlers_block_by_func(browsers_tab, on_browsers_tab_toggled, window);
    g_signal_handlers_block_by_func(saved_tab, on_saved_tab_toggled, window);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(browsers_tab), browsers_selected);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(saved_tab), !browsers_selected);
    g_signal_handlers_unblock_by_func(browsers_tab, on_browsers_tab_toggled, window);
    g_signal_handlers_unblock_by_func(saved_tab, on_saved_tab_toggled, window);

    gtk_widget_set_visible(browsers_view, browsers_selected);
    gtk_widget_set_visible(saved_view, !browsers_selected);
    if (!browsers_selected) ui_saved_links_view_refresh(saved_view);
}

static gboolean on_window_focus_in(GtkWidget *window, GdkEventFocus *event, gpointer user_data) {
    (void) event;
    (void) user_data;
    update_banner_visibility(window);
    ui_browsers_view_refresh(g_object_get_data(G_OBJECT(window), "browsers-view"));
    ui_saved_links_view_refresh(g_object_get_data(G_OBJECT(window), "saved-view"));
    return FALSE;
}

GtkWidget *ui_main_window_show(GtkApplication *app, AppState *state) {
    if (state->main_window) {
        gtk_window_present(GTK_WINDOW(state->main_window));
        return state->main_window;
    }

    GtkWidget *window = gtk_application_window_new(app);
    gtk_window_set_title(GTK_WINDOW(window), "Linker");
    gtk_window_set_default_size(GTK_WINDOW(window), 480, 640);
    gtk_widget_set_size_request(window, 360, 420);
    gtk_window_set_position(GTK_WINDOW(window), GTK_WIN_POS_CENTER);
    if (state->icon_256_path && g_file_test(state->icon_256_path, G_FILE_TEST_EXISTS)) {
        gtk_window_set_icon_from_file(GTK_WINDOW(window), state->icon_256_path, NULL);
    }

    GtkWidget *root = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_container_add(GTK_CONTAINER(window), root);

    /* top bar: the "top-bar" background class lives on the outer, unmargined box so
     * it paints edge-to-edge; the actual 16/12 inset is margin on an inner content
     * box instead — GTK3 widget margin sits *outside* a widget's own CSS background,
     * so putting both the background class and the margin on the same widget would
     * shrink the colored band away from the window edges instead of insetting content. */
    GtkWidget *top_bar = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_style_context_add_class(gtk_widget_get_style_context(top_bar), "top-bar");

    GtkWidget *top_bar_inner = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    gtk_widget_set_margin_start(top_bar_inner, 16);
    gtk_widget_set_margin_end(top_bar_inner, 16);
    gtk_widget_set_margin_top(top_bar_inner, 12);
    gtk_widget_set_margin_bottom(top_bar_inner, 12);
    gtk_widget_set_hexpand(top_bar_inner, TRUE);
    gtk_box_pack_start(GTK_BOX(top_bar), top_bar_inner, TRUE, TRUE, 0);

    GtkWidget *app_icon_img = NULL;
    if (state->icon_256_path && g_file_test(state->icon_256_path, G_FILE_TEST_EXISTS)) {
        GdkPixbuf *pb = gdk_pixbuf_new_from_file_at_scale(state->icon_256_path, 22, 22, TRUE, NULL);
        if (pb) {
            app_icon_img = gtk_image_new_from_pixbuf(pb);
            g_object_unref(pb);
        }
    }
    if (!app_icon_img) app_icon_img = gtk_image_new_from_icon_name("web-browser-symbolic", GTK_ICON_SIZE_MENU);

    GtkWidget *title_label = gtk_label_new("Linker");
    gtk_style_context_add_class(gtk_widget_get_style_context(title_label), "top-bar-title");

    GtkWidget *top_spacer = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_widget_set_hexpand(top_spacer, TRUE);

    GtkWidget *settings_btn = ui_icon_button_new("emblem-system-symbolic", "Notesnook settings", FALSE);

    gtk_box_pack_start(GTK_BOX(top_bar_inner), app_icon_img, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(top_bar_inner), title_label, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(top_bar_inner), top_spacer, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(top_bar_inner), settings_btn, FALSE, FALSE, 0);

    GtkWidget *banner = build_banner(window);

    /* tab row */
    GtkWidget *tab_row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_style_context_add_class(gtk_widget_get_style_context(tab_row), "tab-row");
    gtk_widget_set_size_request(tab_row, -1, 44);
    GtkWidget *browsers_tab = gtk_toggle_button_new_with_label("Browsers");
    GtkWidget *saved_tab = gtk_toggle_button_new_with_label("Saved Links");
    gtk_style_context_add_class(gtk_widget_get_style_context(browsers_tab), "flat");
    gtk_style_context_add_class(gtk_widget_get_style_context(browsers_tab), "tab-button");
    gtk_style_context_add_class(gtk_widget_get_style_context(saved_tab), "flat");
    gtk_style_context_add_class(gtk_widget_get_style_context(saved_tab), "tab-button");
    gtk_widget_set_hexpand(browsers_tab, TRUE);
    gtk_widget_set_hexpand(saved_tab, TRUE);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(browsers_tab), TRUE);
    gtk_box_pack_start(GTK_BOX(tab_row), browsers_tab, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(tab_row), saved_tab, TRUE, TRUE, 0);

    /* content */
    GtkWidget *toast_host = toast_host_new();
    GtkWidget *browsers_view = ui_browsers_view_new(state, toast_host);
    GtkWidget *saved_view = ui_saved_links_view_new(state, toast_host);

    GtkWidget *content_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_box_pack_start(GTK_BOX(content_box), browsers_view, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(content_box), saved_view, TRUE, TRUE, 0);

    GtkWidget *overlay = gtk_overlay_new();
    gtk_container_add(GTK_CONTAINER(overlay), content_box);
    gtk_overlay_add_overlay(GTK_OVERLAY(overlay), toast_host);
    gtk_widget_set_vexpand(overlay, TRUE);

    gtk_box_pack_start(GTK_BOX(root), top_bar, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(root), banner, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(root), tab_row, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(root), overlay, TRUE, TRUE, 0);

    g_object_set_data(G_OBJECT(window), "state", state);
    g_object_set_data(G_OBJECT(window), "toast-host", toast_host);
    g_object_set_data(G_OBJECT(window), "banner", banner);
    g_object_set_data(G_OBJECT(window), "browsers-tab", browsers_tab);
    g_object_set_data(G_OBJECT(window), "saved-tab", saved_tab);
    g_object_set_data(G_OBJECT(window), "browsers-view", browsers_view);
    g_object_set_data(G_OBJECT(window), "saved-view", saved_view);

    g_signal_connect(browsers_tab, "toggled", G_CALLBACK(on_browsers_tab_toggled), window);
    g_signal_connect(saved_tab, "toggled", G_CALLBACK(on_saved_tab_toggled), window);
    g_signal_connect(settings_btn, "clicked", G_CALLBACK(on_settings_clicked), window);
    g_signal_connect(window, "focus-in-event", G_CALLBACK(on_window_focus_in), NULL);

    state->main_window = window;
    gtk_widget_show_all(window);
    gtk_widget_set_visible(saved_view, FALSE);
    update_banner_visibility(window);

    return window;
}
