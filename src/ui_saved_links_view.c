#include "ui_saved_links_view.h"
#include "ui_edit_saved_link_dialog.h"
#include "ui_widgets.h"
#include "theme.h"
#include "toast.h"
#include "util.h"
#include <gio/gio.h>
#include <string.h>

static gboolean refresh_idle_cb(gpointer view) {
    ui_saved_links_view_refresh(GTK_WIDGET(view));
    return G_SOURCE_REMOVE;
}

void ui_saved_links_view_refresh_deferred(GtkWidget *view) {
    g_idle_add(refresh_idle_cb, view);
}

static gchar *build_highlighted_markup(const gchar *url, const gchar *query) {
    if (!query || !*query) return g_markup_escape_text(url, -1);

    gchar *url_lower = g_utf8_strdown(url, -1);
    gchar *query_lower = g_utf8_strdown(query, -1);
    gsize query_len = strlen(query_lower);
    if (query_len == 0) {
        g_free(url_lower);
        g_free(query_lower);
        return g_markup_escape_text(url, -1);
    }

    GString *out = g_string_new(NULL);
    const gchar *cursor = url;
    const gchar *lower_cursor = url_lower;
    while (*cursor) {
        gchar *match = strstr(lower_cursor, query_lower);
        if (!match) {
            gchar *escaped = g_markup_escape_text(cursor, -1);
            g_string_append(out, escaped);
            g_free(escaped);
            break;
        }
        gsize offset = (gsize) (match - lower_cursor);
        if (offset > 0) {
            gchar *pre = g_strndup(cursor, offset);
            gchar *escaped = g_markup_escape_text(pre, -1);
            g_string_append(out, escaped);
            g_free(escaped);
            g_free(pre);
        }
        gchar *matched_text = g_strndup(cursor + offset, query_len);
        gchar *escaped_match = g_markup_escape_text(matched_text, -1);
        g_string_append_printf(out, "<span background=\"%s\" foreground=\"%s\" weight=\"bold\">%s</span>",
                                LINKER_HIGHLIGHT_BG, LINKER_HIGHLIGHT_FG, escaped_match);
        g_free(escaped_match);
        g_free(matched_text);
        cursor += offset + query_len;
        lower_cursor += offset + query_len;
    }

    g_free(url_lower);
    g_free(query_lower);
    return g_string_free(out, FALSE);
}

static GtkWidget *build_empty_state(void) {
    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
    gtk_widget_set_halign(box, GTK_ALIGN_CENTER);
    gtk_widget_set_valign(box, GTK_ALIGN_CENTER);
    gtk_widget_set_vexpand(box, TRUE);
    gtk_widget_set_margin_top(box, 24);
    gtk_widget_set_margin_bottom(box, 24);

    GtkWidget *label = ui_body_small_label_new(
        "No saved links yet.\nSave a link from the browser chooser.");
    g_object_set_data(G_OBJECT(box), "label", label);
    gtk_label_set_line_wrap(GTK_LABEL(label), TRUE);
    gtk_label_set_justify(GTK_LABEL(label), GTK_JUSTIFY_CENTER);
    gtk_label_set_xalign(GTK_LABEL(label), 0.5);
    gtk_widget_set_size_request(label, 260, -1);

    gtk_box_pack_start(GTK_BOX(box), label, FALSE, FALSE, 0);
    return box;
}

static GtkWidget *build_day_header(const gchar *text) {
    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_style_context_add_class(gtk_widget_get_style_context(box), "day-header");
    GtkWidget *label = ui_label_label_new(text);
    gtk_widget_set_margin_start(label, 16);
    gtk_widget_set_margin_end(label, 16);
    gtk_widget_set_margin_top(label, 6);
    gtk_widget_set_margin_bottom(label, 6);
    gtk_box_pack_start(GTK_BOX(box), label, FALSE, FALSE, 0);
    return box;
}

static void on_edit_clicked(GtkButton *btn, gpointer user_data) {
    (void) btn;
    GtkWidget *row = GTK_WIDGET(user_data);
    AppState *state = g_object_get_data(G_OBJECT(row), "state");
    GtkWidget *view = g_object_get_data(G_OBJECT(row), "saved-links-view");
    gint64 *id_box = g_object_get_data(G_OBJECT(row), "link-id");
    const gchar *url = g_object_get_data(G_OBJECT(row), "link-url");
    GtkWidget *toplevel = gtk_widget_get_toplevel(row);
    ui_edit_saved_link_dialog_run(GTK_WINDOW(toplevel), state, *id_box, url, view);
}

