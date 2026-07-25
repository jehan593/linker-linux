#include "ui_browsers_view.h"
#include "ui_edit_browser_dialog.h"
#include "browsers.h"
#include "icon_resolve.h"
#include "ui_widgets.h"
#include <string.h>

static gboolean refresh_idle_cb(gpointer view) {
    ui_browsers_view_refresh(GTK_WIDGET(view));
    return G_SOURCE_REMOVE;
}

void ui_browsers_view_refresh_deferred(GtkWidget *view) {
    g_idle_add(refresh_idle_cb, view);
}

static GtkWidget *build_empty_state(void) {
    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
    gtk_widget_set_halign(box, GTK_ALIGN_CENTER);
    gtk_widget_set_valign(box, GTK_ALIGN_CENTER);
    gtk_widget_set_vexpand(box, TRUE);
    gtk_widget_set_margin_top(box, 24);
    gtk_widget_set_margin_bottom(box, 24);

    GtkWidget *icon = gtk_image_new_from_icon_name("web-browser-symbolic", GTK_ICON_SIZE_DIALOG);
    gtk_image_set_pixel_size(GTK_IMAGE(icon), 32);
    GtkWidget *label = ui_body_small_label_new("No browsers found on this device.");
    gtk_label_set_justify(GTK_LABEL(label), GTK_JUSTIFY_CENTER);
    gtk_label_set_xalign(GTK_LABEL(label), 0.5);

    gtk_box_pack_start(GTK_BOX(box), icon, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(box), label, FALSE, FALSE, 0);
    return box;
}

static void move_row(GtkWidget *row, gint direction) {
    AppState *state = g_object_get_data(G_OBJECT(row), "state");
    GtkWidget *browsers_view = g_object_get_data(G_OBJECT(row), "browsers-view");
    GtkWidget *toast_host = g_object_get_data(G_OBJECT(row), "toast-host");
    const gchar *id = g_object_get_data(G_OBJECT(row), "browser-id");

    GPtrArray *list = browsers_get_manage_list(state->data);
    gint idx = -1;
    for (guint i = 0; i < list->len; i++) {
        BrowserListItem *it = g_ptr_array_index(list, i);
        if (g_strcmp0(it->id, id) == 0) {
            idx = (gint) i;
            break;
        }
    }
    gint other = idx + direction;
    if (idx >= 0 && other >= 0 && (guint) other < list->len) {
        BrowserListItem *a = g_ptr_array_index(list, idx);
        BrowserListItem *b = g_ptr_array_index(list, other);
        BrowserPrefEntity *pa = linker_data_find_browser_pref(state->data, a->id);
        BrowserPrefEntity *pb = linker_data_find_browser_pref(state->data, b->id);
        if (pa && pb) {
            gint tmp = pa->order_index;
            pa->order_index = pb->order_index;
            pb->order_index = tmp;
            app_state_save(state, toast_host);
        }
    }
    browsers_free_list(list);
    ui_browsers_view_refresh_deferred(browsers_view);
}

static void on_move_up_clicked(GtkButton *btn, gpointer user_data) {
    (void) btn;
    move_row(GTK_WIDGET(user_data), -1);
}

static void on_move_down_clicked(GtkButton *btn, gpointer user_data) {
    (void) btn;
    move_row(GTK_WIDGET(user_data), 1);
}

static gboolean on_hide_toggle(GtkSwitch *sw, gboolean new_state, gpointer user_data) {
    GtkWidget *row = GTK_WIDGET(user_data);
    AppState *state = g_object_get_data(G_OBJECT(row), "state");
    GtkWidget *browsers_view = g_object_get_data(G_OBJECT(row), "browsers-view");
    GtkWidget *toast_host = g_object_get_data(G_OBJECT(row), "toast-host");
    const gchar *id = g_object_get_data(G_OBJECT(row), "browser-id");

    BrowserPrefEntity *pref = linker_data_find_browser_pref(state->data, id);
    if (pref) {
        pref->hidden = !new_state;
        app_state_save(state, toast_host);
    }
    gtk_switch_set_state(sw, new_state);
    ui_browsers_view_refresh_deferred(browsers_view);
    return TRUE;
}

