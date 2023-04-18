/*************************************************************************
 * This file is part of tuna
 * git.vrsal.xyz/alex/tuna
 * Copyright 2023 univrsal <uni@vrsal.xyz>.
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
    m_weak_src = nullptr;
}

bool vlc_obs_source::reload()
{
    auto result = !!m_weak_src;
    auto target_source_name = get_target_source_name();
    const auto name_changed_or_no_weak_src = !m_weak_src || m_target_source_name != target_source_name;

    // Try to find the vlc source with the new name
    if (name_changed_or_no_weak_src && !target_source_name.empty())
        load_vlc_source();

    if (m_weak_src) {
        OBSSourceAutoRelease src = obs_weak_source_get_source(m_weak_src);
        if (src) {
            result = obs_source_showing(src);
            if (!result) {
                m_current.clear();
                m_current.set(meta::STATUS, state_stopped);
            }
        } else {
            get_ui<vlc>()->rebuild_mapping();
            result = false;
            m_weak_src = nullptr;
        }
    }

    return result;
}

void vlc_obs_source::load_vlc_source()
{
    m_target_source_name = get_target_source_name();

    OBSSourceAutoRelease src = obs_get_source_by_name(m_target_source_name.c_str());
    m_weak_src = nullptr;

    if (src) {
        const auto* id = obs_source_get_id(src);
        if (strcmp(id, "vlc_source") == 0) {
            m_weak_src = obs_source_get_weak_source(src);
        } else {
            binfo("%s (%s) is not a valid vlc source", m_target_source_name.c_str(), id);
        }
    }
}

std::string vlc_obs_source::get_target_source_name()
{
    auto current_scene_name = get_current_scene_name();

    if (current_scene_name.empty())
        return "";
    m_target_scene = current_scene_name;
    auto mappings = static_cast<vlc*>(get_settings_tab())->get_mappings_for_scene(current_scene_name.c_str());

    if (mappings.empty()) {
        OBSSourceAutoRelease scene = obs_frontend_get_current_scene();
        // We don't have any mappings -> try to find the first active VLC source
        struct data_s {
            const char* name = nullptr;
            bool found_source = false;
        } data;
        obs_source_enum_active_sources(
            scene, [](obs_source_t*, obs_source_t* child, void* data) {
                data_s* datas = static_cast<data_s*>(data);
                if (!datas->found_source && strcmp(obs_source_get_id(child), "vlc_source") == 0) {
                    datas->name = obs_source_get_name(child);
                    datas->found_source = true;
                }
            },
            &data);
        if (data.found_source)
            return data.name;
        return "";
    }

    m_index = qMin(mappings.size(), m_index);
    return qt_to_utf8(mappings[m_index].toString());
}

std::string vlc_obs_source::get_current_scene_name()
{
    OBSSourceAutoRelease active_scene = obs_frontend_get_current_scene();
    if (active_scene)
        return obs_source_get_name(active_scene);
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
    get_ui<vlc>()->build_list();
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
    begin_refresh();
    m_current.clear();
    if (!reload())
        return;

    /* we keep a reference here to make sure that this source won't be freed
     * while we still need it */
    OBSSourceAutoRelease src = get_source();
    if (!src)
        return;

    m_current.set(meta::STATUS, from_obs_state(obs_source_media_get_state(src)));

    /* Prevent polling when vlc is stopped, which otherwise could cause a crash
       when closing obs */
    if (m_current.get<int>(meta::STATUS) == state_stopped)
        return;

    proc_handler_t* ph = obs_source_get_proc_handler(src);

    if (!ph)
        return;

    auto* cd = calldata_create();

    auto get_meta = [cd, ph](const char* tag_id) {
        const char* result = "";
        calldata_set_string(cd, "tag_id", tag_id);
        bool failure = !proc_handler_call(ph, "get_metadata", cd);
        if (failure || !calldata_get_string(cd, "tag_data", &result))
            berr("Failed to retrieve %s tag", tag_id);
        return utf8_to_qt(result);
    };

    m_current.set(meta::PROGRESS, (int)obs_source_media_get_time(src));
    m_current.set(meta::DURATION, (int)obs_source_media_get_duration(src));

    if (m_current.get<int>(meta::STATUS) <= state_paused) {
#define check(t, d)              \
    do {                         \
        auto t = get_meta(#t);   \
        if (t != "")             \
            m_current.set(d, t); \
    } while (0)

#define check_num(t, d)              \
    do {                             \
        auto t = get_meta(#t);       \
        if (t != "") {               \
            bool ok = false;         \
            auto i = t.toInt(&ok);   \
            if (ok)                  \
                m_current.set(d, i); \
        }                            \
    } while (0)

        // Some of these could technically be numbers
        // like season or episode, but I think that VLC
        // allows users to enter anything in there so we'll
        // just use strings instead of assuming that it'll always be a number
        check(artwork_url, meta::COVER);
        check(title, meta::TITLE);
        check(album, meta::ALBUM);
        check(publisher, meta::LABEL);
        check(genre, meta::GENRE);
        check(copyright, meta::COPYRIGHT);
        check(description, meta::DESCRIPTION);
        check(rating, meta::RATING);
        check(setting, meta::SETTING);
        check(language, meta::LANGUAGE);
        check(now_playing, meta::NOW_PLAYING);
        check(encoded_by, meta::ENCODED_BY);
        check(track_id, meta::TRACK_ID);
        check(director, meta::DIRECTOR);
        check(season, meta::SEASON);
        check(episode, meta::EPISODE);
        check(show_name, meta::SHOW_NAME);
        check(actors, meta::ACTORS);
        check(album_artist, meta::ALBUM_ARTIST);
        check_num(track_number, meta::TRACK_NUMBER);
        check_num(disc_number, meta::DISC_NUMBER);
        check_num(track_total, meta::TRACK_TOTAL);
        check_num(disc_total, meta::DISC_TOTAL);
        check(url, meta::URL);

        auto artist = get_meta("artist");
        if (artist != "")
            m_current.set(meta::ARTIST, QStringList(artist));

        auto date = get_meta("date");
        if (!date.isEmpty()) {
            auto splits = date.split("-");

            switch (splits.length()) {
            case 3:
                m_current.set(meta::RELEASE_DAY, splits[2].toInt());
                [[fallthrough]];
            case 2:
                m_current.set(meta::RELEASE_MONTH, splits[1].toInt());
                [[fallthrough]];
            case 1:
                m_current.set(meta::RELEASE_YEAR, splits[0].toInt());
                break;
            default:;
            }
        }
    }

    calldata_destroy(cd);
#undef check
#undef check_num
}

bool vlc_obs_source::execute_capability(capability c)
{
    OBSSourceAutoRelease src = get_source();
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
        break;
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
    return true;
}
