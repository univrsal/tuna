/**
 * This file is part of input-overlay
 * which is licensed under the GPL v2.0
 * See LICENSE or http://www.gnu.org/licenses
 * github.com/univrsal/input-overlay
 */
#pragma once

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

#define FILTER(name, type) name " (" type ");;All Files(*)"
