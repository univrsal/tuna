/**
 * This file is part of tuna
 * which is licensed under the GPL v2.0
 * See LICENSE or http://www.gnu.org/licenses
 * github.com/univrsal/tuna
 */
#include "mpd_source.hpp"
#include "../util/config.hpp"
#include "../gui/tuna_gui.hpp"
#include <obs-module.h>

mpd_source::mpd_source()
{
    m_capabilities = CAP_TITLE | CAP_ALBUM | CAP_PROGRESS | CAP_VOLUME_UP
            | CAP_VOLUME_DOWN | CAP_VOLUME_MUTE | CAP_LENGTH | CAP_PLAY_PAUSE
            | CAP_NEXT_SONG | CAP_PREV_SONG;
    m_address = nullptr;
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
    if (m_connection) {
        mpd_connection_free(m_connection);
        m_connection = nullptr;
    }
    m_connected = false;
}

void mpd_source::connect()
{
    disconnect();
    m_connection = mpd_connection_new(m_address, m_port, 500);
    if (mpd_connection_get_error(m_connection) != MPD_ERROR_SUCCESS) {
        blog(LOG_ERROR, "[tuna] mpd connection to %s:%hu failed with error %s",
             m_address, m_port, mpd_connection_get_error_message(m_connection));
    } else {
        m_connected = true;
        mpd_connection_set_keepalive(m_connection, true);
    }
}

void mpd_source::load()
{
    CDEF_INT(CFG_MPD_PORT, 0);
    CDEF_STR(CFG_MPD_IP, "localhost");
    m_address = CGET_STR(CFG_MPD_IP);
    m_port = CGET_UINT(CFG_MPD_PORT);
}

void mpd_source::save()
{
    CSET_STR(CFG_MPD_IP, m_address);
    CSET_UINT(CFG_MPD_PORT, m_port);
}

void mpd_source::refresh()
{
    if (!m_connected)
        connect();
    m_current = {};
    m_current.release_precision = prec_unkown;

    m_status = mpd_run_status(m_connection);
    m_mpd_song = mpd_run_current_song(m_connection);

    if (m_status) {
        m_mpd_state = mpd_status_get_state(m_status);
        m_current.progress_ms = mpd_status_get_elapsed_ms(m_status);
        m_current.is_playing = m_mpd_state == MPD_STATE_PLAY;
        m_current.data |= CAP_PROGRESS | CAP_STATUS;
    } else {
        /* Connection might have closed in between refreshes */
        connect();
    }

    if (m_mpd_song) {
        const char* title = mpd_song_get_tag(m_mpd_song, MPD_TAG_TITLE, 0);
        const char* artists = mpd_song_get_tag(m_mpd_song, MPD_TAG_ARTIST, 0);
        const char* year = mpd_song_get_tag(m_mpd_song, MPD_TAG_DATE, 0);
        const char* album = mpd_song_get_tag(m_mpd_song, MPD_TAG_ALBUM, 0);
        const char* num = mpd_song_get_tag(m_mpd_song, MPD_TAG_TRACK, 0);
        const char* disc = mpd_song_get_tag(m_mpd_song, MPD_TAG_DISC, 0);

        if (title) m_current.title = title; else m_current.title = "N/A";
        if (artists) m_current.artists = artists;
        if (year) m_current.year = year;
        if (album) m_current.album = album;

        if (num) m_current.track_number = std::stoi(num);
        if (disc) m_current.disc_number = std::atoi(disc);

        m_current.duration_ms = mpd_song_get_duration_ms(m_mpd_song);
        if (!m_current.title.empty())
            m_current.data |= CAP_TITLE;
        if (!m_current.artists.empty())
            m_current.data |= CAP_ARTIST;
        if (!m_current.year.empty()) {
            m_current.data |= CAP_RELEASE;
            m_current.release_precision = prec_year;
        }
        if (!m_current.album.empty())
            m_current.data |= CAP_ALBUM;
        if (m_current.duration_ms > 0)
            m_current.data |= CAP_LENGTH;
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
    switch (c) {
    case CAP_NEXT_SONG:
        return mpd_run_next(m_connection);
    case CAP_PREV_SONG:
        return mpd_run_previous(m_connection);
    case CAP_VOLUME_UP:
        return mpd_run_change_volume(m_connection, 2);
    case CAP_VOLUME_DOWN:
        return mpd_run_change_volume(m_connection, -2);
    case CAP_VOLUME_MUTE:
        return mpd_run_set_volume(m_connection, 0);
    case CAP_PLAY_PAUSE:
        return mpd_run_toggle_pause(m_connection);
    }
    return false;
}

void mpd_source::load_gui_values()
{
    tuna_dialog->set_mpd_ip(m_address);
    tuna_dialog->set_mpd_port(m_port);
}
