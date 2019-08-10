/**
 * This file is part of input-overlay
 * which is licensed under the GPL v2.0
 * See LICENSE or http://www.gnu.org/licenses
 * github.com/univrsal/input-overlay
 */
#pragma once
#include <obs-module.h>

#define S_PLUGIN_ID			"tuna"

/* Translation */

#define _T(s)				obs_module_text(s)
#define T_MENU_TUNA			_T("tuna.gui.menu")
#define T_SPOTIFY_LOGGEDIN	_T("tuna.gui.tab.spotify.loggedin")
#define T_SPOTIFY_LOGGEDOUT _T("tuna.gui.tab.spotify.loggedout")
#define T_SPOTIFY_WARNING	_T("tuna.gui.tab.spotify.linkmessage")
#define T_STATUS_RUNNING	_T("tuna.gui.tab.basics.status.started")
#define T_STATUS_STOPPED	_T("tuna.gui.tab.basics.status.stopped")
#define T_PREVIEW			_T("tuna.gui.tab.basics.preview")
#define T_SOURCE_MPD		_T("tuna.gui.tab.mpd")
#define T_PLACEHOLDER		_T("tuna.config.song.placeholder")
#define T_FORMAT			_T("tuna.config.song.format")
