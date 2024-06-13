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

#include "wmc_source.hpp"
#include "../gui/widgets/wmc.hpp"
#include "../util/config.hpp"
#include "../util/constants.hpp"
#include "../util/utility.hpp"
#include <QFile>

/**
 * Large chunks of this source were taken from
 * https://github.com/rmoalic/obs_media_info_plugin
 */

#define winrt_to_qt(str) utf8_to_qt(winrt::to_string(str).c_str())
#define winrt_to_std(str) winrt::to_string(str)

static void _handle_media_property_change(GlobalSystemMediaTransportControlsSession session, MediaPropertiesChangedEventArgs const& args)
{
    auto wmc_src = music_sources::get<wmc_source>(S_SOURCE_WMC);
    if (wmc_src)
        wmc_src->handle_media_property_change(session, args);
}

static void _handle_media_playback_info_change(GlobalSystemMediaTransportControlsSession session, PlaybackInfoChangedEventArgs const& args)
{
    auto wmc_src = music_sources::get<wmc_source>(S_SOURCE_WMC);
    if (wmc_src)
        wmc_src->handle_media_playback_info_change(session, args);
}

static void _handle_session_change(GlobalSystemMediaTransportControlsSessionManager, SessionsChangedEventArgs const&)
{
    auto wmc_src = music_sources::get<wmc_source>(S_SOURCE_WMC);
    if (wmc_src)
        wmc_src->update_players();
}

void wmc_source::update_players()
{
    std::vector<std::string> players_seen;
    auto sessions = m_session_manager.GetSessions();
    winrt::hstring AUMI;

    for (auto const& session : sessions) {
        AUMI = session.SourceAppUserModelId();
        std::string s = winrt::to_string(AUMI);

        players_seen.push_back(s);
        if (std::find(m_registered_players.begin(), m_registered_players.end(), s) == m_registered_players.end()) { // not found
            session.MediaPropertiesChanged(_handle_media_property_change);
            session.PlaybackInfoChanged(_handle_media_playback_info_change);

            m_registered_players.push_back(s);

            handle_media_property_change(session, NULL);
            handle_media_playback_info_change(session, NULL);
        }
    }
    std::sort(m_registered_players.begin(), m_registered_players.end());
    std::sort(players_seen.begin(), players_seen.end());
    std::vector<std::string> players_not_seen;
    set_difference(m_registered_players.begin(), m_registered_players.end(), players_seen.begin(), players_seen.end(), inserter(players_not_seen, players_not_seen.end()));

    for (auto player_name : players_not_seen) {
        m_registered_players.erase(std::remove(m_registered_players.begin(), m_registered_players.end(), player_name), m_registered_players.end());
    }
}

void wmc_source::save_cover(QImage& image)
{
    auto tmp = config::cover_path + ".tmp";

    if (image.save(tmp, "png")) {
        QFile current(config::cover_path);
        current.remove();
        if (!QFile::rename(tmp, config::cover_path)) {
            util::reset_cover();
            berr("[WMC] Failed to move cover from temporary file to %s", qt_to_utf8(config::cover_path));
        }

    } else {
        berr("[WMC] Failed to save cover to %s", qt_to_utf8(config::cover_path));
    }
}

bool wmc_source::execute_capability(capability c)
{
    /* We don't wait for the result of the async calls because
     * that's apparently not possible in the main thread, so we just assume they work
     */
    switch (c) {
    case CAP_NEXT_SONG:
        m_session_manager.GetCurrentSession().TrySkipNextAsync();
        return true;
    case CAP_PREV_SONG:
        m_session_manager.GetCurrentSession().TrySkipPreviousAsync();
        return true;
    case CAP_PLAY_PAUSE:
        m_session_manager.GetCurrentSession().TryTogglePlayPauseAsync();
        return true;
    case CAP_STOP_SONG:
        m_session_manager.GetCurrentSession().TryStopAsync();
        return true;
    default:
        return false;
    }
}

void wmc_source::request_manager()
{
    m_session_manager = GlobalSystemMediaTransportControlsSessionManager::RequestAsync().get();
    m_session_manager.SessionsChanged(_handle_session_change);
    update_players();
}

void wmc_source::load()
{
    music_source::load();
    CDEF_STR(CFG_WMC_PLAYER, "");
    m_selected_player = CGET_STR(CFG_WMC_PLAYER);
}

