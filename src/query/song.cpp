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

#include "song.hpp"
#include "music_source.hpp"

song::song()
{
    clear();
}

void song::clear()
{
    m_data = 0x0;
    m_title = "";
    m_album = "";
    m_cover = "";
    m_lyrics = "";
    m_artists.clear();
    m_disc_number = 0;
    m_track_number = 0;
    m_duration_ms = 0;
    m_progress_ms = 0;
    m_is_playing = false;
    m_is_explicit = false;
    m_day = "";
    m_month = "";
    m_year = "";
}

void song::update_release_precision()
{
    if (!m_day.isEmpty() && !m_month.isEmpty() && !m_year.isEmpty()) {
        m_release_precision = prec_day;
    } else if (!m_month.isEmpty() && !m_year.isEmpty()) {
        m_release_precision = prec_month;
    } else if (!m_year.isEmpty()) {
        m_release_precision = prec_year;
    } else {
        m_release_precision = prec_unkown;
    }
}

void song::append_artist(const QString &a)
{
    m_data |= CAP_ARTIST;
    m_artists.append(a);
}

void song::set_cover_link(const QString &link)
{
    m_data |= CAP_COVER;
    m_cover = link;
}

void song::set_title(const QString &title)
{
    m_data |= CAP_TITLE;
    m_title = title;
}

void song::set_duration(int ms)
{
    m_data |= CAP_LENGTH;
    m_duration_ms = ms;
}

void song::set_progress(int ms)
{
    m_data |= CAP_PROGRESS;
    m_progress_ms = ms;
}

void song::set_album(const QString &album)
{
    m_data |= CAP_ALBUM;
    m_album = album;
}

void song::set_explicit(bool e)
{
    m_data |= CAP_EXPLICIT;
    m_is_explicit = e;
}

void song::set_playing(bool p)
{
    m_data |= CAP_STATUS;
    m_is_playing = p;
}

void song::set_disc_number(int i)
{
    m_data |= CAP_DISC_NUMBER;
    m_disc_number = i;
}

void song::set_track_number(int i)
{
    m_data |= CAP_TRACK_NUMBER;
    m_track_number = i;
}

void song::set_year(const QString &y)
{
    m_data |= CAP_RELEASE;
    m_year = y;
    update_release_precision();
}

void song::set_month(const QString &m)
{
    m_data |= CAP_RELEASE;
    m_month = m;
    update_release_precision();
}

void song::set_day(const QString &d)
{
    m_data |= CAP_RELEASE;
    m_day = d;
    update_release_precision();
}
