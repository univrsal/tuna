/*************************************************************************
 * This file is part of tuna
 * github.con/univrsal/tuna
 * Copyright 2020 univrsal <universailp@web.de>.
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

#include <obs-module.h>

/* clang-format off */

#define S_PLUGIN_ID 			"tuna"
#define S_SOURCE_SPOTIFY		"spotify"
#define S_SOURCE_MPD			"mpd"
#define S_SOURCE_VLC			"vlc"
#define S_SOURCE_WINDOW_TITLE	"window"

#define S_PROGRESS_FG			"fg"
#define S_PROGRESS_BG			"bg"
#define S_PROGRESS_CX			"cx"
#define S_PROGRESS_CY			"cy"
#define S_PROGRESS_ID           "progress_bar"
#define S_PROGRESS_USE_BG		"use_bg"

/* Translation */
#define T_(s) 					obs_module_text(s)
#define T_MENU_TUNA 			T_("tuna.gui.menu")
#define T_SPOTIFY_LOGGEDIN 		T_("tuna.gui.tab.spotify.loggedin")
#define T_SPOTIFY_LOGGEDOUT 	T_("tuna.gui.tab.spotify.loggedout")
#define T_SPOTIFY_WARNING 		T_("tuna.gui.tab.spotify.linkmessage")
#define T_STATUS_RUNNING 		T_("tuna.gui.tab.basics.status.started")
#define T_STATUS_STOPPED 		T_("tuna.gui.tab.basics.status.stopped")
#define T_PREVIEW 				T_("tuna.gui.tab.basics.preview")
#define T_SOURCE_MPD 			T_("tuna.gui.tab.mpd")
#define T_SOURCE_SPOTIFY		T_("tuna.gui.tab.spotify")
#define T_SOURCE_VLC			T_("tuna.gui.tab.vlc")
#define T_SOURCE_WINDOW_TITLE	T_("tuna.gui.tab.window_title")
#define T_PLACEHOLDER 			T_("tuna.config.song.placeholder")
#define T_FORMAT 				T_("tuna.config.song.format")
#define T_SELECT_SONG_FILE 		T_("tuna.gui.select.song.file")
#define T_SELECT_COVER_FILE 	T_("tuna.gui.select.cover.file")
#define T_SELECT_LYRICS_FILE 	T_("tuna.gui.select.lyrics.file")
#define T_SELECT_MPD_FOLDER		T_("tuna.gui.select.mpd.folder")

#define T_SONG_PATH 			T_("tuna.gui.tab.basics.song.info")
#define T_SONG_FORMAT 			T_("tuna.gui.tab.basics.song.format")
#define T_SONG_FORMAT_DEFAULT 	T_("tuna.config.song.format")

#define T_OUTPUT_ERROR_TITLE 	T_("tuna.gui.output.edit.dialog.error.title")
#define T_OUTPUT_ERROR 			T_("tuna.gui.output.edit.dialog.error")

#define T_VLC_NONE 				T_("tuna.gui.vlc.none")
#define T_VLC_VERSION_ISSUE		T_("tuna.gui.vlc.issue.message")
#define T_ERROR_TITLE			T_("tuna.gui.issue.title")

#define T_PROGRESS_FG			T_("tuna.source.progress.color.fg")
#define T_PROGRESS_BG			T_("tuna.source.progress.color.bg")
#define T_PROGRESS_CX			T_("tuna.source.progress.cx")
#define T_PROGRESS_CY			T_("tuna.source.progress.cy")
#define T_PROGRESS_NAME			T_("tuna.source.progress.name")
#define T_PROGRESS_USE_BG		T_("tuna.source.progress.use.bg")

#define FILTER(name, type) 		name " (" type ");;All Files(*)"

/* Outputs are saved into config folder on linux, but on windows
 * the home directory isn't really used anyways so just save it
 * there */
#if UNIX
#define OUTPUT_FILE ".config/outputs.json"
#else
#define OUTPUT_FILE "outputs.json"
#endif

#define JSON_OUTPUT_PATH_ID 	"output"
#define JSON_FORMAT_ID 			"format"

#define STATUS_RETRY_AFTER 		429
#define HTTP_NO_CONTENT			204
#define HTTP_OK					200

/* clang-format on */