static void on_copy_clicked(GtkButton *btn, gpointer user_data) {
    (void) btn;
    GtkWidget *row = GTK_WIDGET(user_data);
    GtkWidget *toast_host = g_object_get_data(G_OBJECT(row), "toast-host");
    const gchar *url = g_object_get_data(G_OBJECT(row), "link-url");
    gtk_clipboard_set_text(gtk_clipboard_get(GDK_SELECTION_CLIPBOARD), url, -1);
    toast_host_show(toast_host, "Copied to clipboard");
}

static void open_link_from_row(GtkWidget *row) {
    GtkWidget *toast_host = g_object_get_data(G_OBJECT(row), "toast-host");
    const gchar *url = g_object_get_data(G_OBJECT(row), "link-url");
    GError *error = NULL;
    if (!g_app_info_launch_default_for_uri(url, NULL, &error)) {
        toast_host_show(toast_host, "Couldn't open that link");
        g_clear_error(&error);
    }
}

static void on_url_clicked(GtkButton *button, gpointer user_data) {
    (void) button;
    open_link_from_row(GTK_WIDGET(user_data));
}

static gboolean on_url_enter(GtkWidget *widget, GdkEventCrossing *event, gpointer user_data) {
    (void) event;
    (void) user_data;
    GdkCursor *cursor = gdk_cursor_new_for_display(gtk_widget_get_display(widget), GDK_HAND2);
    gdk_window_set_cursor(gtk_widget_get_window(widget), cursor);
    if (cursor) g_object_unref(cursor);
    return FALSE;
}

static gboolean on_url_leave(GtkWidget *widget, GdkEventCrossing *event, gpointer user_data) {
    (void) event;
    (void) user_data;
    gdk_window_set_cursor(gtk_widget_get_window(widget), NULL);
    return FALSE;
}

static void on_delete_clicked(GtkButton *btn, gpointer user_data) {
    (void) btn;
    GtkWidget *row = GTK_WIDGET(user_data);
    AppState *state = g_object_get_data(G_OBJECT(row), "state");
    GtkWidget *toast_host = g_object_get_data(G_OBJECT(row), "toast-host");
    GtkWidget *view = g_object_get_data(G_OBJECT(row), "saved-links-view");
    gint64 *id_box = g_object_get_data(G_OBJECT(row), "link-id");

    if (!ui_confirm_delete(GTK_WINDOW(gtk_widget_get_toplevel(row)), "Delete link?",
                           "This permanently removes the link from your saved links.")) return;

    for (guint i = 0; i < state->data->saved_links->len; i++) {
        SavedLinkEntity *e = g_ptr_array_index(state->data->saved_links, i);
        if (e->id == *id_box) {
            g_ptr_array_remove_index(state->data->saved_links, i);
            break;
        }
    }
    app_state_save(state, toast_host);
    ui_saved_links_view_refresh_deferred(view);
}

