/**
 ** This file is part of the tuna project.
 ** Copyright 2023 univrsal <uni@vrsal.xyz>.
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

#include "music_source.hpp"
#include <atomic>
#include <mutex>
#include <thread>
#include "tinympris.h"

class mpris_source : public music_source {
    friend class mpris; // so the gui can access the mutex and data
    bool m_stopped = false;
    bool init_mpris();

    std::mutex m_internal_mutex;
    std::thread m_internal_thread;
    std::atomic<bool> m_thread_flag;

    struct SongInfo {
        song metadata {};
        int64_t update_time {};
    };

    QMap<QString, QString> m_players {};
    QMap<QString, SongInfo> m_info {};
    QString m_selected_player {};
    void handle_mpris_event(tinympris_event_type event, const tinympris_player* player);

    tinympris_ctx* m_ctx{};

    tinympris_player* get_selected_player();
    void internal_refresh();
public:
    mpris_source();
    ~mpris_source();

    void load() override;
    void refresh() override;
    bool execute_capability(capability) override;
    bool enabled() const override { return true; }

};
