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

#include "gui/music_control.hpp"
#include "gui/tuna_gui.hpp"
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

#define TUNA_VERSION "1.4.2"

OBS_DECLARE_MODULE()

OBS_MODULE_USE_DEFAULT_LOCALE(S_PLUGIN_ID, "en-US")

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

    obs_frontend_push_ui_translation(obs_module_get_string);
    auto* tmp = new music_Control(main_window);

    music_control = reinterpret_cast<music_Control*>(obs_frontend_add_dock(tmp));
    obs_frontend_pop_ui_translation();
}

bool obs_module_load()
{
    binfo("Loading v%s", TUNA_VERSION);

    config::init();
    util::load_vlc();
    register_gui();
    format::init();
    music_sources::init();
    config::load();
    obs_sources::register_progress();
    return true;
}

void obs_module_unload()
{
    thread::thread_mutex.lock();
    util::unload_vlc();
    thread::thread_mutex.unlock();
    config::close();
}
