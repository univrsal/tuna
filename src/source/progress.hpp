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
#include "../query/music_source.hpp"
#include <obs-module.h>
namespace obs_sources {

class progress_source {
    uint32_t m_cx = 300, m_cy = 30;
    uint32_t m_fg {}, m_bg {};
    obs_source_t* m_source = nullptr;
    float m_progress = 0.f;
    float m_bounce_progress = 0.f;

    /* Song progress grabbed from current music source */
    int32_t m_synced_progress = 0;
    int32_t m_duration = 0;
    /* Song progress adjusted with frame time */
    float m_adjusted_progress = 0.f;
    bool m_bounce_up = true;
    play_state m_state = state_unknown;
    bool m_use_bg = true;
    bool m_hide_paused = false;

public:
    progress_source(obs_source_t* src, obs_data_t* settings);
    ~progress_source();

    inline void update(obs_data_t* settings);
    inline void tick(float seconds);
    inline void render(gs_effect_t* effect);

    uint32_t get_width() const { return m_cx; }
    uint32_t get_height() const { return m_cy; }
};

extern void register_progress();
}
