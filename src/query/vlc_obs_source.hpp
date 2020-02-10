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
#include <QString>
#include <obs-module.h>

class vlc_obs_source : public music_source {
    const char* m_target_source_name = nullptr;
    obs_weak_source_t* m_weak_src = nullptr;
    struct vlc_source* get_vlc();

    void reload();

public:
    vlc_obs_source();
    ~vlc_obs_source();

    void load() override;
    void save() override;
    void refresh() override;
    void set_gui_values() override;
    bool execute_capability(capability c) override;
    bool valid_format(const QString& str) override;
    const char* name() const override;
    const char* id() const override;
    bool enabled() const override;
};

#ifdef DISABLE_TUNA_VLC
vlc_obs_source::vlc_obs_source()
{
}
vlc_obs_source::~vlc_obs_source() {}

void vlc_obs_source::load() {}
void vlc_obs_source::save() {}
void vlc_obs_source::refresh() {}
void vlc_obs_source::load_gui_values() {}
bool vlc_obs_source::execute_capability(capability c) { return true; }
bool vlc_obs_source::valid_format(const QString& str) { return true; }
struct vlc_source* vlc_obs_source::get_vlc() { return nullptr; }
bool vlc_obs_source::enabled() const { return false; }

const char* vlc_obs_source::name() const
{
    return T_SOURCE_VLC;
}

const char* vlc_obs_source::id() const
{
    return S_SOURCE_VLC;
}
#endif
