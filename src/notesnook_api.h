/* Notesnook inbox push — the Linux analogue of Data/Remote/NotesnookApi.cs. A single
 * synchronous libcurl POST (the app has no async event loop; the request is small and
 * the 15s timeout is a rare worst case, so blocking briefly is an acceptable tradeoff
 * for staying dependency-minimal). */
#ifndef LINKER_NOTESNOOK_API_H
#define LINKER_NOTESNOOK_API_H

#include <glib.h>

typedef enum {
    NOTESNOOK_SEND_SUCCESS,
    NOTESNOOK_SEND_NO_API_KEY,
    NOTESNOOK_SEND_FAILED,
} NotesnookSendOutcome;

/* POSTs https://inbox.notesnook.com/ with the exact body shape from the Windows app
 * (source: "linker-linux"). Reads the API key/tag from notesnook_store. */
NotesnookSendOutcome notesnook_send_link(const gchar *url);

#endif
