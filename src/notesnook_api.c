#include "notesnook_api.h"
#include "notesnook_store.h"
#include "json_min.h"
#include "util.h"
#include <curl/curl.h>
#include <string.h>

static size_t discard_write(char *ptr, size_t size, size_t nmemb, void *userdata) {
    (void) ptr;
    (void) userdata;
    return size * nmemb;
}

NotesnookSendOutcome notesnook_send_link(const gchar *url) {
    NotesnookSettings *settings = notesnook_settings_load();
    if (!settings->api_key || !*settings->api_key) {
        notesnook_settings_free(settings);
        return NOTESNOOK_SEND_NO_API_KEY;
    }

    gchar *escaped_url = linker_html_escape(url);
    GDateTime *now = g_date_time_new_now_local();
    gchar *sent_at = g_date_time_format(now, "%Y-%m-%d %H:%M");
    g_date_time_unref(now);

    gchar *content_data = g_strdup_printf("<p>%s - <a href=\"%s\">%s</a></p>", sent_at, escaped_url, escaped_url);
    g_free(escaped_url);
    g_free(sent_at);

    JsonValue *root = json_new_object();
    gchar *title = g_strdup_printf("Link: %s", url);
    json_object_set(root, "title", json_new_string(title));
    g_free(title);
    json_object_set(root, "type", json_new_string("note"));
    json_object_set(root, "source", json_new_string("linker-linux"));
    json_object_set(root, "version", json_new_number(1));

    JsonValue *content = json_new_object();
    json_object_set(content, "type", json_new_string("html"));
    json_object_set(content, "data", json_new_string(content_data));
    json_object_set(root, "content", content);
    g_free(content_data);

    if (settings->tag_id && *settings->tag_id) {
        JsonValue *tag_ids = json_new_array();
        json_array_append(tag_ids, json_new_string(settings->tag_id));
        json_object_set(root, "tagIds", tag_ids);
    }

    gchar *body = json_to_string(root);
    json_value_free(root);

    CURL *curl = curl_easy_init();
    NotesnookSendOutcome outcome = NOTESNOOK_SEND_FAILED;
    if (curl) {
        gchar *auth_header = g_strdup_printf("Authorization: %s", settings->api_key);
        struct curl_slist *headers = NULL;
        headers = curl_slist_append(headers, auth_header);
        headers = curl_slist_append(headers, "Content-Type: application/json");

        curl_easy_setopt(curl, CURLOPT_URL, "https://inbox.notesnook.com/");
        curl_easy_setopt(curl, CURLOPT_POST, 1L);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, body);
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 15L);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, discard_write);
        curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L);

        CURLcode res = curl_easy_perform(curl);
        if (res == CURLE_OK) {
            long status_code = 0;
            curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &status_code);
            outcome = (status_code >= 200 && status_code < 300) ? NOTESNOOK_SEND_SUCCESS : NOTESNOOK_SEND_FAILED;
        } else {
            g_warning("notesnook send failed: %s", curl_easy_strerror(res));
        }

        g_free(auth_header);
        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);
    }

    g_free(body);
    notesnook_settings_free(settings);
    return outcome;
}
