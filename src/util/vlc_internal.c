#ifdef _WIN32
#include <windows.h>
#endif

#ifndef DISABLE_TUNA_VLC

#include "vlc_internal.h"
#include <util/platform.h>

/* libvlc core */
LIBVLC_NEW libvlc_new_;
LIBVLC_RELEASE libvlc_release_;
LIBVLC_CLOCK libvlc_clock_;
LIBVLC_EVENT_ATTACH libvlc_event_attach_;

/* libvlc media player */
LIBVLC_MEDIA_PLAYER_GET_TIME libvlc_media_player_get_time_;
LIBVLC_MEDIA_PLAYER_GET_LENGTH libvlc_media_player_get_length_;
LIBVLC_MEDIA_PLAYER_GET_STATE libvlc_media_player_get_state_;

/* libvlc media */
LIBVLC_MEDIA_GET_META libvlc_media_get_meta_;
LIBVLC_MEDIA_PLAYER_CAN_PAUSE libvlc_media_player_can_pause_;
LIBVLC_MEDIA_PLAYER_PAUSE libvlc_media_player_pause_;
LIBVLC_MEDIA_PLAYER_GET_MEDIA libvlc_media_player_get_media_;

/* libvlc media list player */
LIBVLC_MEDIA_LIST_PLAYER_PLAY libvlc_media_list_player_play_;
LIBVLC_MEDIA_LIST_PLAYER_PAUSE libvlc_media_list_player_pause_;
LIBVLC_MEDIA_LIST_PLAYER_STOP libvlc_media_list_player_stop_;

LIBVLC_MEDIA_LIST_PLAYER_NEXT libvlc_media_list_player_next_;
LIBVLC_MEDIA_LIST_PLAYER_PREVIOUS libvlc_media_list_player_previous_;

void* libvlc_module = NULL;

#ifdef __APPLE__
void* libvlc_core_module = NULL;
#endif

libvlc_instance_t* libvlc = NULL;
uint64_t time_start = 0;

bool load_vlc_funcs(void)
{
#define LOAD_VLC_FUNC(func)                               \
    do {                                                  \
        func##_ = os_dlsym(libvlc_module, #func);         \
        if (!func##_) {                                   \
            blog(LOG_WARNING,                             \
                "[tuna] Could not func VLC function %s, " \
                "VLC loading failed",                     \
                #func);                                   \
            return false;                                 \
        }                                                 \
    } while (false)

    /* libvlc core */
    LOAD_VLC_FUNC(libvlc_new);
    LOAD_VLC_FUNC(libvlc_release);
    LOAD_VLC_FUNC(libvlc_clock);
    LOAD_VLC_FUNC(libvlc_event_attach);

    /* libvlc media */
    LOAD_VLC_FUNC(libvlc_media_get_meta);

    /* libvlc media player */
    LOAD_VLC_FUNC(libvlc_media_player_get_time);
    LOAD_VLC_FUNC(libvlc_media_player_get_length);
    LOAD_VLC_FUNC(libvlc_media_player_get_state);
    LOAD_VLC_FUNC(libvlc_media_player_can_pause);
    LOAD_VLC_FUNC(libvlc_media_player_pause);
    LOAD_VLC_FUNC(libvlc_media_player_get_media);

    /* libvlc media list player */
    LOAD_VLC_FUNC(libvlc_media_list_player_play);
    LOAD_VLC_FUNC(libvlc_media_list_player_pause);
    LOAD_VLC_FUNC(libvlc_media_list_player_stop);
    LOAD_VLC_FUNC(libvlc_media_list_player_next);
    LOAD_VLC_FUNC(libvlc_media_list_player_previous);

    return true;
}

bool load_libvlc_module(void)
{
#ifdef _WIN32
    char* path_utf8 = NULL;
    wchar_t path[1024];
    LSTATUS status;
    DWORD size;
    HKEY key;

    memset(path, 0, 1024 * sizeof(wchar_t));

    status = RegOpenKeyW(HKEY_LOCAL_MACHINE, L"SOFTWARE\\VideoLAN\\VLC", &key);
    if (status != ERROR_SUCCESS)
        return false;

    size = 1024;
    status = RegQueryValueExW(key, L"InstallDir", NULL, NULL, (LPBYTE)path, &size);
    if (status == ERROR_SUCCESS) {
        wcscat(path, L"\\libvlc.dll");
        os_wcs_to_utf8_ptr(path, 0, &path_utf8);
        libvlc_module = os_dlopen(path_utf8);
        bfree(path_utf8);
    }

    RegCloseKey(key);
#else

#ifdef __APPLE__
#define LIBVLC_DIR "/Applications/VLC.app/Contents/MacOS/"
/* According to otoolo -L, this is what libvlc.dylib wants. */
#define LIBVLC_CORE_FILE LIBVLC_DIR "lib/libvlccore.dylib"
#define LIBVLC_FILE LIBVLC_DIR "lib/libvlc.5.dylib"
    setenv("VLC_PLUGIN_PATH", LIBVLC_DIR "plugins", false);
    libvlc_core_module = os_dlopen(LIBVLC_CORE_FILE);

    if (!libvlc_core_module)
        return false;
#else
#define LIBVLC_FILE "libvlc.so.5"
#endif
    libvlc_module = os_dlopen(LIBVLC_FILE);

#endif

    return libvlc_module != NULL;
}

bool load_libvlc(void)
{
    if (libvlc)
        return true;

    libvlc = libvlc_new_(0, 0);
    if (!libvlc) {
        blog(LOG_WARNING, "[tuna] Couldn't create libvlc instance");
        return false;
    }

    time_start = (uint64_t)libvlc_clock_() * 1000ULL;
    return true;
}

void unload_libvlc(void)
{
    if (libvlc)
        libvlc_release_(libvlc);
#ifdef __APPLE__
    if (libvlc_core_module)
        os_dlclose(libvlc_core_module);
#endif
    if (libvlc_module)
        os_dlclose(libvlc_module);
}
#endif /* DISABLE VLC */
