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

#include <jansson.h>
#include <obs-module.h>

#define S_PLUGIN_ID "tuna"

/* Translation */

#define T_(s) obs_module_text(s)
#define T_MENU_TUNA T_("tuna.gui.menu")
#define T_SPOTIFY_LOGGEDIN T_("tuna.gui.tab.spotify.loggedin")
#define T_SPOTIFY_LOGGEDOUT T_("tuna.gui.tab.spotify.loggedout")
#define T_SPOTIFY_WARNING T_("tuna.gui.tab.spotify.linkmessage")
#define T_STATUS_RUNNING T_("tuna.gui.tab.basics.status.started")
#define T_STATUS_STOPPED T_("tuna.gui.tab.basics.status.stopped")
#define T_PREVIEW T_("tuna.gui.tab.basics.preview")
#define T_SOURCE_MPD T_("tuna.gui.tab.mpd")
#define T_PLACEHOLDER T_("tuna.config.song.placeholder")
#define T_FORMAT T_("tuna.config.song.format")
#define T_SELECT_SONG_FILE T_("tuna.gui.select.song.file")
#define T_SELECT_COVER_FILE T_("tuna.gui.select.cover.file")
#define T_SELECT_LYRICS_FILE T_("tuna.gui.select.lyrics.file")

#define T_SONG_PATH T_("tuna.gui.tab.basics.song.info")
#define T_SONG_FORMAT T_("tuna.gui.tab.basics.song.format")
#define T_SONG_FORMAT_DEFAULT T_("tuna.config.song.format")

#define T_OUTPUT_ERROR_TITLE T_("tuna.gui.output.edit.dialog.error.title")
#define T_OUTPUT_ERROR T_("tuna.gui.output.edit.dialog.error")

#define FILTER(name, type) name " (" type ");;All Files(*)"

/* Outputs are saved into config folder on linux, but on windows
 * the home directory isn't really used anyways so just save it
 * there */
#if LINUX
#define OUTPUT_FILE ".config/outputs.json"
#else
#define OUTPUT_FILE "outputs.json"
#endif

#define JSON_OUTPUT_PATH_ID "output"
#define JSON_FORMAT_ID "format"

#define STATUS_RETRY_AFTER 429
