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

#pragma once
#include "../util/constants.hpp"
#include "music_source.hpp"

#include <mpd/client.h>

class mpd_source : public music_source {
    bool m_stopped = false;
    QString m_address;
    QString m_base_folder;
    QString m_song_file_path;
    uint16_t m_port;
    bool m_local;
    mpd_connection* m_connection {};
    int m_connection_error_count {};

public:
    mpd_source();
    ~mpd_source() { close_connection(); }

    void load() override;
    void refresh() override;
    bool execute_capability(capability c) override;
    bool enabled() const override;
    void handle_cover() override;
    void handle_lyrics() override;
    void reset_info() override;

private:
    void ensure_connection();

    void close_connection()
    {
        if (m_connection)
            mpd_connection_free(m_connection);
        m_connection = nullptr;
    }
    uint64_t m_last_error_log = 0;
};