static void on_edit_clicked(GtkButton *btn, gpointer user_data) {
    (void) btn;
    GtkWidget *row = GTK_WIDGET(user_data);
    AppState *state = g_object_get_data(G_OBJECT(row), "state");
    GtkWidget *browsers_view = g_object_get_data(G_OBJECT(row), "browsers-view");
    GtkWidget *toast_host = g_object_get_data(G_OBJECT(row), "toast-host");
    const gchar *id = g_object_get_data(G_OBJECT(row), "browser-id");

    GPtrArray *list = browsers_get_manage_list(state->data);
    BrowserListItem *found = NULL;
    for (guint i = 0; i < list->len; i++) {
        BrowserListItem *it = g_ptr_array_index(list, i);
        if (g_strcmp0(it->id, id) == 0) {
            found = it;
            break;
        }
    }
    if (found) {
        GtkWidget *toplevel = gtk_widget_get_toplevel(row);
        ui_edit_browser_dialog_run(GTK_WINDOW(toplevel), state, toast_host, found, browsers_view);
    }
    browsers_free_list(list);
}

static GtkWidget *build_browser_row(AppState *state, GtkWidget *toast_host, GtkWidget *browsers_view,
                                     const BrowserListItem *item, guint index, guint total) {
    GtkWidget *row_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
    gtk_widget_set_margin_start(row_box, 16);
    gtk_widget_set_margin_end(row_box, 20);
    gtk_widget_set_margin_top(row_box, 10);
    gtk_widget_set_margin_bottom(row_box, 10);
    gtk_style_context_add_class(gtk_widget_get_style_context(row_box), "row-hairline");

    GtkWidget *move_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    GtkWidget *up_btn = ui_icon_button_new("go-up-symbolic", "Move up", TRUE);
    GtkWidget *down_btn = ui_icon_button_new("go-down-symbolic", "Move down", TRUE);
    gtk_widget_set_sensitive(up_btn, index > 0);
    gtk_widget_set_sensitive(down_btn, index + 1 < total);
    gtk_box_pack_start(GTK_BOX(move_box), up_btn, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(move_box), down_btn, FALSE, FALSE, 0);

    GdkPixbuf *pixbuf = icon_resolve_browser_icon(browser_list_item_icon_field(item), 30);
    GtkWidget *icon_img = gtk_image_new_from_pixbuf(pixbuf);
    g_object_unref(pixbuf);

    GtkWidget *label_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 2);
    gtk_widget_set_hexpand(label_box, TRUE);
    gtk_widget_set_valign(label_box, GTK_ALIGN_CENTER);
    GtkWidget *display_label = gtk_label_new(NULL);
    gtk_label_set_xalign(GTK_LABEL(display_label), 0.0);
    if (item->hidden) {
        gchar *markup = g_markup_printf_escaped("<s>%s</s>", item->display_label);
        gtk_label_set_markup(GTK_LABEL(display_label), markup);
        g_free(markup);
    } else {
        gtk_label_set_text(GTK_LABEL(display_label), item->display_label);
    }
    gtk_box_pack_start(GTK_BOX(label_box), display_label, FALSE, FALSE, 0);
    if (item->system_label && *item->system_label && g_strcmp0(item->display_label, item->system_label) != 0) {
        gtk_box_pack_start(GTK_BOX(label_box), ui_body_small_label_new(item->system_label), FALSE, FALSE, 0);
    }

    GtkWidget *edit_btn = ui_icon_button_new("document-edit-symbolic", "Edit browser", FALSE);

    GtkWidget *switch_widget = gtk_switch_new();
    gtk_switch_set_active(GTK_SWITCH(switch_widget), !item->hidden);
    gtk_switch_set_state(GTK_SWITCH(switch_widget), !item->hidden);
    gtk_widget_set_valign(switch_widget, GTK_ALIGN_CENTER);

    gtk_box_pack_start(GTK_BOX(row_box), move_box, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(row_box), icon_img, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(row_box), label_box, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(row_box), edit_btn, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(row_box), switch_widget, FALSE, FALSE, 0);

    GtkWidget *row = gtk_list_box_row_new();
    gtk_container_add(GTK_CONTAINER(row), row_box);

    g_object_set_data_full(G_OBJECT(row), "browser-id", g_strdup(item->id), g_free);
    g_object_set_data(G_OBJECT(row), "browsers-view", browsers_view);
    g_object_set_data(G_OBJECT(row), "state", state);
    g_object_set_data(G_OBJECT(row), "toast-host", toast_host);

    g_signal_connect(up_btn, "clicked", G_CALLBACK(on_move_up_clicked), row);
    g_signal_connect(down_btn, "clicked", G_CALLBACK(on_move_down_clicked), row);
    g_signal_connect(edit_btn, "clicked", G_CALLBACK(on_edit_clicked), row);
    g_signal_connect(switch_widget, "state-set", G_CALLBACK(on_hide_toggle), row);

    return row;
}

