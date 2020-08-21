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

#include "music_source.hpp"
#include "../gui/tuna_gui.hpp"
#include "../util/cover_tag_handler.hpp"
#include "../util/tuna_thread.hpp"
#include "../util/utility.hpp"
#include "gpmdp_source.hpp"
#include "lastfm_source.hpp"
#include "mpd_source.hpp"
#include "spotify_source.hpp"
#include "vlc_obs_source.hpp"
#include "window_source.hpp"
#include <obs-frontend-api.h>

namespace music_sources {
static int selected_index = -1;
QList<std::shared_ptr<music_source>> instances;

void init()
{
    obs_frontend_push_ui_translation(obs_module_get_string);
    instances.append(std::make_shared<spotify_source>());
    instances.append(std::make_shared<mpd_source>());
    instances.append(std::make_shared<vlc_obs_source>());
    instances.append(std::make_shared<window_source>());
    instances.append(std::make_shared<lastfm_source>());
    instances.append(std::make_shared<gpmdp_source>());
    obs_frontend_pop_ui_translation();
}

void load()
{
    for (auto& src : instances)
        src->load();
}

void save()
{
    for (auto& src : instances)
        src->save();
}

void select(const char* id)
{
    if (!id)
        return;
    thread::thread_mutex.lock();
    int i = 0;
    for (auto src : instances) {
        if (strcmp(src->id(), id) == 0) {
            selected_index = i;
            break;
        }
        i++;
    }
    /* ensure cover will be refreshed after changing source */
    song s;
    util::download_cover(s, true);
    util::set_placeholder(true);
    cover::find_embedded_cover("", true);
    thread::thread_mutex.unlock();
}

void set_gui_values()
{
    for (const auto& src : instances)
        src->set_gui_values();
}

std::shared_ptr<music_source> selected_source_unsafe()
{
    if (selected_index >= 0) {
        auto ref = std::shared_ptr<music_source>(instances[selected_index]);
        return ref;
    }
    return nullptr;
}

std::shared_ptr<music_source> selected_source()
{
    if (selected_index >= 0) {
        thread::thread_mutex.lock();
        auto ref = std::shared_ptr<music_source>(instances[selected_index]);
        thread::thread_mutex.unlock();
        return ref;
    }
    return nullptr;
}

void deinit()
{
    /* check if all source references were decreased correctly */
    for (int i = 0; i < instances.count(); i++) {
        if (instances[i].use_count() > 1) {
            berr("Shared pointer of source %s is still in use!"
                 " (use count: %li)",
                instances[i]->id(), instances[i].use_count());
        }
    }
    instances.clear();
}
}

music_source::music_source(const char* id, const char* name, source_widget* w)
    : m_id(id)
    , m_name(name)
    , m_settings_tab(w)
{
    binfo("Registered %s (id: %s)", name, id);
    emit tuna_dialog->source_registered(name, id, w);
}

void music_source::save()
{
    if (m_settings_tab)
        m_settings_tab->save_settings();
}

void music_source::set_gui_values()
{
    if (m_settings_tab)
        m_settings_tab->load_settings();
}
