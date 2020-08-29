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
    m_title = "n/a";
    m_album = "n/a";
    m_cover = "n/a";
    m_lyrics = "n/a";
    m_artists.clear();
    m_disc_number = 0;
    m_track_number = 0;
    m_duration_ms = 0;
    m_progress_ms = 0;
    m_is_playing = state_unknown;
    m_is_explicit = false;
    m_release_precision = prec_unknown;
    m_day = "";
    m_month = "";
    m_year = "";
    m_full_release = "";
}

void song::update_release_precision()
{
    if (!m_day.isEmpty() && !m_month.isEmpty() && !m_year.isEmpty()) {
        m_release_precision = prec_day;
        m_full_release = m_year + "." + m_month + "." + m_day;
    } else if (!m_month.isEmpty() && !m_year.isEmpty()) {
        m_release_precision = prec_month;
        m_full_release = m_year + "." + m_month;
    } else if (!m_year.isEmpty()) {
        m_release_precision = prec_year;
        m_full_release = m_year;
    } else {
        m_release_precision = prec_unknown;
        m_full_release = "";
    }
}

void song::append_artist(const QString& a)
{
    if (!a.isEmpty())
        m_artists.append(a);
    if (!m_artists.isEmpty())
        m_data |= CAP_ARTIST;
}

void song::set_label(const QString& l)
{
    if (!l.isEmpty())
        m_data |= CAP_LABEL;
    m_label = l;
}

void song::set_cover_link(const QString& link)
{
    if (!link.isEmpty())
        m_data |= CAP_COVER;
    m_cover = link;
}

void song::set_title(const QString& title)
{
    if (!title.isEmpty())
        m_data |= CAP_TITLE;
    m_title = title;
}

void song::set_duration(int ms)
{
    if (ms > 0)
        m_data |= CAP_DURATION;
    m_duration_ms = ms;
}

void song::set_progress(int ms)
{
    m_data |= CAP_PROGRESS;
    m_progress_ms = ms;
}

void song::set_album(const QString& album)
{
    if (!album.isEmpty())
        m_data |= CAP_ALBUM;
    m_album = album;
}

void song::set_explicit(bool e)
{
    m_data |= CAP_EXPLICIT;
    m_is_explicit = e;
}

void song::set_state(play_state p)
{
    m_data |= CAP_STATUS;
    m_is_playing = p;
}

void song::set_disc_number(int i)
{
    if (i > 0)
        m_data |= CAP_DISC_NUMBER;
    m_disc_number = i;
}

void song::set_track_number(int i)
{
    if (i > 0)
        m_data |= CAP_TRACK_NUMBER;
    m_track_number = i;
}

void song::set_year(const QString& y)
{
    m_data |= CAP_RELEASE;
    m_year = y;
    update_release_precision();
}

void song::set_month(const QString& m)
{
    m_data |= CAP_RELEASE;
    m_month = m;
    update_release_precision();
}

void song::set_day(const QString& d)
{
    m_data |= CAP_RELEASE;
    m_day = d;
    update_release_precision();
}

const QString& song::get_string_value(char specifier) const
{
    static QString empty("");

    switch (specifier) {
    case 't':
        return m_title;
    case 'a':
        return m_album;
    case 'r':
        return m_full_release;
    case 'y':
        return m_year;
    case 'b':
        return m_label;
    default:
        return empty;
    }
}

int32_t song::get_int_value(char specifier) const
{
    switch (specifier) {
    case 'd':
        return m_disc_number;
    case 'a':
        return m_track_number;
    case 'p':
        return m_progress_ms;
    case 'l':
        return m_duration_ms;
    case 'o':
        if (m_data & CAP_TIME_LEFT)
            return m_duration_ms - m_progress_ms;
        /* Fallthrough */
    default:
        return 0;
    }
}

bool song::operator==(const song& other) const
{
    /* basically compare all data that shouldn't change in between
     * updates, unless the song changes
     */
    return state() == other.state() && data() == other.data() && cover() == other.cover() && label() == other.label() && get_int_value('d') == other.get_int_value('d') && get_int_value('a') == other.get_int_value('a') && get_int_value('l') == other.get_int_value('l') && get_string_value('t') == other.get_string_value('t') && get_string_value('a') == other.get_string_value('a') && get_string_value('y') == other.get_string_value('y') && get_string_value('b') == other.get_string_value('b') && get_string_value('r') == other.get_string_value('r');
}

bool song::operator!=(const song& other) const
{
    return !((*this) == other);
}
