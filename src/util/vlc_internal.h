#pragma once
#ifdef __cplusplus
extern "C" {
#endif

#include <obs-module.h>

#ifdef _MSC_VER
#include <basetsd.h>
typedef SSIZE_T ssize_t;
#endif

#ifndef DISABLE_TUNA_VLC

// clang-format off
/* Include order is important, otherwise it won't compile */
#include <libvlc.h>
#include <libvlc_media.h>
#include <libvlc_media_list.h>
#include <libvlc_events.h>
#include <libvlc_media_player.h>
#include <libvlc_media_list_player.h>

// clang-format on

/* Lord forgive me for this atrocity */
#define private priv
#define signals sigs
#include <obs-internal.h>
#undef private
#undef signals

extern libvlc_instance_t* libvlc;
extern uint64_t time_start;

extern bool load_libvlc(void);

extern bool load_libvlc_module();
extern bool load_vlc_funcs();
extern void unload_libvlc();

/* libvlc core */
typedef libvlc_instance_t* (*LIBVLC_NEW)(int argc, const char* const* argv);
typedef void (*LIBVLC_RELEASE)(libvlc_instance_t* p_instance);
typedef int64_t (*LIBVLC_CLOCK)(void);
typedef int (*LIBVLC_EVENT_ATTACH)(libvlc_event_manager_t* p_event_manager,
    libvlc_event_type_t i_event_type,
    libvlc_callback_t f_callback,
    void* user_data);

extern LIBVLC_NEW libvlc_new_;
extern LIBVLC_RELEASE libvlc_release_;
extern LIBVLC_CLOCK libvlc_clock_;
extern LIBVLC_EVENT_ATTACH libvlc_event_attach_;

/* libvlc media player methods */
typedef libvlc_time_t (*LIBVLC_MEDIA_PLAYER_GET_TIME)(
    libvlc_media_player_t* p_mi);
typedef libvlc_time_t (*LIBVLC_MEDIA_PLAYER_GET_LENGTH)(
    libvlc_media_player_t* p_mi);
typedef libvlc_state_t (*LIBVLC_MEDIA_PLAYER_GET_STATE)(
    libvlc_media_player_t* p_mi);
typedef int (*LIBVLC_MEDIA_PLAYER_CAN_PAUSE)(
    libvlc_media_player_t* p_mi);
typedef void (*LIBVLC_MEDIA_PLAYER_PAUSE)(
    libvlc_media_player_t* p_mi);
typedef libvlc_media_t* (*LIBVLC_MEDIA_PLAYER_GET_MEDIA)(
    libvlc_media_player_t* p_mi);

extern LIBVLC_MEDIA_PLAYER_GET_TIME libvlc_media_player_get_time_;
extern LIBVLC_MEDIA_PLAYER_GET_LENGTH libvlc_media_player_get_length_;
extern LIBVLC_MEDIA_PLAYER_GET_STATE libvlc_media_player_get_state_;
extern LIBVLC_MEDIA_PLAYER_CAN_PAUSE libvlc_media_player_can_pause_;
extern LIBVLC_MEDIA_PLAYER_PAUSE libvlc_media_player_pause_;
extern LIBVLC_MEDIA_PLAYER_GET_MEDIA libvlc_media_player_get_media_;

/* libvlc media methods */
typedef char* (*LIBVLC_MEDIA_GET_META)(
    libvlc_media_t* p_md, libvlc_meta_t e_meta);

extern LIBVLC_MEDIA_GET_META libvlc_media_get_meta_;

enum behavior {
    BEHAVIOR_STOP_RESTART,
    BEHAVIOR_PAUSE_UNPAUSE,
    BEHAVIOR_ALWAYS_PLAY,
};

struct vlc_source {
    obs_source_t* source;

    libvlc_media_player_t* media_player;
    libvlc_media_list_player_t* media_list_player;

    struct obs_source_frame frame;
    struct obs_source_audio audio;
    size_t audio_capacity;

    pthread_mutex_t mutex;
    DARRAY(struct media_file_data)
    files;
    enum behavior behavior;
    bool loop;
    bool shuffle;

    obs_hotkey_id play_pause_hotkey;
    obs_hotkey_id restart_hotkey;
    obs_hotkey_id stop_hotkey;
    obs_hotkey_id playlist_next_hotkey;
    obs_hotkey_id playlist_prev_hotkey;
};

#else
bool load_libvlc(void) { return false; }

bool load_libvlc_module() { return false; }
bool load_vlc_funcs() { return false; }
void unload_libvlc() {}
#endif /* DISABLE VLC*/

#ifdef __cplusplus
}
#endif
