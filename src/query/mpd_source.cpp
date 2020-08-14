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

#include "mpd_source.hpp"
#include "../gui/music_control.hpp"
#include "../gui/tuna_gui.hpp"
#include "../gui/widgets/mpd.hpp"
#include "../util/config.hpp"
#include "../util/cover_tag_handler.hpp"
#include "../util/utility.hpp"
#include <obs-module.h>
#include <taglib/fileref.h>

mpd_source::mpd_source()
    : music_source(S_SOURCE_MPD, T_SOURCE_MPD, new mpd)
{
    m_capabilities = CAP_TITLE | CAP_ALBUM | CAP_LABEL | CAP_STOP_SONG | CAP_PROGRESS | CAP_VOLUME_UP | CAP_VOLUME_DOWN | CAP_VOLUME_MUTE | CAP_DURATION | CAP_PLAY_PAUSE | CAP_NEXT_SONG | CAP_PREV_SONG | CAP_COVER;
    m_address = nullptr;
    m_connection = nullptr;
    m_port = 0;
    m_connected = false;
}

mpd_source::~mpd_source()
{
    if (m_connection)
        mpd_connection_free(m_connection);
    if (m_status)
        mpd_status_free(m_status);
    if (m_mpd_song)
        mpd_song_free(m_mpd_song);
    m_mpd_song = nullptr;
    m_connection = nullptr;
    m_status = nullptr;
}

void mpd_source::disconnect()
{
    if (m_connected) {
        mpd_connection_free(m_connection);
        m_connection = nullptr;
    }
    m_connected = false;
}

void mpd_source::connect()
{
    bdebug("Opening MPD connection to %s:%hu", qt_to_utf8(m_address), m_port);
    disconnect();
    if (m_local)
        m_connection = mpd_connection_new(nullptr, 0, 0);
    else
        m_connection = mpd_connection_new(qt_to_utf8(m_address), m_port, 2000);

    if (mpd_connection_get_error(m_connection) != MPD_ERROR_SUCCESS) {
        berr("mpd connection to %s:%hu failed with error %s", qt_to_utf8(m_address), m_port,
            mpd_connection_get_error_message(m_connection));
        mpd_connection_free(m_connection);
        m_connection = nullptr;
    } else {
        m_connected = true;
        mpd_connection_set_keepalive(m_connection, true);
    }
}

bool mpd_source::enabled() const
{
    return true;
}

void mpd_source::load()
{
    CDEF_INT(CFG_MPD_PORT, 0);
    CDEF_STR(CFG_MPD_IP, "localhost");
    CDEF_BOOL(CFG_MPD_LOCAL, true);
    CDEF_STR(CFG_MPD_BASE_FOLDER, "");

    m_address = utf8_to_qt(CGET_STR(CFG_MPD_IP));
    m_base_folder = utf8_to_qt(CGET_STR(CFG_MPD_BASE_FOLDER));
    m_port = CGET_UINT(CFG_MPD_PORT);
    m_local = CGET_BOOL(CFG_MPD_LOCAL);
}

void mpd_source::save()
{
    CSET_STR(CFG_MPD_IP, qt_to_utf8(m_address));
    CSET_STR(CFG_MPD_BASE_FOLDER, qt_to_utf8(m_base_folder));
    CSET_UINT(CFG_MPD_PORT, m_port);
    CSET_BOOL(CFG_MPD_LOCAL, m_local);
}

void mpd_source::refresh()
{
    if (!m_connected)
        connect();
    if (!m_connected) {
        util::set_placeholder(true);
        return;
    }

    bool old_state = m_current.playing();
    m_current.clear();

    m_status = mpd_run_status(m_connection);
    m_mpd_song = mpd_run_current_song(m_connection);

    if (m_status) {
        bool new_state = mpd_status_get_state(m_status) == MPD_STATE_PLAY;
        m_current.set_progress(mpd_status_get_elapsed_ms(m_status));
        if (new_state != old_state)
            util::set_placeholder(!new_state);
        m_current.set_playing(new_state);
    }

/* Thanks ubuntu for using ancient packages */
#define MPD_TAG_LABEL (mpd_tag_type)21
    if (m_mpd_song) {
        const char* title = mpd_song_get_tag(m_mpd_song, MPD_TAG_TITLE, 0);
        const char* artists = mpd_song_get_tag(m_mpd_song, MPD_TAG_ARTIST, 0);
        const char* year = mpd_song_get_tag(m_mpd_song, MPD_TAG_DATE, 0);
        const char* album = mpd_song_get_tag(m_mpd_song, MPD_TAG_ALBUM, 0);
        const char* num = mpd_song_get_tag(m_mpd_song, MPD_TAG_TRACK, 0);
        const char* disc = mpd_song_get_tag(m_mpd_song, MPD_TAG_DISC, 0);
        const char* label = mpd_song_get_tag(m_mpd_song, MPD_TAG_LABEL, 0);
#undef MPD_TAG_LABEL

        if (title)
            m_current.set_title(title);
        if (artists)
            m_current.append_artist(artists);
        if (year)
            m_current.set_year(year);
        if (album)
            m_current.set_album(album);
        if (num)
            m_current.set_track_number(QString(num).toUInt());
        if (disc)
            m_current.set_disc_number(QString(disc).toUInt());
        if (label)
            m_current.set_label(label);

        m_current.set_duration(mpd_song_get_duration_ms(m_mpd_song));

        QString file_path = mpd_song_get_uri(m_mpd_song);
        QString tmp;
        file_path.prepend(m_base_folder);

        if (m_current.playing()) {
            if (!cover::find_embedded_cover(file_path)) {
                cover::get_file_folder(file_path);

                if (!file_path.startsWith("http")) /* this is not a url */
                    file_path.prepend("file://");
                cover::find_local_cover(file_path, tmp);
                m_current.set_cover_link(tmp);
                util::download_cover(m_current);
            }
        } else {
            util::download_cover(m_current);
        }
    } else {
        cover::find_embedded_cover("", true);
        util::download_cover(m_current);
    }

    if (m_mpd_song)
        mpd_song_free(m_mpd_song);
    m_mpd_song = nullptr;

    if (m_status)
        mpd_status_free(m_status);
    m_status = nullptr;
}

bool mpd_source::execute_capability(capability c)
{
    if (!m_connected)
        return false;
    bool result = false;
    switch (c) {
    case CAP_NEXT_SONG:
        result = mpd_run_next(m_connection);
        break;
    case CAP_PREV_SONG:
        result = mpd_run_previous(m_connection);
        break;
    case CAP_VOLUME_UP:
        result = mpd_run_change_volume(m_connection, 2);
        break;
    case CAP_VOLUME_DOWN:
        result = mpd_run_change_volume(m_connection, -2);
        break;
    case CAP_VOLUME_MUTE:
        result = mpd_run_set_volume(m_connection, 0);
        break;
    case CAP_PLAY_PAUSE:
        result = mpd_run_toggle_pause(m_connection);
        if (m_stopped)
            result = mpd_run_play_pos(m_connection, 0);
        m_stopped = false;
        break;
    case CAP_STOP_SONG:
        m_stopped = true;
        result = mpd_run_stop(m_connection);
        break;
    default:;
    }

    return result;
}

bool mpd_source::valid_format(const QString& str)
{
    /* Supports all specifiers */
    UNUSED_PARAMETER(str);
    return true;
}
