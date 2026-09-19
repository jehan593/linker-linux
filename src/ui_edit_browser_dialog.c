#include "ui_edit_browser_dialog.h"
#include "ui_browsers_view.h"
#include "ui_widgets.h"
#include "icon_resolve.h"
#include "toast.h"
#include <string.h>

#define RESPONSE_LEFT 100 /* Delete (custom) / Reset to defaults (system) */

typedef struct {
    GtkWindow *parent;
    GtkWidget *icon_img;
    GtkWidget *icon_hint;
    gchar *custom_icon_path; /* owned, nullable */
    gchar *fallback_icon_field;
    GtkWidget *toast_host;
} IconPickerCtx;

static void refresh_icon_preview(IconPickerCtx *ctx) {
    GdkPixbuf *pixbuf = NULL;
    if (ctx->custom_icon_path && *ctx->custom_icon_path) {
        pixbuf = icon_resolve_from_file(ctx->custom_icon_path, 32);
    }
    if (!pixbuf) {
        pixbuf = icon_resolve_browser_icon(ctx->fallback_icon_field, 32);
    }
    gtk_image_set_from_pixbuf(GTK_IMAGE(ctx->icon_img), pixbuf);
    g_object_unref(pixbuf);

    if (ctx->custom_icon_path && *ctx->custom_icon_path) {
        gchar *base = g_path_get_basename(ctx->custom_icon_path);
        gchar *hint = g_strdup_printf("Custom: %s", base);
        gtk_label_set_text(GTK_LABEL(ctx->icon_hint), hint);
        g_free(hint);
        g_free(base);
    } else {
        gtk_label_set_text(GTK_LABEL(ctx->icon_hint), "");
    }
}

static void on_change_icon_clicked(GtkButton *btn, gpointer user_data) {
    (void) btn;
    IconPickerCtx *ctx = user_data;
    GtkWidget *chooser = gtk_file_chooser_dialog_new(
        "Choose an icon", ctx->parent, GTK_FILE_CHOOSER_ACTION_OPEN,
        "_Cancel", GTK_RESPONSE_CANCEL, "_Open", GTK_RESPONSE_ACCEPT, NULL);

    GtkFileFilter *filter = gtk_file_filter_new();
    gtk_file_filter_set_name(filter, "Icon images");
    gtk_file_filter_add_pattern(filter, "*.png");
    gtk_file_filter_add_pattern(filter, "*.svg");
    gtk_file_filter_add_pattern(filter, "*.xpm");
    gtk_file_filter_add_pattern(filter, "*.ico");
    gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(chooser), filter);

    if (gtk_dialog_run(GTK_DIALOG(chooser)) == GTK_RESPONSE_ACCEPT) {
        gchar *path = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(chooser));
        GdkPixbuf *test = icon_resolve_from_file(path, 32);
        if (!test) {
            toast_host_show(ctx->toast_host, "Couldn't load an icon from that file");
            g_free(path);
        } else {
            g_object_unref(test);
            g_free(ctx->custom_icon_path);
            ctx->custom_icon_path = path;
            refresh_icon_preview(ctx);
        }
    }
    gtk_widget_destroy(chooser);
}

static void on_use_default_clicked(GtkButton *btn, gpointer user_data) {
    (void) btn;
    IconPickerCtx *ctx = user_data;
    g_free(ctx->custom_icon_path);
    ctx->custom_icon_path = NULL;
    refresh_icon_preview(ctx);
}

static void on_browse_exec_clicked(GtkButton *btn, gpointer user_data) {
    (void) btn;
    GtkTextView *view = user_data;
    GtkWidget *toplevel = gtk_widget_get_toplevel(GTK_WIDGET(view));
    GtkWidget *chooser = gtk_file_chooser_dialog_new(
        "Choose an executable", GTK_WINDOW(toplevel), GTK_FILE_CHOOSER_ACTION_OPEN,
        "_Cancel", GTK_RESPONSE_CANCEL, "_Open", GTK_RESPONSE_ACCEPT, NULL);
    if (gtk_dialog_run(GTK_DIALOG(chooser)) == GTK_RESPONSE_ACCEPT) {
        gchar *path = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(chooser));
        ui_textview_set_text(view, path);
        g_free(path);
    }
    gtk_widget_destroy(chooser);
}

