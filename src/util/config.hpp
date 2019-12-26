/*************************************************************************
 * This file is part of tuna
 * github.con/univrsal/tuna
 * Copyright 2019 univrsal <universailp@web.de>.
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
#include <QPair>
#include <QString>
#include <util/config-file.h>

/* Config macros */
#define CDEF_STR(id, value) \
    config_set_default_string(config::instance, CFG_REGION, id, value)
#define CDEF_INT(id, value) \
    config_set_default_int(config::instance, CFG_REGION, id, value)
#define CDEF_UINT(id, value) \
    config_set_default_uint(config::instance, CFG_REGION, id, value)
#define CDEF_BOOL(id, value) \
    config_set_default_bool(config::instance, CFG_REGION, id, value)

#define CGET_STR(id) config_get_string(config::instance, CFG_REGION, id)
#define CGET_INT(id) config_get_int(config::instance, CFG_REGION, id)
#define CGET_UINT(id) config_get_uint(config::instance, CFG_REGION, id)
#define CGET_BOOL(id) config_get_bool(config::instance, CFG_REGION, id)

#define CSET_STR(id, value) \
    config_set_string(config::instance, CFG_REGION, id, value)
#define CSET_INT(id, value) \
    config_set_int(config::instance, CFG_REGION, id, value)
#define CSET_UINT(id, value) \
    config_set_uint(config::instance, CFG_REGION, id, value)
#define CSET_BOOL(id, value) \
    config_set_bool(config::instance, CFG_REGION, id, value)

#define CFG_REGION "tuna"

#define CFG_RUNNING "running"
#define CFG_SONG_PATH "song_path"
#define CFG_COVER_PATH "cover_path"
#define CFG_LYRICS_PATH "lyrics_path"
#define CFG_SELECTED_SOURCE "source"
#define CFG_REFRESH_RATE "refresh_rate"
#define CFG_SONG_FORMAT "song_format"
#define CFG_SONG_PLACEHOLDER "song_placeholder"
#define CFG_DOWNLOAD_COVER "download_cover"

#define CFG_SPOTIFY_LOGGEDIN "spotify.login"
#define CFG_SPOTIFY_TOKEN "spotify.token"
#define CFG_SPOTIFY_REFRESH_TOKEN "spotify.refresh_token"
#define CFG_SPOTIFY_AUTH_CODE "spotify.auth_code"
#define CFG_SPOTIFY_TOKEN_TERMINATION "spotify.token_termination"

#define CFG_MPD_IP "mpd.ip"
#define CFG_MPD_PORT "mpd.port"
#define CFG_MPD_LOCAL "mpd.local"

#define CFG_VLC_ID "vlc.id"

#define CFG_WINDOW_TITLE "window.title"
#define CFG_WINDOW_PAUSE "window.title.pause"
#define CFG_WINDOW_SEARCH "window.search"
#define CFG_WINDOW_REPLACE "window.replace"
#define CFG_WINDOW_CUT_BEGIN "window.cut.begin"
#define CFG_WINDOW_CUT_END "window.cut.end"
#define CFG_WINDOW_REGEX "window.regex"

class spotify_source;

class window_source;

class mpd_source;

class vlc_obs_source;

class music_source;

namespace config {
enum source {
    src_spotify,
    src_window_title,
    src_vlc_obs,
    src_mpd,
    src_count
};

extern music_source* selected_source;
extern config_t* instance;
extern vlc_obs_source* vlc_obs;
extern spotify_source* spotify;
extern window_source* window;
extern mpd_source* mpd;

/* Temp storage for config values */
extern uint16_t refresh_rate;
extern const char* placeholder;
extern const char* cover_path;
extern const char* lyrics_path;
extern QList<QPair<QString, QString>> outputs;
extern const char* cover_placeholder;
extern bool download_cover;

void load_gui_values();

void init_default();

void load();

void save();

void close();

void select_source(source s);

void load_outputs(QList<QPair<QString, QString>>& table_content);

void save_outputs(const QList<QPair<QString, QString>>& table_content);
} // namespace config
