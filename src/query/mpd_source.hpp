/*************************************************************************
 * This file is part of tuna
 * github.con/univrsal/tuna
 * Copyright 2020 univrsal <universailp@web.de>.
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

#ifdef UNIX
#include <mpd/client.h>

class mpd_source : public music_source {
    struct mpd_connection* m_connection = nullptr;
    struct mpd_status* m_status = nullptr;
    struct mpd_song* m_mpd_song = nullptr;
    enum mpd_state m_mpd_state;
    QString m_address;
    QString m_base_folder;
    bool m_connected;
    uint16_t m_port;
    bool m_local;

public:
    mpd_source();
    ~mpd_source() override;

    void load() override;
    void save() override;
    void refresh() override;
    void set_gui_values() override;
    bool execute_capability(capability c) override;
    bool valid_format(const QString& str) override;
    const char* name() const override;
    const char* id() const override;
    bool enabled() const override;

private:
    void connect();

    void disconnect();
};
#else

class mpd_source : public music_source {
public:
    mpd_source() = default;
    ~mpd_source() {}
    void load() override {}
    void save() override {}
    void refresh() override {}
    void set_gui_values() override {}
    bool execute_capability(capability c) override { return false; }
    bool valid_format(const QString& str) override { return false; }
    const char* name() const override { return T_SOURCE_MPD; }
    const char* id() const override { return S_SOURCE_MPD; }
    bool enabled() const override { return false; }
};
#endif
