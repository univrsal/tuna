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

#include "mpd_source.hpp"
#include "../gui/music_control.hpp"
#include "../gui/tuna_gui.hpp"
#include "../gui/widgets/mpd.hpp"
#include "../util/config.hpp"
#include "../util/cover_tag_handler.hpp"
#include "../util/utility.hpp"
#include <QStringList>
#include <obs-module.h>
#include <taglib/fileref.h>

mpd_source::mpd_source()
    : music_source(S_SOURCE_MPD, T_SOURCE_MPD, new mpd)
{
    m_capabilities = CAP_NEXT_SONG | CAP_PREV_SONG | CAP_PLAY_PAUSE | CAP_STOP_SONG | CAP_VOLUME_UP | CAP_VOLUME_DOWN | CAP_VOLUME_MUTE;
    supported_metadata({ meta::TITLE, meta::ARTIST, meta::ALBUM, meta::RELEASE, meta::RELEASE_DAY, meta::RELEASE_MONTH, meta::RELEASE_YEAR, meta::COVER, meta::LYRICS, meta::DURATION, meta::DISC_NUMBER, meta::TRACK_NUMBER, meta::PROGRESS, meta::STATUS, meta::LABEL, meta::FILE_NAME });
    m_address = nullptr;
    m_port = 0;
}

void mpd_source::reset_info()
{
    music_source::reset_info();
    disconnect();
}

struct mpd_connection* mpd_source::connect()
{
    struct mpd_connection* result = nullptr;
    if (m_local)
        result = mpd_connection_new(nullptr, 0, 20); // 0 ms timeout on windows blocks this thread, which is bad
                                                     // but it also can't be as high as the refresh rate otherwise
                                                     // the frehfresh thread will never sleep
    else
        result = mpd_connection_new(qt_to_utf8(m_address), m_port, 20);

    if (mpd_connection_get_error(result) != MPD_ERROR_SUCCESS) {
        if (util::epoch() - m_last_error_log > 5) {
            if (m_local) {
                berr("local mpd connection on default port (usually %i) failed with error '%s'", 6600,
                    mpd_connection_get_error_message(result));
            } else {
                berr("mpd connection to %s:%hu failed with error '%s'", qt_to_utf8(m_address), m_port,
                    mpd_connection_get_error_message(result));
            }
            m_last_error_log = util::epoch();
        }

        mpd_connection_free(result);
        result = nullptr;
    }
    return result;
}

bool mpd_source::enabled() const
{
    return true;
}

void mpd_source::load()
{
    music_source::load();
    CDEF_INT(CFG_MPD_PORT, 0);
    CDEF_STR(CFG_MPD_IP, "localhost");
    CDEF_BOOL(CFG_MPD_LOCAL, true);
    CDEF_STR(CFG_MPD_BASE_FOLDER, "");

    m_address = utf8_to_qt(CGET_STR(CFG_MPD_IP));
    m_base_folder = utf8_to_qt(CGET_STR(CFG_MPD_BASE_FOLDER));
    m_port = CGET_UINT(CFG_MPD_PORT);
    m_local = CGET_BOOL(CFG_MPD_LOCAL);
}

static inline play_state from_mpd_state(mpd_state s)
{
    switch (s) {
    case MPD_STATE_PLAY:
        return state_playing;
    case MPD_STATE_PAUSE:
        return state_paused;
    case MPD_STATE_STOP:
        return state_stopped;
    default:
    case MPD_STATE_UNKNOWN:
        return state_unknown;
    }
}

