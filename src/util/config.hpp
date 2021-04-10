/*************************************************************************
 * This file is part of tuna
 * github.com/univrsal/tuna
 * Copyright 2020 univrsal <uni@vrsal.cf>.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 2 of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *************************************************************************/

#pragma once

#include <QList>
#include <QString>
#include <util/config-file.h>

/* Config macros */
#define CDEF_STR(id, value) config_set_default_string(config::instance, CFG_REGION, id, value)
#define CDEF_INT(id, value) config_set_default_int(config::instance, CFG_REGION, id, value)
#define CDEF_UINT(id, value) config_set_default_uint(config::instance, CFG_REGION, id, value)
#define CDEF_BOOL(id, value) config_set_default_bool(config::instance, CFG_REGION, id, value)

#define CGET_STR(id) config_get_string(config::instance, CFG_REGION, id)
#define CGET_INT(id) config_get_int(config::instance, CFG_REGION, id)
#define CGET_UINT(id) config_get_uint(config::instance, CFG_REGION, id)
#define CGET_BOOL(id) config_get_bool(config::instance, CFG_REGION, id)

#define CSET_STR(id, value) config_set_string(config::instance, CFG_REGION, id, value)
#define CSET_INT(id, value) config_set_int(config::instance, CFG_REGION, id, value)
#define CSET_UINT(id, value) config_set_uint(config::instance, CFG_REGION, id, value)
#define CSET_BOOL(id, value) config_set_bool(config::instance, CFG_REGION, id, value)

/* clang-format off */

#define CFG_REGION 						"tuna"

#define CFG_SERVER_PORT                 "server_port"
#define CFG_SERVER_ENABLED              "server_enabled"

#define CFG_RUNNING 					"running"
#define CFG_SONG_PATH 					"song_path"
#define CFG_COVER_PATH 					"cover_path"
#define CFG_PLACEHOLDER_WHEN_PAUSED     "placeholder_when_paused"
#define CFG_LYRICS_PATH 				"lyrics_path"
#define CFG_SELECTED_SOURCE 			"music.source"
#define CFG_REFRESH_RATE 				"refresh_rate"
#define CFG_SONG_FORMAT 				"song_format"
#define CFG_SONG_PLACEHOLDER 			"song_placeholder"
#define CFG_DOWNLOAD_COVER 				"download_cover"
#define CFG_REMOVE_EXTENSIONS           "removeextensions"

#define CFG_SPOTIFY_LOGGEDIN 			"spotify.login"
#define CFG_SPOTIFY_TOKEN 				"spotify.token"
#define CFG_SPOTIFY_REFRESH_TOKEN 		"spotify.refresh_token"
#define CFG_SPOTIFY_AUTH_CODE 			"spotify.auth_code"
#define CFG_SPOTIFY_TOKEN_TERMINATION 	"spotify.token_termination"
#define CFG_SPOTIFY_CLIENT_ID           "spotify.client_id"
#define CFG_SPOTIFY_CLIENT_SECRET       "spotify.client_secret"

#define CFG_DEEZER_CLIENT_ID            "deezer.client.id"
#define CFG_DEEZER_CLIENT_SECRET        "deezer.client.secret"
#define CFG_DEEZER_LOGGEDIN             "deezer.login"
#define CFG_DEEZER_TOKEN                "deezer.token"
#define CFG_DEEZER_AUTH_CODE            "deezer.auth_code"

#define CFG_MPD_IP 						"mpd.ip"
#define CFG_MPD_PORT 					"mpd.port"
#define CFG_MPD_LOCAL 					"mpd.local"
#define CFG_MPD_BASE_FOLDER				"mpd.base.folder"

#define CFG_LASTFM_USERNAME             "lastfm.username"
#define CFG_LASTFM_API_KEY              "lastfm.apikey"

#define CFG_VLC_ID 						"vlc.id"
#define CFG_FORCE_VLC_DECISION			"vlc.force.enable"
#define CFG_ERROR_MESSAGE_SHOWN			"vlc.error.message.shown"

#define CFG_WINDOW_TITLE 				"window.title"
#define CFG_WINDOW_PAUSE 				"window.title.pause"
#define CFG_WINDOW_SEARCH 				"window.search"
#define CFG_WINDOW_REPLACE 				"window.replace"
#define CFG_WINDOW_CUT_BEGIN 			"window.cut.begin"
#define CFG_WINDOW_CUT_END 				"window.cut.end"
#define CFG_WINDOW_REGEX 				"window.regex"
#define CFG_WINDOW_USE_PROCRESS         "window.use.process"
#define CFG_WINDOW_PROCESS_NAME         "window.process.name"

#define CFG_DOCK_GEOMETRY				"dock_geometry"
#define CFG_DOCK_VISIBLE				"dock_visible"
#define CFG_DOCK_INFO_VISIBLE			"dock_info_visible"
#define CFG_DOCK_VOLUME_VISIBLE			"dock_volume_visible"
/* clang-format on */

namespace config {
struct output {
	QString format;
	QString path;
	QString last_output;
	bool log_mode;
};

extern config_t *instance;

/* Temp storage for config values */
extern uint16_t refresh_rate;
extern const char *placeholder;
extern const char *selected_source;
extern const char *cover_path;
extern const char *lyrics_path;
extern QList<output> outputs;
extern const char *cover_placeholder;
extern bool download_cover;
extern bool placeholder_when_paused;

void init();

void load();

void close();

void load_outputs();

void save_outputs();
} // namespace config
