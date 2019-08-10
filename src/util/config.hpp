/**
 * This file is part of tuna
 * which is licensed under the GPL v2.0
 * See LICENSE or http://www.gnu.org/licenses
 * github.com/univrsal/tuna
 */
#pragma once
#include <util/config-file.h>

#define CFG_REGION						"tuna"

#define CFG_RUNNING						"running"
#define CFG_SONG_PATH					"song_path"
#define CFG_COVER_PATH					"cover_path"
#define CFG_LYRICS_PATH					"lyrics_path"
#define CFG_SELECTED_SOURCE				"source"
#define CFG_REFRESH_RATE				"refresh_rate"
#define CFG_SONG_FORMAT					"song_format"
#define CFG_SONG_PLACEHOLDER			"song_placeholder"
#define CFG_DOWNLOAD_COVER				"download_cover"

#define CFG_SPOTIFY_LOGGEDIN			"spotify.login"
#define CFG_SPOTIFY_TOKEN				"spotify.token"
#define CFG_SPOTIFY_REFRESH_TOKEN		"spotify.refresh_token"
#define CFG_SPOTIFY_AUTH_CODE			"spotify.auth_code"
#define CFG_SPOTIFY_TOKEN_TERMINATION	"spotify.token_termination"

#define CFG_MPD_IP						"mpd.ip"
#define CFG_MPD_PORT					"mpd.port"

class spotify_source;
class mpd_source;
class music_source;

namespace config
{
    enum source {
        src_spotify,
        src_window_title,
        src_mpd,
        src_count
    };

    extern music_source* selected_source;
    extern config_t* instance;
    extern spotify_source* spotify;
    extern mpd_source* mpd;

    /* Temp storage for config values */
    extern uint16_t refresh_rate;
    extern const char* format_string;
    extern const char* placeholder;
    extern const char* cover_path;
    extern const char* lyrics_path;
    extern const char* song_path;
    extern bool download_cover;

    void load_gui_values();
    void init_default();
    void load();
    void save();
    void close();
    void select_source(source s);
}
