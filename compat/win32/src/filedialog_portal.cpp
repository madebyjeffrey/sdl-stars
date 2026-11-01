/* Native Linux chooser: libdbus wire protocol, without a GTK/Qt dependency. */
#include "filedialog_backend.h"
#include "input_io.h"
#include "time_io.h"
#include <dbus/dbus.h>
#include <climits>
#include <cstdio>
#include <cstdlib>
#include <cstring>

constexpr char PORTAL_SERVICE[] = "org.freedesktop.portal.Desktop";
constexpr char PORTAL_PATH[] = "/org/freedesktop/portal/desktop";
constexpr char PORTAL_CHOOSER[] = "org.freedesktop.portal.FileChooser";
constexpr char PORTAL_REQUEST[] = "org.freedesktop.portal.Request";

struct PORTALRESPONSES {
    DBusMessage *rgMessage[16];
    int cMessage;
    BOOL fOverflow;
};
static DBusHandlerResult SavePortalResponse(DBusConnection *lpBus, DBusMessage *lpMessage, void *lpv) {
    PORTALRESPONSES *lp = static_cast<PORTALRESPONSES *>(lpv);
    static_cast<void>(lpBus);
    if (!dbus_message_is_signal(lpMessage, PORTAL_REQUEST, "Response")) return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
    if (lp->cMessage < 16) lp->rgMessage[lp->cMessage++] = dbus_message_ref(lpMessage);
    else lp->fOverflow = 1;
    return DBUS_HANDLER_RESULT_HANDLED;
}
static BOOL FOption(DBusMessageIter *lpOptions, LPCSTR pszKey, LPCSTR pszType,
                    int type, const void *lpValue) {
    DBusMessageIter entry, value;
    return dbus_message_iter_open_container(lpOptions, DBUS_TYPE_DICT_ENTRY, nullptr, &entry) &&
           dbus_message_iter_append_basic(&entry, DBUS_TYPE_STRING, &pszKey) &&
           dbus_message_iter_open_container(&entry, DBUS_TYPE_VARIANT, pszType, &value) &&
           dbus_message_iter_append_basic(&value, type, lpValue) &&
           dbus_message_iter_close_container(&entry, &value) &&
           dbus_message_iter_close_container(lpOptions, &entry);
}
static BOOL FFolderOption(DBusMessageIter *lpOptions, LPCSTR pszFolder) {
    DBusMessageIter entry, value, bytes;
    const char *pszKey = "current_folder";
    int cb = static_cast<int>(std::strlen(pszFolder)) + 1;
    return dbus_message_iter_open_container(lpOptions, DBUS_TYPE_DICT_ENTRY, nullptr, &entry) &&
           dbus_message_iter_append_basic(&entry, DBUS_TYPE_STRING, &pszKey) &&
           dbus_message_iter_open_container(&entry, DBUS_TYPE_VARIANT, "ay", &value) &&
           dbus_message_iter_open_container(&value, DBUS_TYPE_ARRAY, "y", &bytes) &&
           dbus_message_iter_append_fixed_array(&bytes, DBUS_TYPE_BYTE, &pszFolder, cb) &&
           dbus_message_iter_close_container(&value, &bytes) &&
           dbus_message_iter_close_container(&entry, &value) &&
           dbus_message_iter_close_container(lpOptions, &entry);
}
static char *PszPortalLabel(LPCSTR psz) {
    return FUtf8Valid(psz, std::strlen(psz)) ? SDL_strdup(psz) : nullptr;
}
static BOOL FFilter(DBusMessageIter *lpIter, const COMPATFILEFILTER *lpFilter) {
    DBusMessageIter filter, patterns, item;
    const char *psz, *pszEnd;
    char *pszLabel, *pszPatternUtf, szPattern[16385];
    std::size_t i, c;
    dbus_uint32_t kind = 0;
    BOOL fOk = 0;
    pszLabel = PszPortalLabel(lpFilter->pszName);
    pszPatternUtf = PszPortalLabel(lpFilter->pszPattern);
    if (!pszLabel || !pszPatternUtf) { SDL_free(pszLabel); SDL_free(pszPatternUtf); return 0; }
    if (!dbus_message_iter_open_container(lpIter, DBUS_TYPE_STRUCT, nullptr, &filter) ||
        !dbus_message_iter_append_basic(&filter, DBUS_TYPE_STRING, &pszLabel) ||
        !dbus_message_iter_open_container(&filter, DBUS_TYPE_ARRAY, "(us)", &patterns)) goto done;
    for (psz = pszPatternUtf; *psz; psz = *pszEnd ? pszEnd + 1 : pszEnd) {
        pszEnd = std::strchr(psz, ';'); if (!pszEnd) pszEnd = psz + std::strlen(psz);
        c = 0;
        if (pszEnd - psz == 3 && std::memcmp(psz, "*.*", 3) == 0) szPattern[c++] = '*';
        else for (i = 0; i < static_cast<std::size_t>(pszEnd - psz); i++) {
            if ((psz[i] >= 'a' && psz[i] <= 'z') || (psz[i] >= 'A' && psz[i] <= 'Z')) {
                szPattern[c++] = '['; szPattern[c++] = static_cast<char>(SDL_tolower(static_cast<unsigned char>(psz[i])));
                szPattern[c++] = static_cast<char>(SDL_toupper(static_cast<unsigned char>(psz[i]))); szPattern[c++] = ']';
            } else szPattern[c++] = psz[i];
        }
        szPattern[c] = 0;
        {
            const char *pszPattern = szPattern;
            if (!dbus_message_iter_open_container(&patterns, DBUS_TYPE_STRUCT, nullptr, &item) ||
                !dbus_message_iter_append_basic(&item, DBUS_TYPE_UINT32, &kind) ||
                !dbus_message_iter_append_basic(&item, DBUS_TYPE_STRING, &pszPattern) ||
                !dbus_message_iter_close_container(&patterns, &item)) goto done;
        }
    }
    fOk = dbus_message_iter_close_container(&filter, &patterns) && dbus_message_iter_close_container(lpIter, &filter);
done:
    SDL_free(pszLabel); SDL_free(pszPatternUtf); return fOk;
}
static BOOL FFilterOptions(DBusMessageIter *lpOptions, const COMPATFILEFILTER *rgfilter, int cFilter, int iFilter) {
    DBusMessageIter entry, value, filters;
    const char *pszKey = "filters";
    int i;
    if (cFilter == 0) return 1;
    if (!dbus_message_iter_open_container(lpOptions, DBUS_TYPE_DICT_ENTRY, nullptr, &entry) ||
        !dbus_message_iter_append_basic(&entry, DBUS_TYPE_STRING, &pszKey) ||
        !dbus_message_iter_open_container(&entry, DBUS_TYPE_VARIANT, "a(sa(us))", &value) ||
        !dbus_message_iter_open_container(&value, DBUS_TYPE_ARRAY, "(sa(us))", &filters)) return 0;
    for (i = 0; i < cFilter; i++) if (!FFilter(&filters, &rgfilter[i])) return 0;
    if (!dbus_message_iter_close_container(&value, &filters) ||
        !dbus_message_iter_close_container(&entry, &value) ||
        !dbus_message_iter_close_container(lpOptions, &entry)) return 0;
    pszKey = "current_filter";
    return dbus_message_iter_open_container(lpOptions, DBUS_TYPE_DICT_ENTRY, nullptr, &entry) &&
           dbus_message_iter_append_basic(&entry, DBUS_TYPE_STRING, &pszKey) &&
           dbus_message_iter_open_container(&entry, DBUS_TYPE_VARIANT, "(sa(us))", &value) &&
           FFilter(&value, &rgfilter[iFilter]) && dbus_message_iter_close_container(&entry, &value) &&
           dbus_message_iter_close_container(lpOptions, &entry);
}
static int IHex(char ch) {
    if (ch >= '0' && ch <= '9') return ch - '0';
    if (ch >= 'a' && ch <= 'f') return ch - 'a' + 10;
    if (ch >= 'A' && ch <= 'F') return ch - 'A' + 10;
    return -1;
}
static BOOL FUriPath(LPCSTR pszUri, char *pszPath, std::size_t cbPath) {
    std::size_t c = 0;
    int hi, lo;
    unsigned char ch;
    if (std::strncmp(pszUri, "file://", 7) != 0) return 0;
    pszUri += 7;
    if (std::strncmp(pszUri, "localhost/", 10) == 0) pszUri += 9;
    if (*pszUri != '/') return 0;
    while (*pszUri) {
        if (*pszUri == '?' || *pszUri == '#') return 0;
        ch = static_cast<unsigned char>(*pszUri++);
        if (ch == '%') {
            if (!pszUri[0] || !pszUri[1] || (hi = IHex(pszUri[0])) < 0 || (lo = IHex(pszUri[1])) < 0) return 0;
            ch = static_cast<unsigned char>((hi << 4) | lo); pszUri += 2;
            if (ch == 0) return 0;
        }
        if (c + 1 >= cbPath) return 0;
        pszPath[c++] = static_cast<char>(ch);
    }
    pszPath[c] = 0; return 1;
}
static int IResponse(DBusMessage *lpMessage, const COMPATFILEFILTER *rgfilter, int cFilter,
                     int *lpiFilter, char *pszPath, std::size_t cbPath) {
    DBusMessageIter args, results, entry, value, uris, filter;
    dbus_uint32_t response;
    const char *pszKey, *pszUri, *pszName;
    char *pszLabel;
    int i;
    BOOL fUri = 0;
    if (!dbus_message_has_signature(lpMessage, "ua{sv}")) return -1;
    dbus_message_iter_init(lpMessage, &args); dbus_message_iter_get_basic(&args, &response);
    if (response == 1) return 0;
    if (response != 0) return -1;
    dbus_message_iter_next(&args); dbus_message_iter_recurse(&args, &results);
    while (dbus_message_iter_get_arg_type(&results) == DBUS_TYPE_DICT_ENTRY) {
        dbus_message_iter_recurse(&results, &entry); dbus_message_iter_get_basic(&entry, &pszKey);
        dbus_message_iter_next(&entry); dbus_message_iter_recurse(&entry, &value);
        if (std::strcmp(pszKey, "uris") == 0) {
            if (fUri || dbus_message_iter_get_arg_type(&value) != DBUS_TYPE_ARRAY ||
                dbus_message_iter_get_element_type(&value) != DBUS_TYPE_STRING) return -1;
            dbus_message_iter_recurse(&value, &uris);
            if (dbus_message_iter_get_arg_type(&uris) != DBUS_TYPE_STRING) return -1;
            dbus_message_iter_get_basic(&uris, &pszUri);
            if (!FUriPath(pszUri, pszPath, cbPath) || dbus_message_iter_next(&uris)) return -1;
            fUri = 1;
        } else if (std::strcmp(pszKey, "current_filter") == 0) {
            if (dbus_message_iter_get_arg_type(&value) != DBUS_TYPE_STRUCT) return -1;
            dbus_message_iter_recurse(&value, &filter);
            if (dbus_message_iter_get_arg_type(&filter) != DBUS_TYPE_STRING) return -1;
            dbus_message_iter_get_basic(&filter, &pszName);
            for (i = 0; i < cFilter; i++) {
                pszLabel = PszPortalLabel(rgfilter[i].pszName); if (!pszLabel) return -1;
                if (std::strcmp(pszName, pszLabel) == 0) { *lpiFilter = i; SDL_free(pszLabel); break; }
                SDL_free(pszLabel);
            }
        }
        dbus_message_iter_next(&results);
    }
    return fUri ? 1 : -1;
}
static BOOL FPumpChooser(HWND hwndOwner) {
    win32::MSG msg;
    while (win32::PeekMessageA(&msg, nullptr, 0, 0, win32::PM_REMOVE)) {
        if (msg.message == win32::WM_QUIT) { win32::PostQuitMessage(static_cast<int>(msg.wParam)); return 0; }
        win32::TranslateMessage(&msg); win32::DispatchMessageA(&msg);
        if (FStarsErrorPending()) return 0;
        if (hwndOwner && !win32::IsWindow(hwndOwner)) return 0;
    }
    return !FStarsErrorPending() && (!hwndOwner || win32::IsWindow(hwndOwner));
}
int ICompatChooseFile(HWND hwndOwner, BOOL fSave, LPCSTR pszTitle, LPCSTR pszFolder,
                     LPCSTR pszName, const COMPATFILEFILTER *rgfilter, int cFilter,
                     int *lpiFilter, char *pszPath, std::size_t cbPath) {
    DBusConnection *lpBus = nullptr;
    DBusMessage *lpCall = nullptr, *lpReply = nullptr, *lpMessage, *lpClose;
    DBusPendingCall *lpPending = nullptr;
    DBusMessageIter args, options;
    DBusError error;
    dbus_bool_t fTrue = 1, fFalse = 0;
    const char *pszParent, *pszHandle = nullptr;
    char szParent[64] = "", szToken[64], szRequest[512], szSender[256];
    NATIVEFILE *lpRandom;
    unsigned char rgbToken[16];
    const char *pszUnique;
    char *pszTitleUtf = nullptr, *pszRequest = nullptr, *pszSender = nullptr, *pszWaylandParent = nullptr;
    Uint64 nsStart;
    int iResult = -1;
    PORTALRESPONSES responses = {};
    int i;
    BOOL fFilter = 0;
    Uint64 msOwnerCheck = 0;
    SDL_PropertiesID props;
    Sint64 xid;

    dbus_error_init(&error);
    lpBus = dbus_bus_get_private(DBUS_BUS_SESSION, &error);
    if (!lpBus) goto done;
    dbus_connection_set_exit_on_disconnect(lpBus, 0);
    fFilter = dbus_connection_add_filter(lpBus, SavePortalResponse, &responses, nullptr);
    if (!fFilter) goto done;
    dbus_bus_add_match(lpBus, "type='signal',sender='org.freedesktop.portal.Desktop',interface='org.freedesktop.portal.Request',member='Response'", &error);
    if (dbus_error_is_set(&error)) goto done;
    if (hwndOwner && hwndOwner->lpWindow.get()) {
        props = SDL_GetWindowProperties(hwndOwner->lpWindow.get());
        /* SDL exports xdg-foreign asynchronously. Copy the handle before the
           modal pump, which may destroy its window and property storage. */
        nsStart = CompatTimeIO().pfnGetTicks();
        while (std::strcmp(CompatInputIO().pfnGetCurrentVideoDriver(), "wayland") == 0 &&
               !SDL_GetStringProperty(props, SDL_PROP_WINDOW_WAYLAND_XDG_TOPLEVEL_EXPORT_HANDLE_STRING, nullptr) &&
               CompatTimeIO().pfnGetTicks() - nsStart < 1000) {
            if (!FPumpChooser(hwndOwner)) { iResult = 0; goto done; }
            SDL_Delay(1);
        }
        pszHandle = SDL_GetStringProperty(props, SDL_PROP_WINDOW_WAYLAND_XDG_TOPLEVEL_EXPORT_HANDLE_STRING, nullptr);
        if (pszHandle && *pszHandle && SDL_asprintf(&pszWaylandParent, "wayland:%s", pszHandle) < 0) goto done;
        xid = SDL_GetNumberProperty(props, SDL_PROP_WINDOW_X11_WINDOW_NUMBER, 0);
        if (xid != 0) std::snprintf(szParent, sizeof(szParent), "x11:%llx", (unsigned long long)xid);
    }
    pszParent = pszWaylandParent ? pszWaylandParent : szParent;
    lpRandom = PfNativeOpen("/dev/urandom", "rb");
    if (!lpRandom) goto done;
    if (CReadNative(rgbToken, 1, sizeof(rgbToken), lpRandom) != sizeof(rgbToken)) { ICloseNativeFile(lpRandom); goto done; }
    ICloseNativeFile(lpRandom);
    std::strcpy(szToken, "stars_");
    for (i = 0; i < 16; i++) std::snprintf(szToken + 6 + i * 2, 3, "%02x", rgbToken[i]);
    pszUnique = dbus_bus_get_unique_name(lpBus);
    if (!pszUnique || std::strlen(pszUnique) >= sizeof(szSender)) goto done;
    std::strcpy(szSender, pszUnique + 1);
    for (i = 0; szSender[i]; i++) if (szSender[i] == '.') szSender[i] = '_';
    std::snprintf(szRequest, sizeof(szRequest), "%s/request/%s/%s", PORTAL_PATH, szSender, szToken);
    pszRequest = strdup(szRequest); if (!pszRequest) goto done;
    if (!dbus_validate_utf8(pszName, &error)) goto done;
    pszTitleUtf = PszPortalLabel(pszTitle); if (!pszTitleUtf) goto done;
    lpCall = dbus_message_new_method_call(PORTAL_SERVICE, PORTAL_PATH, PORTAL_CHOOSER, fSave ? "SaveFile" : "OpenFile");
    if (!lpCall) goto done;
    dbus_message_iter_init_append(lpCall, &args);
    if (!dbus_message_iter_append_basic(&args, DBUS_TYPE_STRING, &pszParent) ||
        !dbus_message_iter_append_basic(&args, DBUS_TYPE_STRING, &pszTitleUtf) ||
        !dbus_message_iter_open_container(&args, DBUS_TYPE_ARRAY, "{sv}", &options)) goto done;
    {
        const char *pszToken = szToken;
        if (!FOption(&options, "handle_token", "s", DBUS_TYPE_STRING, &pszToken) ||
            !FOption(&options, "modal", "b", DBUS_TYPE_BOOLEAN, &fTrue) ||
            (!fSave && !FOption(&options, "multiple", "b", DBUS_TYPE_BOOLEAN, &fFalse)) ||
            !FFolderOption(&options, pszFolder) || !FFilterOptions(&options, rgfilter, cFilter, *lpiFilter) ||
            (fSave && !FOption(&options, "current_name", "s", DBUS_TYPE_STRING, &pszName)) ||
            !dbus_message_iter_close_container(&args, &options)) goto done;
    }
    if (!dbus_connection_send_with_reply(lpBus, lpCall, &lpPending, 10000) || !lpPending) goto done;
    nsStart = CompatTimeIO().pfnGetTicks();
    while (!dbus_pending_call_get_completed(lpPending)) {
        if (!FPumpChooser(hwndOwner)) { iResult = 0; goto done; }
        if (!dbus_connection_read_write_dispatch(lpBus, 10) || CompatTimeIO().pfnGetTicks() - nsStart > 11000) goto done;
    }
    lpReply = dbus_pending_call_steal_reply(lpPending);
    if (!lpReply || dbus_message_get_type(lpReply) == DBUS_MESSAGE_TYPE_ERROR ||
        !dbus_message_get_args(lpReply, &error, DBUS_TYPE_OBJECT_PATH, &pszHandle, DBUS_TYPE_INVALID)) goto done;
    if (!dbus_message_get_sender(lpReply)) goto done;
    std::free(pszRequest); pszRequest = strdup(pszHandle); pszSender = strdup(dbus_message_get_sender(lpReply));
    if (!pszRequest || !pszSender) goto done;
    for (;;) {
        if (!FPumpChooser(hwndOwner)) { iResult = 0; break; }
        if (!dbus_connection_read_write_dispatch(lpBus, 10) || responses.fOverflow) break;
        for (i = 0; i < responses.cMessage; i++) {
            lpMessage = responses.rgMessage[i];
            if (dbus_message_has_path(lpMessage, pszRequest) && dbus_message_has_sender(lpMessage, pszSender)) {
                iResult = IResponse(lpMessage, rgfilter, cFilter, lpiFilter, pszPath, cbPath); goto done;
            }
            dbus_message_unref(lpMessage); responses.rgMessage[i] = nullptr;
        }
        responses.cMessage = 0;
        if (CompatTimeIO().pfnGetTicks() - msOwnerCheck >= 1000) {
            msOwnerCheck = CompatTimeIO().pfnGetTicks();
            if (!dbus_bus_name_has_owner(lpBus, pszSender, &error)) break;
        }
    }
done:
    if (lpBus && pszRequest && iResult != 1) {
        lpClose = dbus_message_new_method_call(PORTAL_SERVICE, pszRequest, PORTAL_REQUEST, "Close");
        if (lpClose) { dbus_message_set_no_reply(lpClose, 1); dbus_connection_send(lpBus, lpClose, nullptr); dbus_message_unref(lpClose); }
    }
    if (dbus_error_is_set(&error)) SDL_SetError("File chooser portal: %s", error.message);
    else if (iResult < 0) SDL_SetError("File chooser portal failed or returned an invalid result");
    dbus_error_free(&error);
    if (lpPending) { dbus_pending_call_cancel(lpPending); dbus_pending_call_unref(lpPending); }
    if (lpReply) dbus_message_unref(lpReply);
    if (lpCall) dbus_message_unref(lpCall);
    for (i = 0; i < responses.cMessage; i++) if (responses.rgMessage[i]) dbus_message_unref(responses.rgMessage[i]);
    if (fFilter) dbus_connection_remove_filter(lpBus, SavePortalResponse, &responses);
    if (lpBus) { dbus_connection_flush(lpBus); dbus_connection_close(lpBus); dbus_connection_unref(lpBus); }
    SDL_free(pszTitleUtf); SDL_free(pszWaylandParent); std::free(pszRequest); std::free(pszSender);
    return iResult;
}
