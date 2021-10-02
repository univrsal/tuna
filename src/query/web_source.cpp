/*************************************************************************
 * This file is part of tuna
 * github.com/univrsal/tuna
 * Copyright 2021 univrsal <uni@vrsal.de>.
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

#include "web_source.hpp"
#include "../util/constants.hpp"
#include "../util/web_server.hpp"

web_source::web_source()
    : music_source(S_SOURCE_WEB, T_SOURCE_WEB)
{
    m_capabilities = CAP_ARTIST | CAP_TITLE | CAP_ALBUM | CAP_PROGRESS | CAP_TIME_LEFT | CAP_DURATION | CAP_COVER;
}

void web_source::refresh()
{
    begin_refresh();
    m_current.clear();
    web_thread::song_mutex.lock();
    m_current = web_thread::current_song;
    web_thread::song_mutex.unlock();
}

bool web_source::execute_capability(capability)
{
    return false;
}

bool web_source::enabled() const
{
    return true;
}
