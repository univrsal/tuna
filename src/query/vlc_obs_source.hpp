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
#pragma once
#include "music_source.hpp"
#include <string>
#include <obs-module.h>

class vlc_obs_source : public music_source
{
    std::string m_target_source_name;
    obs_weak_source_t *m_weak_src = nullptr;
    /* If obs version changed since the plugin was
     * compiled, vlc source will be disabled since its
     * functionality is based on some hacks that can easily
     * break in between obs versions */

    struct vlc_source *get_vlc();
    bool m_lib_vlc_loaded = false;
public:
    vlc_obs_source();
    ~vlc_obs_source();

    void load() override;
    void save() override;
    void refresh() override;
    void load_gui_values() override;
    bool execute_capability(capability c) override;
};

#if DISABLE_TUNA_VLC

/* Stubs */
vlc_obs_source::vlc_obs_source() {}
vlc_obs_source::~vlc_obs_source() {}

void vlc_obs_source::load() {}
void vlc_obs_source::save() {}
void vlc_obs_source::refresh() {}
void vlc_obs_source::load_gui_values() {}
void vlc_obs_source::execute_capability(capability c) {}

#endif
