/**
 * This file is part of tuna
 * which is licensed under the GPL v2.0
 * See LICENSE or http://www.gnu.org/licenses
 * github.com/univrsal/tuna
 */
#pragma once
#include <util/config-file.h>

#define CFG_REGION						"tuna"

#define CFG_MPD_ENABLED					"mpd_enabled"
#define CFG_WINDOW_TITLE_ENABLED		"window_title_enabled"

#define CFG_SONG_PATH					"song_path"
#define CFG_COVER_PATH					"cover_path"

#define CFG_SPOTIFY_ENABLED				"spotify_enabled"
#define CFG_SPOTIFY_LOGGEDIN			"login"
#define CFG_SPOTIFY_TOKEN				"token"
#define CFG_SPOTIFY_REFRESH_TOKEN		"refresh_token"
#define CFG_SPOTIFY_AUTH_CODE			"auth_code"
#define CFG_SPOTIFY_TOKEN_TERMINATION	"token_termination"

class spotify_source;

namespace config
{
    extern config_t* instance;
    extern spotify_source* spotify;

    void init_default();
    void load();

}
