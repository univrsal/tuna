/*************************************************************************
 * This file is part of tuna
 * github.com/univrsal/tuna
 * Copyright 2020 univrsal <uni@vrsal.cf>.
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

#include "vlc_obs_source.hpp"
#include "../gui/tuna_gui.hpp"
#include "../gui/widgets/vlc.hpp"
#include "../util/constants.hpp"
#include "../util/config.hpp"
#include "../util/utility.hpp"
#include <QUrl>

vlc_obs_source::vlc_obs_source()
    : music_source(S_SOURCE_VLC, T_SOURCE_VLC, new vlc)
{
    m_capabilities = CAP_TITLE | CAP_LABEL | CAP_ALBUM | CAP_PROGRESS | CAP_VOLUME_UP | CAP_VOLUME_DOWN | CAP_VOLUME_MUTE | CAP_DURATION | CAP_PLAY_PAUSE | CAP_NEXT_SONG | CAP_PREV_SONG;
}

vlc_obs_source::~vlc_obs_source()
{
    /* Currently VLC sources cause crashes on exit when stopped
     * so this might mitigate the issue */
    if (m_weak_src) {
        obs_source_t *src = obs_weak_source_get_source(m_weak_src);
        if (src && obs_source_media_get_state(src) == OBS_MEDIA_STATE_STOPPED) {
            obs_source_media_restart(src);
            obs_source_release(src);
        }
    }
    obs_weak_source_release(m_weak_src);
    m_weak_src = nullptr;
}

bool vlc_obs_source::reload()
{
    auto result = !!m_weak_src;
    if (!m_weak_src && m_target_source_name != std::string(T_VLC_NONE))
        load_vlc_source();

    if (m_weak_src) {
        auto src = obs_weak_source_get_source(m_weak_src);
        if (src) {
            result = obs_source_showing(src);
            obs_source_release(src);
        }
    }

    return result;
}

void vlc_obs_source::load_vlc_source()
{
    CDEF_STR(CFG_VLC_ID, "");
    m_target_source_name = CGET_STR(CFG_VLC_ID);

    auto* src = obs_get_source_by_name(m_target_source_name);

    obs_weak_source_release(m_weak_src);
    m_weak_src = nullptr;

    if (src) {
        const auto* id = obs_source_get_id(src);
        if (strcmp(id, "vlc_source") == 0) {
            m_weak_src = obs_source_get_weak_source(src);
        } else {
            binfo("%s (%s) is not a valid vlc source", m_target_source_name, id);
        }
        obs_source_release(src);
    }
}

bool vlc_obs_source::enabled() const
{
    return util::have_vlc_source;
}

void vlc_obs_source::load()
{
    if (!util::have_vlc_source || m_weak_src)
        return;
    music_source::load();
    load_vlc_source();
}

static play_state from_obs_state(obs_media_state s)
{
    switch (s) {
    case OBS_MEDIA_STATE_PLAYING:
        return state_playing;
    case OBS_MEDIA_STATE_BUFFERING:
    case OBS_MEDIA_STATE_OPENING:
    case OBS_MEDIA_STATE_PAUSED:
        return state_paused;
    case OBS_MEDIA_STATE_STOPPED:
        return state_paused;
    default:
    case OBS_MEDIA_STATE_NONE:
    case OBS_MEDIA_STATE_ERROR:
        return state_unknown;
    }
}

void vlc_obs_source::refresh()
{

    if (!reload())
        return;

    /* we keep a reference here to make sure that this source won't be freed
     * while we still need it */
    obs_source_t* src = obs_weak_source_get_source(m_weak_src);
    if (!src)
        return;

    begin_refresh();
    m_current.clear();
    m_current.set_state(from_obs_state(obs_source_media_get_state(src)));

    /* Prevent polling when vlc is stopped, which otherwise could cause a crash
       when closing obs */
    if (m_current.state() == state_stopped)
        return;

    proc_handler_t* ph = obs_source_get_proc_handler(src);

    if (!ph)
        return;

    auto* calldata = calldata_create();

    auto get_meta = [](const char* tag_id, calldata_t* cd, proc_handler_t* ph) {
        const char* result = nullptr;
        calldata_set_string(cd, "tag_id", tag_id);
        bool failure = !proc_handler_call(ph, "get_metadata",
            cd);
        if (failure || !calldata_get_string(cd, "tag_data", &result))
            berr("Failed to retrieve %s tag", tag_id);
        return utf8_to_qt(result);
    };

    m_current.set_progress(obs_source_media_get_time(src));
    m_current.set_duration(obs_source_media_get_duration(src));

    if (m_current.state() <= state_paused) {
        auto artwork_url = get_meta("artwork_url", calldata, ph);
        auto title = get_meta("title", calldata, ph);
        auto artist = get_meta("artist", calldata, ph);
        auto date = get_meta("date", calldata, ph);
        auto album = get_meta("album", calldata, ph);
        auto publisher = get_meta("publisher", calldata, ph);
        auto track_number = get_meta("track_number", calldata, ph);
        auto disc_number = get_meta("disc_number", calldata, ph);

        if (artwork_url != "")
            m_current.set_cover_link(QUrl::fromPercentEncoding(qt_to_utf8(artwork_url)));
        if (title != "")
            m_current.set_title(title);
        if (artist != "")
            m_current.append_artist(artist);
        if (date != "")
            m_current.set_year(date);
        if (album != "")
            m_current.set_album(album);
        if (publisher != "")
            m_current.set_label(publisher);
        if (track_number != "")
            m_current.set_track_number(track_number.toUInt());
        if (disc_number != "")
            m_current.set_track_number(disc_number.toUInt());
    }

    calldata_destroy(calldata);
    obs_source_release(src);
#undef GET_VLC_META
}

bool vlc_obs_source::execute_capability(capability c)
{
    obs_source_t* src = obs_weak_source_get_source(m_weak_src);
    if (!src)
        return false;

    float vol = obs_source_get_volume(src);
    auto state = obs_source_media_get_state(src);

    switch (c) {
    case CAP_PREV_SONG:
        obs_source_media_previous(src);
        break;
    case CAP_NEXT_SONG:
        obs_source_media_next(src);
        break;
    case CAP_PLAY_PAUSE:
        obs_source_media_play_pause(src, state == OBS_MEDIA_STATE_PLAYING);
    case CAP_STOP_SONG:
        obs_source_media_stop(src);
        break;
    case CAP_VOLUME_UP:
        if (src)
            obs_source_set_volume(src, vol + (vol / 10));
        break;
    case CAP_VOLUME_DOWN:
        if (src)
            obs_source_set_volume(src, vol - (vol / 10));
        break;
    default:;
    }

    obs_source_release(src);
    return true;
}

bool vlc_obs_source::valid_format(const QString& str)
{
    UNUSED_PARAMETER(str);
    return true;
}
