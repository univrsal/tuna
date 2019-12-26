/*************************************************************************
 * This file is part of tuna
 * github.con/univrsal/tuna
 * Copyright 2019 univrsal <universailp@web.de>.
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
#include "../util/config.hpp"
#include "../util/utility.hpp"
#include "../util/vlc_internal.h"

vlc_obs_source::vlc_obs_source()
{
    m_capabilities = CAP_TITLE | CAP_ALBUM | CAP_PROGRESS | CAP_VOLUME_UP | CAP_VOLUME_DOWN | CAP_VOLUME_MUTE | CAP_LENGTH | CAP_PLAY_PAUSE | CAP_NEXT_SONG | CAP_PREV_SONG;

    if (!load_libvlc()) {
        blog(LOG_INFO, "[tuna] Couldn't load libVLC, VLC support disabled");
    } else {
        m_lib_vlc_loaded = true;
    }
}

vlc_obs_source::~vlc_obs_source()
{
    obs_weak_source_release(m_weak_src);
}

void vlc_obs_source::load()
{
    if (!util::vlc_loaded)
        return;
    CDEF_STR(CFG_VLC_ID, "");
    m_target_source_name = CGET_STR(CFG_VLC_ID);
    auto* src = obs_get_source_by_name(m_target_source_name.c_str());

    if (src) {
        std::string id(obs_source_get_id(src));
        if (id == "vlc_source") {
            if (m_weak_src)
                obs_weak_source_release(m_weak_src);
            m_weak_src = obs_source_get_weak_source(src);
        } else {
            blog(LOG_INFO, "[tuna] %s is not a valid vlc source", id.c_str());
        }
    }
}

void vlc_obs_source::save()
{
    if (!util::vlc_loaded)
        return;
    CSET_STR(CFG_VLC_ID, m_target_source_name.c_str());
}

struct vlc_source* vlc_obs_source::get_vlc()
{
    if (!m_weak_src || !m_lib_vlc_loaded)
        return nullptr;

    auto* src = obs_weak_source_get_source(m_weak_src);
    if (src) {
        void* data = src->context.data;
        if (data) {
            return reinterpret_cast<struct vlc_source*>(data);
        }
    }
    return nullptr;
}

void vlc_obs_source::refresh()
{
    if (!util::vlc_loaded)
        return;
    auto* vlc = get_vlc();

    if (vlc) {
        m_current.progress_ms = libvlc_media_player_get_time_(vlc->media_player);
        m_current.duration_ms = libvlc_media_player_get_time_(vlc->media_player);
        m_current.is_playing = libvlc_media_player_get_state_(vlc->media_player)
            == libvlc_Playing;
        m_current.data |= CAP_PROGRESS | CAP_STATUS | CAP_TITLE | CAP_LENGTH;

        auto* media = libvlc_media_player_get_media_(vlc->media_player);
        if (media) {
            const char* title = libvlc_media_get_meta_(media, libvlc_meta_Title);
            const char* artists = libvlc_media_get_meta_(media, libvlc_meta_Artist);
            const char* year = libvlc_media_get_meta_(media, libvlc_meta_Date);
            const char* album = libvlc_media_get_meta_(media, libvlc_meta_Album);
            const char* num = libvlc_media_get_meta_(media, libvlc_meta_TrackID);
            const char* disc = libvlc_media_get_meta_(media, libvlc_meta_DiscNumber);
            const char* cover = libvlc_media_get_meta_(media, libvlc_meta_ArtworkURL);

            if (title)
                m_current.title = title;
            else
                m_current.title = "N/A";

            if (cover) {
                m_current.cover = cover;
                m_current.data |= CAP_COVER;
            }

            if (artists) {
                m_current.artists = artists;
                m_current.data |= CAP_ARTIST;
            }

            if (year) {
                m_current.year = year;
                m_current.data |= CAP_RELEASE;
                m_current.release_precision = prec_year;
            }

            if (album) {
                m_current.album = album;
                m_current.data |= CAP_ALBUM;
            }

            if (num) {
                m_current.track_number = std::stoi(num);
                m_current.data |= CAP_TRACK_NUMBER;
            }

            if (disc) {
                m_current.disc_number = std::stoi(disc);
                m_current.data |= CAP_DISC_NUMBER;
            }
        }
    } else {
        m_current = {};
    }
}

bool vlc_obs_source::execute_capability(capability c)
{
    auto* vlc = get_vlc();
    if (vlc) {
        switch (c) {
        case CAP_NEXT_SONG:
            break;
        case CAP_PREV_SONG:
            break;
        case CAP_VOLUME_UP:
            break;
        case CAP_VOLUME_DOWN:
            break;
        case CAP_VOLUME_MUTE:
            break;
        case CAP_PLAY_PAUSE:
            if (libvlc_media_player_can_pause(vlc->media_player))
                libvlc_media_player_pause(vlc->media_player);
            break;
        }
    }
    return true;
}

void vlc_obs_source::load_gui_values()
{
    tuna_dialog->select_vlc_source(m_target_source_name.c_str());
}