void ui_browsers_view_refresh(GtkWidget *view) {
    AppState *state = g_object_get_data(G_OBJECT(view), "state");
    GtkWidget *toast_host = g_object_get_data(G_OBJECT(view), "toast-host");
    GtkWidget *list_box = g_object_get_data(G_OBJECT(view), "list-box");
    GtkWidget *empty_state = g_object_get_data(G_OBJECT(view), "empty-state");
    GtkWidget *scroller = g_object_get_data(G_OBJECT(view), "scroller");

    GList *children = gtk_container_get_children(GTK_CONTAINER(list_box));
    for (GList *l = children; l; l = l->next) gtk_widget_destroy(GTK_WIDGET(l->data));
    g_list_free(children);

    GPtrArray *list = browsers_get_manage_list(state->data);
    app_state_save(state, NULL); /* persist any newly-materialized order_index, silently */

    if (list->len == 0) {
        gtk_widget_show(empty_state);
        gtk_widget_hide(scroller);
    } else {
        gtk_widget_hide(empty_state);
        gtk_widget_show(scroller);
        for (guint i = 0; i < list->len; i++) {
            BrowserListItem *item = g_ptr_array_index(list, i);
            GtkWidget *row = build_browser_row(state, toast_host, view, item, i, list->len);
            gtk_list_box_insert(GTK_LIST_BOX(list_box), row, -1);
        }
        gtk_widget_show_all(list_box);
    }
    browsers_free_list(list);
}

static void on_add_clicked(GtkButton *btn, gpointer user_data) {
    (void) btn;
    GtkWidget *view = GTK_WIDGET(user_data);
    AppState *state = g_object_get_data(G_OBJECT(view), "state");
    GtkWidget *toast_host = g_object_get_data(G_OBJECT(view), "toast-host");
    GtkWidget *toplevel = gtk_widget_get_toplevel(view);
    ui_edit_browser_dialog_run(GTK_WINDOW(toplevel), state, toast_host, NULL, view);
}

static void on_refresh_clicked(GtkButton *btn, gpointer user_data) {
    (void) btn;
    ui_browsers_view_refresh(GTK_WIDGET(user_data));
}

GtkWidget *ui_browsers_view_new(AppState *state, GtkWidget *toast_host) {
    GtkWidget *root = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);

    GtkWidget *header = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);
    gtk_widget_set_margin_start(header, 16);
    gtk_widget_set_margin_end(header, 16);
    gtk_widget_set_margin_top(header, 4);
    gtk_widget_set_margin_bottom(header, 4);
    GtkWidget *spacer = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_widget_set_hexpand(spacer, TRUE);
    GtkWidget *add_btn = ui_icon_button_new("list-add-symbolic", "Add browser", FALSE);
    GtkWidget *refresh_btn = ui_icon_button_new("view-refresh-symbolic", "Refresh browser list", FALSE);
    gtk_box_pack_start(GTK_BOX(header), spacer, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(header), add_btn, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(header), refresh_btn, FALSE, FALSE, 0);

    GtkWidget *empty_state = build_empty_state();

    GtkWidget *scroller = gtk_scrolled_window_new(NULL, NULL);
    gtk_widget_set_vexpand(scroller, TRUE);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroller), GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
    GtkWidget *list_box = gtk_list_box_new();
    gtk_list_box_set_selection_mode(GTK_LIST_BOX(list_box), GTK_SELECTION_NONE);
    gtk_container_add(GTK_CONTAINER(scroller), list_box);

    gtk_box_pack_start(GTK_BOX(root), header, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(root), empty_state, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(root), scroller, TRUE, TRUE, 0);

    g_object_set_data(G_OBJECT(root), "state", state);
    g_object_set_data(G_OBJECT(root), "toast-host", toast_host);
    g_object_set_data(G_OBJECT(root), "list-box", list_box);
    g_object_set_data(G_OBJECT(root), "empty-state", empty_state);
    g_object_set_data(G_OBJECT(root), "scroller", scroller);

    g_signal_connect(add_btn, "clicked", G_CALLBACK(on_add_clicked), root);
    g_signal_connect(refresh_btn, "clicked", G_CALLBACK(on_refresh_clicked), root);

    gtk_widget_show_all(root);
    ui_browsers_view_refresh(root);
    return root;
}
