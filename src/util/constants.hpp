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
