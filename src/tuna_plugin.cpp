/*************************************************************************
 * This file is part of tuna
 * git.vrsal.xyz/alex/tuna
 * Copyright 2022 univrsal <uni@vrsal.xyz>.
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

#include "gui/music_control.hpp"
#include "gui/tuna_gui.hpp"
#include "gui/widgets/lastfm.hpp"
#include "query/vlc_obs_source.hpp"
#include "source/progress.hpp"
#include "util/config.hpp"
#include "util/constants.hpp"
#include "util/format.hpp"
#include "util/tuna_thread.hpp"
#include "util/utility.hpp"
#include <QAction>
#include <QMainWindow>
#include <obs-frontend-api.h>
#include <obs-module.h>

OBS_DECLARE_MODULE()

OBS_MODULE_USE_DEFAULT_LOCALE(S_PLUGIN_ID, "en-US")

MODULE_EXPORT const char* obs_module_description(void)
{
    return "Song information plugin";
}

obs_hotkey_id vlc_next, vlc_prev;

void register_gui()
{
    /* UI registration from
     * https://github.com/Palakis/obs-websocket/
     */
    const auto menu_action = static_cast<QAction*>(obs_frontend_add_tools_menu_qaction(T_MENU_TUNA));
    obs_frontend_push_ui_translation(obs_module_get_string);
    const auto main_window = static_cast<QMainWindow*>(obs_frontend_get_main_window());
    tuna_dialog = new tuna_gui(main_window);
    obs_frontend_pop_ui_translation();

    const auto menu_cb = [] { tuna_dialog->toggleShowHide(); };
    QAction::connect(menu_action, &QAction::triggered, menu_cb);

    /* Register dock */
#ifndef __APPLE__
    obs_frontend_push_ui_translation(obs_module_get_string);
    music_dock = new music_control(main_window);
    obs_frontend_add_dock(music_dock);
    obs_frontend_pop_ui_translation();
#endif
}

void tuna_save_cb(obs_data_t* save_data, bool saving, void*)
{
    obs_data_t* data = nullptr;
    obs_data_array_t *prev = nullptr, *next = nullptr;

    if (saving) {
        data = obs_data_create();
        prev = obs_hotkey_save(vlc_prev);
        next = obs_hotkey_save(vlc_next);
        obs_data_set_array(data, "vlc_prev_hotkey", prev);
        obs_data_set_array(data, "vlc_next_hotkey", next);
        obs_data_set_obj(save_data, "tuna", data);
        obs_data_array_release(prev);
        obs_data_array_release(next);
        obs_data_release(data);
    } else {
        data = obs_data_get_obj(save_data, "tuna");
        if (!data)
            data = obs_data_create();
        prev = obs_data_get_array(data, "vlc_prev_hotkey");
        next = obs_data_get_array(data, "vlc_next_hotkey");
        obs_hotkey_load(vlc_prev, prev);
        obs_hotkey_load(vlc_next, next);
        obs_data_array_release(prev);
        obs_data_array_release(next);
        obs_data_release(data);
    }
}

bool obs_module_load()
{
    binfo("Loading v%s (build time %s). Qt version: compile-time: %s, run-time: %s. libobs: compile-time: %s, run-time: %s",
        TUNA_VERSION, BUILD_TIME, QT_VERSION_STR, qVersion(), OBS_VERSION_CANONICAL, obs_get_version_string());
    config::init();
    register_gui();
    format::init();
    music_sources::init();
    config::load();
    obs_sources::register_progress();
    obs_frontend_add_save_callback(&tuna_save_cb, nullptr);

    obs_frontend_add_event_callback([](enum obs_frontend_event event, void*) {
        if (event == OBS_FRONTEND_EVENT_EXIT)
            tuna_thread::stop();
    },
        nullptr);
    return true;
}

void vlc_next_cb(void*, obs_hotkey_id id, obs_hotkey_t*, bool pressed)
{
    if (id != vlc_next || !pressed)
        return;
    auto src = music_sources::get<vlc_obs_source>(S_SOURCE_VLC);
    if (src && src->enabled())
        src->next_vlc_source();
}

void vlc_prev_cb(void*, obs_hotkey_id id, obs_hotkey_t*, bool pressed)
{
    if (id != vlc_prev || !pressed)
        return;
    auto src = music_sources::get<vlc_obs_source>(S_SOURCE_VLC);
    if (src && src->enabled())
        src->prev_vlc_source();
}

void obs_module_post_load()
{
    // Just tries to create a vlc source, preferably with a name that isn't already taken
    obs_source_t* vlc_source = obs_source_create("vlc_source", "tuna_module_load_vlc_presence_test_source", nullptr, nullptr);
    util::have_vlc_source = vlc_source != nullptr;
    obs_source_release(vlc_source);

    if (util::have_vlc_source) {
        vlc_next = obs_hotkey_register_frontend(S_HOTKEY_NEXT, T_HOTKEY_VLC_NEXT,
            vlc_next_cb, nullptr);
        vlc_prev = obs_hotkey_register_frontend(S_HOTKEY_PREV, T_HOTKEY_VLC_PREV,
            vlc_prev_cb, nullptr);
    }
    config::post_load = true;
}

void obs_module_unload()
{
    config::close();
}
