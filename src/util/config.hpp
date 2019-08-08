/**
 * This file is part of tuna
 * which is licensed under the GPL v2.0
 * See LICENSE or http://www.gnu.org/licenses
 * github.com/univrsal/tuna
 */
#pragma once

#define CFG_REGION_SOURCES				"music_sources"
#define CFG_REGION_BASICS				"basics"
#define CFG_REGION_SPOTIFY				"spotify"

#define CFG_SPOTIFY_ENABLED				"spotify_enabled"
#define CFG_MPD_ENABLED					"mpd_enabled"
#define CFG_WINDOW_TITLE_ENABLED		"window_title_enabled"

#define CFG_SONG_PATH					"song_path"
#define CFG_COVER_PATH					"cover_path"

#define CFG_SPOTIFY_LOGGED_IN			"login"
#define CFG_SPOTIFY_TOKEN				"token"
struct config_t;

namespace config
{
    extern config_t* instance;

    void init_default();
    bool load(const char* path);
    bool save();
    void close();

}
