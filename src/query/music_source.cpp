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
#include "../util/cover_tag_handler.hpp"
#include "../util/tuna_thread.hpp"
#include "../util/utility.hpp"
#include "mpd_source.hpp"
#include "spotify_source.hpp"
#include "vlc_obs_source.hpp"
#include "window_source.hpp"
#include <exception>

namespace music_sources {
int selected_index = 0;
QList<std::shared_ptr<music_source>> instances;

void init()
{
    instances.append(std::make_shared<spotify_source>());
    instances.append(std::make_shared<mpd_source>());
    instances.append(std::make_shared<vlc_obs_source>());
    instances.append(std::make_shared<window_source>());
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
    thread::mutex.lock();
    int i = 0;
    for (auto src : instances) {
        if (strcmp(src->id(), id) == 0) {
            selected_index = i;
            break;
        }
        i++;
    }
    /* ensure cover will be refreshed after changing source */
    util::download_cover(nullptr, true);
#ifdef LINUX
    cover::find_embedded_cover("", true);
#endif
    thread::mutex.unlock();
}

void set_gui_values()
{
    for (auto src : instances)
        src->set_gui_values();
}

std::shared_ptr<music_source> selected_source()
{
    return std::shared_ptr<music_source>(instances[selected_index]);
}

void deinit()
{
    /* check if all source references were decreased correctly */
    for (int i = 0; i < instances.count(); i++) {
        if (instances[i].use_count() > 1) {
            berr("Shared pointer of source %s is still in use!"
                 " (use count: %li)",
                instances[i]->id(),
                instances[i].use_count());
        }
    }
    instances.clear();
}
}