static GtkWidget *build_link_row(AppState *state, GtkWidget *toast_host, GtkWidget *view,
                                  const SavedLinkEntity *entity, const gchar *query) {
    GtkWidget *row = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);
    gtk_style_context_add_class(gtk_widget_get_style_context(row), "card");
    gtk_style_context_add_class(gtk_widget_get_style_context(row), "content-pad-12");
    gtk_widget_set_margin_start(row, 16);
    gtk_widget_set_margin_end(row, 16);
    gtk_widget_set_margin_top(row, 12);
    gtk_widget_set_margin_bottom(row, 0);

    gchar *markup = build_highlighted_markup(entity->url, query);
    GtkWidget *url_label = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(url_label), markup);
    g_free(markup);
    gtk_label_set_line_wrap(GTK_LABEL(url_label), TRUE);
    gtk_label_set_line_wrap_mode(GTK_LABEL(url_label), PANGO_WRAP_WORD_CHAR);
    gtk_label_set_max_width_chars(GTK_LABEL(url_label), 1);
    gtk_label_set_xalign(GTK_LABEL(url_label), 0.0);
    gtk_style_context_add_class(gtk_widget_get_style_context(url_label), "linker-url-text");

    GtkWidget *url_event = gtk_button_new();
    gtk_widget_set_can_focus(url_event, FALSE);
    gtk_widget_add_events(url_event, GDK_ENTER_NOTIFY_MASK | GDK_LEAVE_NOTIFY_MASK);
    gtk_style_context_add_class(gtk_widget_get_style_context(url_event), "url-link");
    gtk_widget_set_tooltip_text(url_event, entity->url);
    atk_object_set_name(gtk_widget_get_accessible(url_event), entity->url);
    gtk_container_add(GTK_CONTAINER(url_event), url_label);

    gchar *time_str = linker_format_time_of_day(entity->saved_at_millis);
    GtkWidget *time_label = ui_body_small_label_new(time_str);
    g_free(time_str);

    GtkWidget *action_row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 2);
    gtk_widget_set_halign(action_row, GTK_ALIGN_END);
    GtkWidget *edit_btn = ui_icon_button_new("document-edit-symbolic", "Edit", TRUE);
    GtkWidget *copy_btn = ui_icon_button_new("edit-copy-symbolic", "Copy", TRUE);
    GtkWidget *delete_btn = ui_icon_button_new("user-trash-symbolic", "Delete", TRUE);
    gtk_style_context_add_class(gtk_widget_get_style_context(delete_btn), "icon-tint-error");
    gtk_box_pack_start(GTK_BOX(action_row), edit_btn, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(action_row), copy_btn, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(action_row), delete_btn, FALSE, FALSE, 0);

    gtk_box_pack_start(GTK_BOX(row), url_event, FALSE, FALSE, 0);
    GtkWidget *footer = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
    gtk_box_pack_start(GTK_BOX(footer), time_label, TRUE, TRUE, 0);
    gtk_box_pack_end(GTK_BOX(footer), action_row, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(row), footer, FALSE, FALSE, 0);

    gint64 *id_box = g_new(gint64, 1);
    *id_box = entity->id;
    g_object_set_data_full(G_OBJECT(row), "link-id", id_box, g_free);
    g_object_set_data_full(G_OBJECT(row), "link-url", g_strdup(entity->url), g_free);
    g_object_set_data(G_OBJECT(row), "state", state);
    g_object_set_data(G_OBJECT(row), "toast-host", toast_host);
    g_object_set_data(G_OBJECT(row), "saved-links-view", view);

    g_signal_connect(url_event, "clicked", G_CALLBACK(on_url_clicked), row);
    g_signal_connect(url_event, "enter-notify-event", G_CALLBACK(on_url_enter), NULL);
    g_signal_connect(url_event, "leave-notify-event", G_CALLBACK(on_url_leave), NULL);
    g_signal_connect(edit_btn, "clicked", G_CALLBACK(on_edit_clicked), row);
    g_signal_connect(copy_btn, "clicked", G_CALLBACK(on_copy_clicked), row);
    g_signal_connect(delete_btn, "clicked", G_CALLBACK(on_delete_clicked), row);

    return row;
}

static gint compare_saved_desc(gconstpointer a, gconstpointer b) {
    const SavedLinkEntity *ea = *(const SavedLinkEntity **) a;
    const SavedLinkEntity *eb = *(const SavedLinkEntity **) b;
    if (ea->saved_at_millis > eb->saved_at_millis) return -1;
    if (ea->saved_at_millis < eb->saved_at_millis) return 1;
    return 0;
}