void wmc_source::handle_media_property_change(GlobalSystemMediaTransportControlsSession session, MediaPropertiesChangedEventArgs const&)
{
    GlobalSystemMediaTransportControlsSessionMediaProperties media_properties { nullptr };

    winrt::hstring t = session.SourceAppUserModelId();
    std::string id = winrt::to_string(t);

    media_properties = session.TryGetMediaPropertiesAsync().get();

    if (media_properties == nullptr)
        return;

    auto info = session.GetPlaybackInfo();
    if (info == nullptr)
        return;

    {
        std::lock_guard<std::mutex> lock(m_internal_mutex);

        m_info[id].set(meta::TITLE, winrt_to_qt(media_properties.Title()));
        m_info[id].set(meta::ARTIST, QStringList(winrt_to_qt(media_properties.Artist())));
        m_info[id].set(meta::ALBUM, winrt_to_qt(media_properties.AlbumTitle()));
        m_info[id].set(meta::ALBUM_ARTIST, winrt_to_qt(media_properties.AlbumArtist()));
        m_info[id].set<int>(meta::TRACK_NUMBER, media_properties.TrackNumber());
        m_info[id].set<int>(meta::TRACK_TOTAL, media_properties.AlbumTrackCount());
        auto tl = session.GetTimelineProperties();
        auto pos = tl.Position();
        auto foo = tl.LastUpdatedTime();
        auto bar = tl.MaxSeekTime();
        auto baz = tl.MinSeekTime();
        auto bams = tl.StartTime();
        auto end = tl.EndTime();
        auto progress_ms = std::chrono::duration_cast<std::chrono::milliseconds>(pos).count();
        auto duration_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end).count();
        m_info[id].set<int>(meta::PROGRESS, progress_ms);
        m_info[id].set<int>(meta::DURATION, duration_ms);
    }

    auto thumbnail = media_properties.Thumbnail();
    com_array<uint8_t> pixel_data_detached;
    uint8_t* data = NULL;
    if (thumbnail != nullptr && id == m_selected_player) { // only fetch cover for selected source
        auto stream = thumbnail.OpenReadAsync().get();

        auto decoder = BitmapDecoder::CreateAsync(stream).get();
        auto transform = BitmapTransform();
        uint32_t width, height;

        if (id == "Spotify.exe") { // The spotify player has branding on the thumbnail, cropping it out
            static const int Wcrop = 34;
            static const int Hcrop = 66;
            BitmapBounds spotify_bound;
            width = decoder.PixelWidth() - 2 * Wcrop;
            height = decoder.PixelHeight() - Hcrop;

            spotify_bound.X = Wcrop;
            spotify_bound.Y = 0;
            spotify_bound.Width = width;
            spotify_bound.Height = height;

            transform.Bounds(spotify_bound);

        } else {
            width = decoder.PixelWidth();
            height = decoder.PixelHeight();
        }

        auto pixel_data = decoder.GetPixelDataAsync(BitmapPixelFormat::Rgba8,
                                     BitmapAlphaMode::Premultiplied,
                                     transform,
                                     ExifOrientationMode::IgnoreExifOrientation,
                                     ColorManagementMode::ColorManageToSRgb)
                              .get();
        pixel_data_detached = pixel_data.DetachPixelData();
        data = (uint8_t*)pixel_data_detached.data();

        QImage image(data, width, height, QImage::Format_RGBA8888);
        auto current_source = music_sources::selected_source();
        m_covers[id] = image;

        /* We receive cover updates regardless of whether tuna is
         * configured to monitor WMC so if the current source isn't WMC, we
         * just save the cover for when the user switches to WMC*/
        if (curren_source.get() == this) {
            save_cover(image);
        }
    }
}

void wmc_source::handle_media_playback_info_change(GlobalSystemMediaTransportControlsSession session, PlaybackInfoChangedEventArgs const&)
{
    winrt::hstring t = session.SourceAppUserModelId();
    std::string id = winrt::to_string(t);

    auto info = session.GetPlaybackInfo();
    if (info == nullptr)
        return;
    std::lock_guard<std::mutex> lock(m_internal_mutex);

    switch (session.GetPlaybackInfo().PlaybackStatus()) {
    case GlobalSystemMediaTransportControlsSessionPlaybackStatus::Playing:
        m_info[id].set(meta::STATUS, play_state::state_playing);
        break;
    case GlobalSystemMediaTransportControlsSessionPlaybackStatus::Paused:
        m_info[id].set(meta::STATUS, play_state::state_paused);
        break;
    default:
    case GlobalSystemMediaTransportControlsSessionPlaybackStatus::Stopped:
        m_info[id].set(meta::STATUS, play_state::state_stopped);
        break;
    }
}

wmc_source::wmc_source()
    : music_source(S_SOURCE_WMC, T_SOURCE_WMC, new wmc)
{
    m_capabilities = CAP_NEXT_SONG | CAP_PREV_SONG | CAP_PLAY_PAUSE | CAP_STOP_SONG;
    supported_metadata({ meta::TITLE, meta::ARTIST, meta::ALBUM, meta::COVER, meta::PROGRESS, meta::STATUS });

    std::thread([this] {
        try {
            this->request_manager();
        } catch (...) {
            berr("[WMC] An error occured while getting the GlobalSystemMediaTransportControlsSessionManager");
        }
    }).detach();
}

void wmc_source::refresh()
{
    music_source::begin_refresh();
    std::lock_guard<std::mutex> lock(m_internal_mutex);
    if (m_info.find(m_selected_player) != m_info.end())
        m_current = m_info[m_selected_player];
    if (m_covers.find(m_selected_player) != m_covers.end())
        save_cover(m_covers[m_selected_player]);
}