void mpd_source::refresh()
{
    struct mpd_connection* connection = connect();
    struct mpd_status* status = nullptr;
    struct mpd_song* mpd_song = nullptr;

    if (!connection)
        return;
    begin_refresh();
    m_current.clear();

    status = mpd_run_status(connection);
    mpd_song = mpd_run_current_song(connection);

    if (status) {
        auto new_state = mpd_status_get_state(status);
        m_current.set<int>(meta::PROGRESS, ((int)mpd_status_get_elapsed_ms(status)));
        m_current.set<int>(meta::STATUS, from_mpd_state(new_state));
    }

/* Thanks ubuntu for using ancient packages */
#define MPD_TAG_LABEL (mpd_tag_type)21
    if (mpd_song) {
        const char* title = mpd_song_get_tag(mpd_song, MPD_TAG_TITLE, 0);
        const char* artists = mpd_song_get_tag(mpd_song, MPD_TAG_ARTIST, 0);
        const char* date = mpd_song_get_tag(mpd_song, MPD_TAG_DATE, 0);
        const char* album = mpd_song_get_tag(mpd_song, MPD_TAG_ALBUM, 0);
        const char* num = mpd_song_get_tag(mpd_song, MPD_TAG_TRACK, 0);
        const char* disc = mpd_song_get_tag(mpd_song, MPD_TAG_DISC, 0);
        const char* label = mpd_song_get_tag(mpd_song, MPD_TAG_LABEL, 0);
        const char* uri = mpd_song_get_uri(mpd_song);
#undef MPD_TAG_LABEL

        if (title)
            m_current.set(meta::TITLE, utf8_to_qt(title));
        if (artists)
            m_current.set(meta::ARTIST, QStringList(utf8_to_qt(artists)));
        if (date) {
            QStringList list = utf8_to_qt(date).split("-");
            switch (list.length()) {
            case 3:
                m_current.set(meta::RELEASE_DAY, QString(list[2]).toInt());
                [[clang::fallthrough]];
            case 2:
                m_current.set(meta::RELEASE_MONTH, QString(list[1]).toInt());
                [[clang::fallthrough]];
            case 1:
                m_current.set(meta::RELEASE_YEAR, QString(list[0]).toInt());
            }
        }
        if (album)
            m_current.set(meta::ALBUM, utf8_to_qt(album));
        if (num)
            m_current.set(meta::TRACK_NUMBER, QString(num).toInt());
        if (disc)
            m_current.set(meta::DISC_NUMBER, QString(disc).toInt());
        if (label)
            m_current.set(meta::LABEL, utf8_to_qt(label));

        QString file_path = utf8_to_qt(uri);
        if (uri) {
            auto file = util::file_from_path(file_path);
            m_current.set(meta::FILE_NAME, file);
            if (!title)
                m_current.set(meta::TITLE, util::remove_extensions(file));
        }

        m_current.set(meta::DURATION, (int)mpd_song_get_duration_ms(mpd_song));

        /* Absolute path to current song file */
        file_path.prepend(m_base_folder);
        m_song_file_path = file_path;

        /* The song url link is now used by the browser widget which requires
         * a proper url to embed it into the browser source, the actal
         * retrieval of the cover is done via m_song_file_path which checks
         * the song file for cover tags as well as the folder the file is in
         */
        QString path = config::cover_path;

        // Convert to proper file:// url
        path = '/' + path;
        path.replace('\\', '/'); // url has to use unix separators
        path = "file://" + path;
        m_current.set(meta::COVER, path);
    }

    mpd_connection_free(connection);
    if (mpd_song)
        mpd_song_free(mpd_song);
    if (status)
        mpd_status_free(status);
}

void mpd_source::handle_cover()
{
    if (m_current == m_prev)
        return;

    if (m_current.get<int>(meta::STATUS) == state_playing) {
        bool result = false;
        QString file_path = m_song_file_path, tmp;
        if (cover::find_embedded_cover(file_path)) {
            result = true;
        } else {
            cover::get_file_folder(file_path);

            /* try to find a cover image in the same folder*/
            if (cover::find_local_cover(file_path, tmp)) {
                tmp = "file://" + tmp; /* cURL needs this to "download" the file */
                result = util::download_cover(tmp);
            }
        }
        if (!result && !download_missing_cover())
            util::reset_cover();
    } else if (m_current.get<int>(meta::STATUS) != state_paused || config::placeholder_when_paused) {
        if (!download_missing_cover())
            util::reset_cover();
    }
}

bool mpd_source::execute_capability(capability c)
{
    struct mpd_connection* connection = connect();
    if (!connection)
        return false;
    bool result = false;
    switch (c) {
    case CAP_NEXT_SONG:
        result = mpd_run_next(connection);
        break;
    case CAP_PREV_SONG:
        result = mpd_run_previous(connection);
        break;
    case CAP_VOLUME_UP:
        result = mpd_run_change_volume(connection, 2);
        break;
    case CAP_VOLUME_DOWN:
        result = mpd_run_change_volume(connection, -2);
        break;
    case CAP_VOLUME_MUTE:
        result = mpd_run_set_volume(connection, 0);
        break;
    case CAP_PLAY_PAUSE:
        result = mpd_run_toggle_pause(connection);
        if (m_stopped)
            result = mpd_run_play_pos(connection, 0);
        m_stopped = false;
        break;
    case CAP_STOP_SONG:
        m_stopped = true;
        result = mpd_run_stop(connection);
        break;
    default:;
    }

    mpd_connection_free(connection);
    return result;
}
