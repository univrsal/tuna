/**
 ** This file is part of the tuna project.
 ** Copyright 2026 univrsal <uni@vrsal.cc>.
 **
 ** This program is free software: you can redistribute it and/or modify
 ** it under the terms of the GNU General Public License as published by
 ** the Free Software Foundation, either version 3 of the License, or
 ** (at your option) any later version.
 **
 ** This program is distributed in the hope that it will be useful,
 ** but WITHOUT ANY WARRANTY; without even the implied warranty of
 ** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 ** GNU General Public License for more details.
 **
 ** You should have received a copy of the GNU General Public License
 ** along with this program.  If not, see <http://www.gnu.org/licenses/>.
 **/

#include "mpris_source.hpp"
#include "../gui/widgets/mpris.hpp"
#include "../util/config.hpp"
#include "../util/constants.hpp"
#include "../util/utility.hpp"

#define TINYMPRIS_IMPLEMENTATION
#include "tinympris.h"

static QString correct_art_url(const char* url)
{
    auto str = utf8_to_qt(url);
    // Don't encode if it's a file, data:image or http(s) url
    if (str.startsWith("data:image/") || str.startsWith("file://") || str.startsWith("http://") || str.startsWith("https://")) {
        return str;
    }
    return QUrl::toPercentEncoding(utf8_to_qt(url)).replace("%2F", "/").replace("file%3A", "file:").replace("%3A", ":"); // idk why it encodes slashes
}

void mpris_source::handle_mpris_event(tinympris_event_type event, const tinympris_player* player)
{
    if (event == TINYMPRIS_EVENT_PLAYER_ADDED || event == TINYMPRIS_EVENT_PLAYER_UPDATED) {
        SongInfo info{};
        info.update_time = util::epoch();
        info.metadata.set(meta::TITLE, utf8_to_qt(player->metadata.title));
        info.metadata.set(meta::ALBUM, utf8_to_qt(player->metadata.album));
        info.metadata.set(meta::TRACK_ID, utf8_to_qt(player->metadata.track_id));
        info.metadata.set(meta::COVER, correct_art_url(player->metadata.art_url));
        info.metadata.set<int>(meta::DISC_NUMBER, player->metadata.disc_number);
        info.metadata.set<int>(meta::TRACK_NUMBER, player->metadata.track_number);
        info.metadata.set<int>(meta::DURATION, player->metadata.length_us / 1000);
        info.metadata.set<int>(meta::PROGRESS, player->position_us / 1000);

        if (player->status == TINYMPRIS_STATUS_PAUSED) {
            info.metadata.set<int>(meta::STATUS, state_paused);
        } else if (player->status == TINYMPRIS_STATUS_PLAYING) {
            info.metadata.set<int>(meta::STATUS, state_playing);
        } else {
            info.metadata.set<int>(meta::STATUS, state_stopped);
        }
        //
        QStringList artists{};
        for (int i = 0; i < player->metadata.artist_count; ++i) {
            artists.append(utf8_to_qt(player->metadata.artists[i]));
        }
        info.metadata.set(meta::ARTIST, artists);
        m_info[player->identity] = info;
        m_players[player->identity] = utf8_to_qt(player->identity);
    } else if (event == TINYMPRIS_EVENT_PLAYER_REMOVED) {
        m_info.remove(player->identity);
        m_players.remove(player->identity);
    }
}

bool mpris_source::init_mpris()
{
    m_ctx = tinympris_init( [](tinympris_event_type event, const tinympris_player* player, void* user_data) {
        const auto self = static_cast<mpris_source*>(user_data);
        self->handle_mpris_event(event, player);
    }, this);
    return !!m_ctx;
}

mpris_source::mpris_source()
    : music_source(S_SOURCE_MPRIS, T_SOURCE_MPRIS, new mpris)
    , m_thread_flag(false)
{
    m_capabilities = CAP_NEXT_SONG | CAP_PREV_SONG | CAP_PLAY_PAUSE |
        CAP_STOP_SONG;
    supported_metadata({ meta::ALBUM, meta::TITLE, meta::ARTIST, meta::STATUS, meta::DURATION, meta::DISC_NUMBER, meta::TRACK_NUMBER, meta::PROGRESS, meta::COVER });
    bdebug("[MPRIS] Initialising dbus session for mpris source");
    if (init_mpris()) {
        m_thread_flag = true;
        m_internal_thread = std::thread([](mpris_source* s) {
            util::set_thread_name("tuna-mpris");
            s->internal_refresh();
        },
            this);
    } else {
        berr("[MPRIS] Failed to initialize mpris source");
    }
}

mpris_source::~mpris_source()
{
    m_thread_flag = false;
    m_internal_thread.join();
    tinympris_shutdown(m_ctx);
}

void mpris_source::load()
{
    music_source::load();
    CDEF_STR(CFG_MPRIS_PLAYER, "");
    m_selected_player = utf8_to_qt(CGET_STR(CFG_MPRIS_PLAYER));
}

void mpris_source::refresh()
{
    begin_refresh();
    std::scoped_lock lock(m_internal_mutex);
    if (m_info.contains(m_selected_player)) {
        m_current = m_info[m_selected_player].metadata;
    } else {
        qint64 most_recent {};
        QString recent_key {};
        for (auto const& p : m_info.keys()) {
            auto const& info = m_info[p];
            if (info.update_time > most_recent) {
                most_recent = info.update_time;
                recent_key = p;
            }
        }

        if (!recent_key.isEmpty()) {
            m_current = m_info[recent_key].metadata;
        }
    }
}

tinympris_player* mpris_source::get_selected_player()
{
    std::scoped_lock lock(m_internal_mutex);
    for (int i = 0; i < m_ctx->count; i++) {
        binfo("%s // %s", m_ctx->players[i]->identity, qt_to_utf8(m_selected_player));
        if (utf8_to_qt(m_ctx->players[i]->identity) == m_selected_player) {
                return m_ctx->players[i];
        }
    }
    return nullptr;
}

void mpris_source::internal_refresh()
{
    while (m_thread_flag) {
        // we have to hold the lock here to make sure we can access the player array
        // in other places. calling process() can change it if a player is added or removed,
        // so we need to make sure we don't access it while that happens
        m_internal_mutex.lock();
        tinympris_process(m_ctx, 10);
        m_internal_mutex.unlock();
        std::this_thread::sleep_for(std::chrono::milliseconds(90));
    }
}

bool mpris_source::execute_capability(capability cap)
{
    auto* current_player = get_selected_player();
    if (!current_player) {
        return false;
    }
    if (cap == CAP_NEXT_SONG && current_player->can_go_next) {
        return tinympris_next(m_ctx, current_player->well_known_name);
    }
    if (cap == CAP_PREV_SONG && current_player->can_go_previous) {
        return tinympris_previous(m_ctx, current_player->well_known_name);
    }
    if (cap == CAP_PLAY_PAUSE) {
       return tinympris_play_pause(m_ctx, current_player->well_known_name);
    }
    if (cap == CAP_STOP_SONG && current_player->can_pause) {
        return tinympris_stop(m_ctx, current_player->well_known_name);
    }
    return false;
}