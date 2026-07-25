/* Notesnook API key/tag persistence — a separate small JSON file
 * ($XDG_DATA_HOME/linker/settings.json), mirroring the Windows app's split from the
 * main data store. */
#ifndef LINKER_NOTESNOOK_STORE_H
#define LINKER_NOTESNOOK_STORE_H

#include <glib.h>

typedef struct {
    gchar *api_key; /* nullable */
    gchar *tag_id;  /* nullable */
} NotesnookSettings;

NotesnookSettings *notesnook_settings_load(void);
gboolean notesnook_settings_save(const NotesnookSettings *settings, GError **error);
void notesnook_settings_free(NotesnookSettings *settings);

#endif