static void response_trampoline_ok(GtkButton *btn, gpointer dialog) {
    (void) btn;
    gtk_dialog_response(GTK_DIALOG(dialog), GTK_RESPONSE_OK);
}
static void response_trampoline_cancel(GtkButton *btn, gpointer dialog) {
    (void) btn;
    gtk_dialog_response(GTK_DIALOG(dialog), GTK_RESPONSE_CANCEL);
}
static void response_trampoline_left(GtkButton *btn, gpointer dialog) {
    (void) btn;
    gtk_dialog_response(GTK_DIALOG(dialog), RESPONSE_LEFT);
}

void ui_edit_browser_dialog_run(GtkWindow *parent, AppState *state,
                                 const BrowserListItem *item, GtkWidget *browsers_view) {
    gboolean is_add = (item == NULL);
    gboolean is_custom_edit = item && item->is_custom;
    gboolean validate = is_add || is_custom_edit;

    GtkWidget *dialog = gtk_dialog_new_with_buttons(
        is_add ? "Add browser" : "Edit browser", parent,
        GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT, NULL, NULL);
    gtk_window_set_default_size(GTK_WINDOW(dialog), 500, 620);
    gtk_widget_set_size_request(dialog, 380, 420);
    gtk_window_set_position(GTK_WINDOW(dialog), GTK_WIN_POS_CENTER_ON_PARENT);

    GtkWidget *content = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    gtk_style_context_add_class(gtk_widget_get_style_context(content), "content-pad-16");

    GtkWidget *title_label = ui_title_label_new(is_add ? "Add browser" : "Edit browser");
    gtk_widget_set_margin_bottom(title_label, 16);
    gtk_box_pack_start(GTK_BOX(content), title_label, FALSE, FALSE, 0);

    GtkWidget *form = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);

    GtkWidget *icon_row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 12);
    gtk_widget_set_margin_bottom(icon_row, 8);
    GtkWidget *icon_img = gtk_image_new();
    gtk_widget_set_size_request(icon_img, 32, 32);
    gtk_widget_set_valign(icon_img, GTK_ALIGN_START);

    GtkWidget *icon_col = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);
    GtkWidget *icon_btn_row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);
    GtkWidget *change_icon_btn = ui_text_button_new("Change icon…", "text-button-primary");
    GtkWidget *use_default_btn = ui_text_button_new("Use default", "text-button-neutral");
    gtk_box_pack_start(GTK_BOX(icon_btn_row), change_icon_btn, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(icon_btn_row), use_default_btn, FALSE, FALSE, 0);
    GtkWidget *icon_hint = ui_body_small_label_new("");
    gtk_box_pack_start(GTK_BOX(icon_col), icon_btn_row, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(icon_col), icon_hint, FALSE, FALSE, 0);

    gtk_box_pack_start(GTK_BOX(icon_row), icon_img, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(icon_row), icon_col, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(form), icon_row, FALSE, FALSE, 0);

    gtk_box_pack_start(GTK_BOX(form), ui_label_label_new("Display name"), FALSE, FALSE, 0);
    GtkWidget *name_entry = ui_outlined_entry_new();
    atk_object_set_name(gtk_widget_get_accessible(name_entry), "Display name");
    gtk_box_pack_start(GTK_BOX(form), name_entry, FALSE, FALSE, 0);

    GtkWidget *target_scroll, *target_view;
    gtk_box_pack_start(GTK_BOX(form), ui_label_label_new("Target"), FALSE, FALSE, 0);
    target_scroll = ui_outlined_textview_new(&target_view);
    atk_object_set_name(gtk_widget_get_accessible(target_view), "Target");
    gtk_widget_set_hexpand(target_scroll, TRUE);
    gtk_widget_set_size_request(target_scroll, -1, 84);
    gtk_box_pack_start(GTK_BOX(form), target_scroll, FALSE, FALSE, 0);
    ui_bind_enter(target_view, dialog, GTK_RESPONSE_OK);
    GtkWidget *browse_btn = ui_text_button_new("Browse…", "text-button-neutral");
    gtk_widget_set_halign(browse_btn, GTK_ALIGN_START);
    gtk_box_pack_start(GTK_BOX(form), browse_btn, FALSE, FALSE, 0);

    GtkWidget *args_scroll, *args_view;
    gtk_box_pack_start(GTK_BOX(form), ui_label_label_new("Additional parameters"), FALSE, FALSE, 0);
    args_scroll = ui_outlined_textview_new(&args_view);
    atk_object_set_name(gtk_widget_get_accessible(args_view), "Additional parameters");
    gtk_widget_set_size_request(args_scroll, -1, 84);
    gtk_box_pack_start(GTK_BOX(form), args_scroll, FALSE, FALSE, 0);
    ui_bind_enter(args_view, dialog, GTK_RESPONSE_OK);
    GtkWidget *args_hint = ui_body_small_label_new(
        "Space-separated, added before the link — e.g. --private-window --new-window. "
        "Quote arguments that contain spaces.");
    gtk_label_set_line_wrap(GTK_LABEL(args_hint), TRUE);
    gtk_label_set_max_width_chars(GTK_LABEL(args_hint), 56);
    gtk_box_pack_start(GTK_BOX(form), args_hint, FALSE, FALSE, 0);

    gtk_box_pack_start(GTK_BOX(content), form, FALSE, FALSE, 0);

    GtkWidget *button_row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
    gtk_widget_set_margin_top(button_row, 16);
    GtkWidget *left_btn = NULL;
    if (!is_add) {
        left_btn = is_custom_edit ? ui_text_button_new("Delete", "text-button-error")
                                   : ui_text_button_new("Reset to defaults", "text-button-warning");
    }
    GtkWidget *spacer = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_widget_set_hexpand(spacer, TRUE);
    GtkWidget *cancel_btn = ui_text_button_new("Cancel", "text-button-neutral");
    GtkWidget *save_btn = ui_pill_button_new(is_add ? "Add" : "Save");

    if (left_btn) gtk_box_pack_start(GTK_BOX(button_row), left_btn, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(button_row), spacer, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(button_row), cancel_btn, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(button_row), save_btn, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(content), button_row, FALSE, FALSE, 0);

    /* Toasts from inside this dialog are shown here — the parent window is
     * hidden behind the modal, so its toast host can't be seen. */
    GtkWidget *dialog_toast_host = toast_host_new();
    gtk_widget_set_size_request(dialog_toast_host, -1, 34);
    gtk_box_pack_start(GTK_BOX(content), dialog_toast_host, FALSE, FALSE, 0);
    g_object_set_data(G_OBJECT(dialog), "toast-host", dialog_toast_host);

    /* prefill */
    gtk_entry_set_text(GTK_ENTRY(name_entry), item ? item->display_label : "");
    ui_textview_set_text(GTK_TEXT_VIEW(target_view), item ? browser_list_item_effective_cmdline(item) : "");
    ui_textview_set_text(GTK_TEXT_VIEW(args_view), (item && item->extra_arguments) ? item->extra_arguments : "");
    gtk_entry_set_activates_default(GTK_ENTRY(name_entry), TRUE);
    gtk_widget_set_can_default(save_btn, TRUE);
    gtk_window_set_default(GTK_WINDOW(dialog), save_btn);

    IconPickerCtx ctx = {
        .parent = GTK_WINDOW(dialog),
        .icon_img = icon_img,
        .icon_hint = icon_hint,
        .custom_icon_path = (item && item->custom_icon_path && *item->custom_icon_path) ? g_strdup(item->custom_icon_path) : NULL,
        .fallback_icon_field = g_strdup(item ? item->system_icon : ""),
        .toast_host = dialog_toast_host,
    };
    refresh_icon_preview(&ctx);

    g_signal_connect(change_icon_btn, "clicked", G_CALLBACK(on_change_icon_clicked), &ctx);
    g_signal_connect(use_default_btn, "clicked", G_CALLBACK(on_use_default_clicked), &ctx);
    g_signal_connect(browse_btn, "clicked", G_CALLBACK(on_browse_exec_clicked), target_view);
    g_signal_connect(save_btn, "clicked", G_CALLBACK(response_trampoline_ok), dialog);
    g_signal_connect(cancel_btn, "clicked", G_CALLBACK(response_trampoline_cancel), dialog);
    if (left_btn) g_signal_connect(left_btn, "clicked", G_CALLBACK(response_trampoline_left), dialog);

    gtk_widget_show_all(dialog);
    gtk_widget_grab_focus(name_entry);
    gtk_editable_select_region(GTK_EDITABLE(name_entry), 0, -1);

    gint response;
    for (;;) {
        response = gtk_dialog_run(GTK_DIALOG(dialog));
        if (response == RESPONSE_LEFT && is_custom_edit &&
            !ui_confirm_delete(GTK_WINDOW(dialog), "Delete browser?",
                               "This removes this browser's custom settings from Linker.")) continue;
        if (response != GTK_RESPONSE_OK || !validate) break;

        gchar *exec_trim = g_strstrip(ui_textview_get_text(GTK_TEXT_VIEW(target_view)));
        gchar *name_trim = g_strstrip(g_strdup(gtk_entry_get_text(GTK_ENTRY(name_entry))));
        gboolean ok = TRUE;
        if (!*exec_trim) {
            toast_host_show(dialog_toast_host, "Choose an executable first");
            ok = FALSE;
        } else if (!*name_trim) {
            toast_host_show(dialog_toast_host, "Give it a name first");
            ok = FALSE;
        }
        g_free(exec_trim);
        g_free(name_trim);
        if (ok) break;
    }

    if (response == GTK_RESPONSE_OK) {
        gchar *name_trim = g_strstrip(g_strdup(gtk_entry_get_text(GTK_ENTRY(name_entry))));
        gchar *exec_trim = g_strstrip(ui_textview_get_text(GTK_TEXT_VIEW(target_view)));
        gchar *args_trim = g_strstrip(ui_textview_get_text(GTK_TEXT_VIEW(args_view)));

        if (is_add) {
            gchar *uuid = g_uuid_string_random();
            gchar *id = g_strdup_printf("custom:%s", uuid);
            g_free(uuid);
            BrowserPrefEntity *pref = browser_pref_entity_new(id);
            g_free(id);
            pref->is_custom = TRUE;
            pref->custom_label = g_strdup(name_trim);
            pref->custom_executable_path = g_strdup(exec_trim);
            pref->custom_icon_path = ctx.custom_icon_path ? g_strdup(ctx.custom_icon_path) : NULL;
            pref->extra_arguments = *args_trim ? g_strdup(args_trim) : NULL;

            gint max_order = -1;
            for (guint i = 0; i < state->data->browser_prefs->len; i++) {
                BrowserPrefEntity *e = g_ptr_array_index(state->data->browser_prefs, i);
                if (e->order_index > max_order) max_order = e->order_index;
            }
            pref->order_index = max_order + 1;
            g_ptr_array_add(state->data->browser_prefs, pref);
        } else {
            BrowserPrefEntity *pref = linker_data_find_browser_pref(state->data, item->id);
            if (!pref) {
                pref = browser_pref_entity_new(item->id);
                pref->order_index = item->order_index;
                g_ptr_array_add(state->data->browser_prefs, pref);
            }
            if (is_custom_edit) {
                g_free(pref->custom_label);
                pref->custom_label = g_strdup(name_trim);
                g_free(pref->custom_executable_path);
                pref->custom_executable_path = g_strdup(exec_trim);
            } else {
                g_free(pref->custom_label);
                pref->custom_label = *name_trim ? g_strdup(name_trim) : NULL;
                g_free(pref->custom_executable_path);
                pref->custom_executable_path =
                    (!*exec_trim || g_ascii_strcasecmp(exec_trim, item->system_exec_cmdline) == 0)
                        ? NULL : g_strdup(exec_trim);
            }
            g_free(pref->custom_icon_path);
            pref->custom_icon_path = ctx.custom_icon_path ? g_strdup(ctx.custom_icon_path) : NULL;
            g_free(pref->extra_arguments);
            pref->extra_arguments = *args_trim ? g_strdup(args_trim) : NULL;
        }
        app_state_save(state, dialog_toast_host);

        g_free(name_trim);
        g_free(exec_trim);
        g_free(args_trim);
    } else if (response == RESPONSE_LEFT) {
        if (item && is_custom_edit) {
            for (guint i = 0; i < state->data->browser_prefs->len; i++) {
                BrowserPrefEntity *e = g_ptr_array_index(state->data->browser_prefs, i);
                if (g_strcmp0(e->id, item->id) == 0) {
                    g_ptr_array_remove_index(state->data->browser_prefs, i);
                    break;
                }
            }
        } else if (item) {
            BrowserPrefEntity *pref = linker_data_find_browser_pref(state->data, item->id);
            if (pref) {
                g_free(pref->custom_label);
                pref->custom_label = NULL;
                g_free(pref->custom_executable_path);
                pref->custom_executable_path = NULL;
                g_free(pref->custom_icon_path);
                pref->custom_icon_path = NULL;
                g_free(pref->extra_arguments);
                pref->extra_arguments = NULL;
            }
        }
        app_state_save(state, dialog_toast_host);
    }

    g_free(ctx.custom_icon_path);
    g_free(ctx.fallback_icon_field);
    gtk_widget_destroy(dialog);
    ui_browsers_view_refresh_deferred(browsers_view);
}