void ui_saved_links_view_refresh(GtkWidget *view) {
    AppState *state = g_object_get_data(G_OBJECT(view), "state");
    GtkWidget *toast_host = g_object_get_data(G_OBJECT(view), "toast-host");
    GtkWidget *search_entry = g_object_get_data(G_OBJECT(view), "search-entry");
    GtkWidget *rows_container = g_object_get_data(G_OBJECT(view), "rows-container");
    GtkWidget *empty_state = g_object_get_data(G_OBJECT(view), "empty-state");
    GtkWidget *scroller = g_object_get_data(G_OBJECT(view), "scroller");

    gchar *query = g_strdup(gtk_entry_get_text(GTK_ENTRY(search_entry)));
    g_strstrip(query);

    GList *children = gtk_container_get_children(GTK_CONTAINER(rows_container));
    for (GList *l = children; l; l = l->next) gtk_widget_destroy(GTK_WIDGET(l->data));
    g_list_free(children);

    GPtrArray *filtered = g_ptr_array_new();
    for (guint i = 0; i < state->data->saved_links->len; i++) {
        SavedLinkEntity *e = g_ptr_array_index(state->data->saved_links, i);
        gboolean matches = TRUE;
        if (*query) {
            gchar *url_lower = g_utf8_strdown(e->url, -1);
            gchar *query_lower = g_utf8_strdown(query, -1);
            matches = strstr(url_lower, query_lower) != NULL;
            g_free(url_lower);
            g_free(query_lower);
        }
        if (matches) g_ptr_array_add(filtered, e);
    }
    g_ptr_array_sort(filtered, compare_saved_desc);

    if (filtered->len == 0) {
        gtk_label_set_text(GTK_LABEL(g_object_get_data(G_OBJECT(empty_state), "label")),
            *query ? "No matching links." : "No saved links yet.\nSave a link from the browser chooser.");
        gtk_widget_show(empty_state);
        gtk_widget_hide(scroller);
    } else {
        gtk_widget_hide(empty_state);
        gtk_widget_show(scroller);

        gchar *last_day = NULL;
        for (guint i = 0; i < filtered->len; i++) {
            SavedLinkEntity *e = g_ptr_array_index(filtered, i);
            gchar *day = linker_format_day_header(e->saved_at_millis);
            if (!last_day || g_strcmp0(last_day, day) != 0) {
                gtk_box_pack_start(GTK_BOX(rows_container), build_day_header(day), FALSE, FALSE, 0);
                g_free(last_day);
                last_day = g_strdup(day);
            }
            g_free(day);
            gtk_box_pack_start(GTK_BOX(rows_container), build_link_row(state, toast_host, view, e, query), FALSE, FALSE, 0);
        }
        g_free(last_day);
        gtk_widget_show_all(rows_container);
    }

    g_ptr_array_free(filtered, TRUE);
    g_free(query);
}

static void on_search_changed(GtkSearchEntry *entry, gpointer user_data) {
    (void) entry;
    ui_saved_links_view_refresh(GTK_WIDGET(user_data));
}

GtkWidget *ui_saved_links_view_new(AppState *state, GtkWidget *toast_host) {
    GtkWidget *root = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);

    GtkWidget *search_row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_widget_set_margin_start(search_row, 16);
    gtk_widget_set_margin_end(search_row, 16);
    gtk_widget_set_margin_top(search_row, 8);
    gtk_widget_set_margin_bottom(search_row, 8);
    GtkWidget *search_entry = gtk_search_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(search_entry), "Search saved links");
    atk_object_set_name(gtk_widget_get_accessible(search_entry), "Search saved links");
    gtk_style_context_add_class(gtk_widget_get_style_context(search_entry), "outlined");
    gtk_box_pack_start(GTK_BOX(search_row), search_entry, TRUE, TRUE, 0);

    GtkWidget *empty_state = build_empty_state();

    GtkWidget *scroller = gtk_scrolled_window_new(NULL, NULL);
    gtk_widget_set_vexpand(scroller, TRUE);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroller), GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
    GtkWidget *rows_container = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_container_add(GTK_CONTAINER(scroller), rows_container);

    gtk_box_pack_start(GTK_BOX(root), search_row, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(root), empty_state, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(root), scroller, TRUE, TRUE, 0);

    g_object_set_data(G_OBJECT(root), "state", state);
    g_object_set_data(G_OBJECT(root), "toast-host", toast_host);
    g_object_set_data(G_OBJECT(root), "search-entry", search_entry);
    g_object_set_data(G_OBJECT(root), "rows-container", rows_container);
    g_object_set_data(G_OBJECT(root), "empty-state", empty_state);
    g_object_set_data(G_OBJECT(root), "scroller", scroller);

    g_signal_connect(search_entry, "search-changed", G_CALLBACK(on_search_changed), root);

    gtk_widget_show_all(root);
    ui_saved_links_view_refresh(root);
    return root;
}
