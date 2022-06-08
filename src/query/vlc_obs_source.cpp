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

#include "vlc_obs_source.hpp"
#include "../gui/tuna_gui.hpp"
#include "../gui/widgets/vlc.hpp"
#include "../util/config.hpp"
#include "../util/constants.hpp"
#include "../util/tuna_thread.hpp"
#include "../util/utility.hpp"
#include <QUrl>
#include <obs-frontend-api.h>

vlc_obs_source::vlc_obs_source()
    : music_source(S_SOURCE_VLC, T_SOURCE_VLC, new vlc)
{
    /* clang-format off */
    m_capabilities = CAP_NEXT_SONG | CAP_PREV_SONG | CAP_PLAY_PAUSE |
        CAP_STOP_SONG | CAP_VOLUME_UP | CAP_VOLUME_DOWN | CAP_VOLUME_MUTE;
    supported_metadata({ meta::TITLE, meta::ARTIST, meta::ALBUM, meta::RELEASE,
        meta::COVER, meta::LYRICS, meta::DURATION, meta::DISC_NUMBER, meta::TRACK_NUMBER,
        meta::PROGRESS, meta::STATUS, meta::LABEL, meta::FILE_NAME,
        meta::GENRE, meta::COPYRIGHT, meta::DESCRIPTION, meta::RATING, meta::DATE,
        meta::SETTING, meta::URL, meta::LANGUAGE, meta::NOW_PLAYING, meta::PUBLISHER,
        meta::ENCODED_BY, meta::ARTWORK_URL, meta::TRACK_TOTAL, meta::DIRECTOR, meta::SEASON,
        meta::EPISODE, meta::SHOW_NAME, meta::ALBUM_ARTIST, meta::DISC_TOTAL });
    /* clang-format on */
}

vlc_obs_source::~vlc_obs_source()
{
    obs_weak_source_release(m_weak_src);
    m_weak_src = nullptr;
}

bool vlc_obs_source::reload()
{
    auto result = !!m_weak_src;
    auto target_source = get_target_source();
    const auto name_changed_or_no_weak_src = !m_weak_src || m_target_source_name != target_source;
    if (name_changed_or_no_weak_src && !target_source.empty())
        load_vlc_source();
    if (m_weak_src) {
        auto src = obs_weak_source_get_source(m_weak_src);
        if (src) {
            result = obs_source_showing(src);
            if (!result) {
                m_current.clear();
                m_current.set(meta::STATUS, state_stopped);
            }
            obs_source_release(src);
        } else {
            static_cast<vlc*>(m_settings_tab)->rebuild_mapping();
            result = false;
            obs_weak_source_release(m_weak_src);
            m_weak_src = nullptr;
        }
    }

    return result;
}

void vlc_obs_source::load_vlc_source()
{
    m_target_source_name = get_target_source();

    auto* src = obs_get_source_by_name(m_target_source_name.c_str());

    obs_weak_source_release(m_weak_src);
    m_weak_src = nullptr;

    if (src) {
        const auto* id = obs_source_get_id(src);
        if (strcmp(id, "vlc_source") == 0) {
            m_weak_src = obs_source_get_weak_source(src);
        } else {
            binfo("%s (%s) is not a valid vlc source", m_target_source_name.c_str(), id);
        }
        obs_source_release(src);
    }
}

std::string vlc_obs_source::get_target_source()
{
    auto current_scene_name = get_current_scene_name();

    if (!current_scene_name.empty()) {
        m_target_scene = current_scene_name;
        auto mappings = static_cast<vlc*>(get_settings_tab())->get_mappings_for_scene(current_scene_name.c_str());

        m_index = qMin(mappings.size(), m_index);
        return qt_to_utf8(mappings[m_index].toString());
    }
    return "";
}

std::string vlc_obs_source::get_current_scene_name()
{
    auto active_scene = obs_frontend_get_current_scene();
    if (active_scene) {
        auto name = obs_source_get_name(active_scene);
        obs_source_release(active_scene);
        return name;
    }
    return "";
}

bool vlc_obs_source::enabled() const
{
    return util::have_vlc_source;
}

void vlc_obs_source::next_vlc_source()
{
    std::lock_guard<std::mutex> lock(tuna_thread::thread_mutex);
    auto mappings = static_cast<vlc*>(get_settings_tab())->get_mappings_for_scene(m_target_scene.c_str());
    if (mappings.empty())
        return;
    m_index = (m_index + 1) % mappings.size();
}

void vlc_obs_source::prev_vlc_source()
{
    std::lock_guard<std::mutex> lock(tuna_thread::thread_mutex);
    auto mappings = static_cast<vlc*>(get_settings_tab())->get_mappings_for_scene(m_target_scene.c_str());
    if (mappings.empty())
        return;
    m_index--;
    if (m_index < 0)
        m_index = mappings.size() - 1;
}

void vlc_obs_source::set_gui_values()
{
    music_source::set_gui_values();
    static_cast<vlc*>(m_settings_tab)->build_list();
}

void vlc_obs_source::load()
{
    music_source::load();
    if (!util::have_vlc_source || m_weak_src)
        return;
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
    case OBS_MEDIA_STATE_ENDED:
        return state_stopped;
    default:
    case OBS_MEDIA_STATE_NONE:
    case OBS_MEDIA_STATE_ERROR:
        return state_unknown;
        break;
    }
}

void vlc_obs_source::refresh()
{
    if (!reload())
        return;

    /* we keep a reference here to make sure that this source won't be freed
     * while we still need it */
    obs_source_t* src = get_source();
    if (!src)
        return;

    begin_refresh();
    m_current.clear();
    m_current.set(meta::STATUS, from_obs_state(obs_source_media_get_state(src)));

    /* Prevent polling when vlc is stopped, which otherwise could cause a crash
       when closing obs */
    if (m_current.get<int>(meta::STATUS) == state_stopped) {
        obs_source_release(src);
        return;
    }

    proc_handler_t* ph = obs_source_get_proc_handler(src);

    if (!ph) {
        obs_source_release(src);
        return;
    }

    auto* cd = calldata_create();

    auto get_meta = [cd, ph](const char* tag_id) {
        const char* result = nullptr;
        calldata_set_string(cd, "tag_id", tag_id);
        bool failure = !proc_handler_call(ph, "get_metadata", cd);
        if (failure || !calldata_get_string(cd, "tag_data", &result))
            berr("Failed to retrieve %s tag", tag_id);
        return utf8_to_qt(result);
    };

    m_current.set(meta::PROGRESS, (int)obs_source_media_get_time(src));
    m_current.set(meta::DURATION, (int)obs_source_media_get_duration(src));

    if (m_current.get<int>(meta::STATUS) <= state_paused) {
#define check(t, d)        \
    auto t = get_meta(#t); \
    if (t != "")           \
    m_current.set(d, t)

        check(artwork_url, meta::COVER);
        check(title, meta::TITLE);
        check(artist, meta::ARTIST);
        check(date, meta::RELEASE);
        check(album, meta::ALBUM);
        check(publisher, meta::LABEL);
        check(track_number, meta::TRACK_NUMBER);
        check(disc_number, meta::DISC_NUMBER);
        check(url, meta::FILE_NAME);
        check(genre, meta::GENRE);
#undef check
    }

    calldata_destroy(cd);
    obs_source_release(src);
}

bool vlc_obs_source::execute_capability(capability c)
{
    obs_source_t* src = get_source();
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
