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

#include "progress.hpp"
#include "../util/constants.hpp"
#include "../util/tuna_thread.hpp"

namespace obs_sources {
progress_source::progress_source(obs_source_t* src, obs_data_t* settings)
    : m_source(src)
{
    update(settings);
}

progress_source::~progress_source() {}

void progress_source::tick(float seconds)
{
    song tmp;
    tuna_thread::copy_mutex.lock();
    tmp = tuna_thread::copy;
    tuna_thread::copy_mutex.unlock();
    m_state = tmp.state();
    if (m_state == state_playing) {
        seconds *= 1000; /* s -> ms */
        if ((tmp.data() & CAP_DURATION) && (tmp.data() & CAP_PROGRESS)) {
            if (tmp.get_int_value('p') != m_synced_progress) {
                m_synced_progress = tmp.get_int_value('p');
                m_adjusted_progress = m_synced_progress + seconds;
            } else {
                m_adjusted_progress += seconds;
            }
        } else if (m_synced_progress > 0) {
            m_adjusted_progress += seconds;
        }

        float duration = tmp.get_int_value('l');
        if (duration > 0)
            m_progress = m_adjusted_progress / duration;
    } else if (m_state == state_paused) {
        float step = 0.0005f * m_cx;
        if (m_bounce_up)
            m_bounce_progress = fmin(m_bounce_progress + seconds * step, 1.f);
        else
            m_bounce_progress = fmax(m_bounce_progress - seconds * step, 0.f);

        if (m_bounce_progress >= 1.f || m_bounce_progress <= 0.f)
            m_bounce_up = !m_bounce_up;
    }
}

void progress_source::render(gs_effect_t* effect)
{
    UNUSED_PARAMETER(effect);
    if (m_hide_paused && m_state >= state_paused)
        return;

    gs_effect_t* solid = obs_get_base_effect(OBS_EFFECT_SOLID);
    gs_eparam_t* color = gs_effect_get_param_by_name(solid, "color");
    gs_technique_t* tech = gs_effect_get_technique(solid, "Solid");

    struct vec4 bg, fg;
    vec4_from_rgba(&bg, m_bg);
    vec4_from_rgba(&fg, m_fg);

    if (m_use_bg) {
        gs_effect_set_vec4(color, &bg);

        gs_technique_begin(tech);
        gs_technique_begin_pass(tech, 0);
        gs_draw_sprite(nullptr, 0, m_cx, m_cy);
        gs_technique_end_pass(tech);
        gs_technique_end(tech);
    }

    gs_effect_set_vec4(color, &fg);

    if (m_state == state_playing) {
        uint32_t progress = static_cast<uint32_t>(m_cx * m_progress);
        if (progress > 0) {
            gs_technique_begin(tech);
            gs_technique_begin_pass(tech, 0);
            gs_draw_sprite(nullptr, 0, progress, m_cy);
            gs_technique_end_pass(tech);
            gs_technique_end(tech);
        }
    } else if (m_state == state_paused) {
        uint32_t w = static_cast<uint32_t>(m_cx * .25);
        uint32_t x = static_cast<uint32_t>(m_bounce_progress * (m_cx - w));
        if (w > 0) {
            gs_matrix_push();
            gs_technique_begin(tech);
            gs_technique_begin_pass(tech, 0);
            gs_matrix_translate3f(x, 0, 0);
            gs_draw_sprite(nullptr, 0, w, m_cy);
            gs_technique_end_pass(tech);
            gs_technique_end(tech);
            gs_matrix_pop();
        }
    }
}

void progress_source::update(obs_data_t* settings)
{
    m_cx = static_cast<uint32_t>(obs_data_get_int(settings, S_PROGRESS_CX));
    m_cy = static_cast<uint32_t>(obs_data_get_int(settings, S_PROGRESS_CY));
    m_fg = static_cast<uint32_t>(obs_data_get_int(settings, S_PROGRESS_FG));
    m_bg = static_cast<uint32_t>(obs_data_get_int(settings, S_PROGRESS_BG));
    m_use_bg = obs_data_get_bool(settings, S_PROGRESS_USE_BG);
    m_hide_paused = obs_data_get_bool(settings, S_PROGRESS_HIDE_PAUSED);
}

static bool use_bg_changed(obs_properties_t* props, obs_property_t* property, obs_data_t* settings)
{
    UNUSED_PARAMETER(property);
    auto* prop = obs_properties_get(props, S_PROGRESS_BG);
    obs_property_set_visible(prop, obs_data_get_bool(settings, S_PROGRESS_USE_BG));
    return true;
}

obs_properties_t* get_properties_for_progress(void* data)
{
    UNUSED_PARAMETER(data);
    auto* p = obs_properties_create();
    obs_properties_add_color(p, S_PROGRESS_FG, T_PROGRESS_FG);
    auto* use_bg = obs_properties_add_bool(p, S_PROGRESS_USE_BG, T_PROGRESS_USE_BG);
    obs_property_set_modified_callback(use_bg, use_bg_changed);
    obs_properties_add_color(p, S_PROGRESS_BG, T_PROGRESS_BG);
    obs_properties_add_int(p, S_PROGRESS_CX, T_PROGRESS_CX, 2, UINT16_MAX, 1);
    obs_properties_add_int(p, S_PROGRESS_CY, T_PROGRESS_CY, 2, UINT16_MAX, 1);
    obs_properties_add_bool(p, S_PROGRESS_HIDE_PAUSED, T_PROGRESS_HIDE_PAUSED);
    return p;
}

void register_progress()
{
    obs_source_info si {};
    si.id = S_PROGRESS_ID;
    si.type = OBS_SOURCE_TYPE_INPUT;
    si.output_flags = OBS_SOURCE_VIDEO | OBS_SOURCE_CUSTOM_DRAW;
    si.get_properties = get_properties_for_progress;
    si.get_name = [](void*) { return T_PROGRESS_NAME; };
    si.create = [](obs_data_t* d, obs_source_t* s) { return static_cast<void*>(new progress_source(s, d)); };
    si.destroy = [](void* data) { delete reinterpret_cast<progress_source*>(data); };
    si.get_width = [](void* data) { return reinterpret_cast<progress_source*>(data)->get_width(); };
    si.get_height = [](void* data) { return reinterpret_cast<progress_source*>(data)->get_height(); };
    si.get_defaults = [](obs_data_t* settings) {
        obs_data_set_default_int(settings, S_PROGRESS_FG, 0xFF10BC40);
        obs_data_set_default_int(settings, S_PROGRESS_BG, 0xFF323232);
        obs_data_set_default_int(settings, S_PROGRESS_CX, 300);
        obs_data_set_default_int(settings, S_PROGRESS_CY, 30);
        obs_data_set_default_bool(settings, S_PROGRESS_HIDE_PAUSED, false);
    };

    si.update = [](void* data, obs_data_t* settings) { reinterpret_cast<progress_source*>(data)->update(settings); };
    si.video_tick = [](void* data, float seconds) { reinterpret_cast<progress_source*>(data)->tick(seconds); };
    si.video_render = [](void* data, gs_effect_t* effect) {
        reinterpret_cast<progress_source*>(data)->render(effect);
    };

    obs_register_source(&si);
}
}
