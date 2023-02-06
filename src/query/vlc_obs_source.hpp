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
#include "music_source.hpp"
#include <QString>
#include <obs-module.h>
#include <obs.hpp>

class vlc_obs_source : public music_source {
    std::string m_target_source_name {};
    std::string m_target_scene {};
    OBSWeakSourceAutoRelease m_weak_src {};
    bool reload();

    void load_vlc_source();

    std::string get_target_source_name();
    std::string get_current_scene_name();
    int m_index = 0;

    // Gets the currently tracked obs vlc source with increased ref count
    obs_source_t* get_source()
    {
        auto* src = obs_weak_source_get_source(m_weak_src);
        if (!src)
            m_weak_src = {};
        return src;
    }

public:
    vlc_obs_source();
    ~vlc_obs_source();

    void load() override;
    void refresh() override;
    bool execute_capability(capability c) override;
    bool enabled() const override;

    void next_vlc_source();
    void prev_vlc_source();
    void set_gui_values() override;
};
