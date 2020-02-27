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
#include "../query/music_source.hpp"
#include "../util/tuna_thread.hpp"

namespace obs_sources {
progress_source::progress_source(obs_source_t *src, obs_data_t *settings)
    : m_source(src)
{
    update(settings);
}

progress_source::~progress_source()
{

}

void progress_source::tick(float seconds)
{
	thread::mutex.lock();
	m_active = thread::thread_state;
	if (thread::thread_state) {
		auto src = music_sources::selected_source();
		const song* s;
		if (src && (s = src->song_info())) {
			if ((s->data() & CAP_DURATION) && (s->data() & CAP_PROGRESS)) {
				float progress = s->get_int_value('p');
				float duration = s->get_int_value('l');
				if (duration > 0)
					m_progress = progress / duration;
			}
		}
	}
	thread::mutex.unlock();

	if (!m_active) {
		float step = 0.001f * m_cx;
		if (m_bounce_up)
			m_bounce_progress = fmin(m_bounce_progress + seconds * step, 1.f);
		else
			m_bounce_progress = fmax(m_bounce_progress - seconds * step, 0.f);

		if (m_bounce_progress >= 1.f || m_bounce_progress <= 0.f)
			m_bounce_up = !m_bounce_up;
	}
}

void progress_source::render(gs_effect_t *effect)
{
	UNUSED_PARAMETER(effect);

	gs_effect_t *solid = obs_get_base_effect(OBS_EFFECT_SOLID);
	gs_eparam_t *color = gs_effect_get_param_by_name(solid, "color");
	gs_technique_t *tech = gs_effect_get_technique(solid, "Solid");

	struct vec4 bg, fg;
	vec4_from_rgba(&bg, m_bg);
	vec4_from_rgba(&fg, m_fg);

	gs_effect_set_vec4(color, &bg);

	gs_technique_begin(tech);
	gs_technique_begin_pass(tech, 0);
	gs_draw_sprite(0, 0, m_cx, m_cy);
	gs_technique_end_pass(tech);
	gs_technique_end(tech);

	gs_effect_set_vec4(color, &fg);

	if (m_active) {
		uint32_t progress = m_cx * m_progress;
		if (progress > 0) {
			gs_technique_begin(tech);
			gs_technique_begin_pass(tech, 0);
			gs_draw_sprite(0, 0, progress, m_cy);
			gs_technique_end_pass(tech);
			gs_technique_end(tech);
		}
	} else {
		uint32_t w = m_cx * .25;
		uint32_t x = m_bounce_progress * (m_cx - w);
		if (w > 0) {
			gs_matrix_push();
			gs_technique_begin(tech);
			gs_technique_begin_pass(tech, 0);
			gs_matrix_translate3f(x, 0, 0);
			gs_draw_sprite(0, 0, w, m_cy);
			gs_technique_end_pass(tech);
			gs_technique_end(tech);
			gs_matrix_pop();
		}
	}
}

void progress_source::update(obs_data_t *settings)
{
	m_cx = obs_data_get_int(settings, S_PROGRESS_CX);
	m_cy = obs_data_get_int(settings, S_PROGRESS_CY);
	m_fg = obs_data_get_int(settings, S_PROGRESS_FG);
	m_bg = obs_data_get_int(settings, S_PROGRESS_BG);
}

obs_properties_t *get_properties_for_progress(void *data)
{
    auto *p = obs_properties_create();
    obs_properties_add_color(p, S_PROGRESS_FG, T_PROGRESS_FG);
    obs_properties_add_color(p, S_PROGRESS_BG, T_PROGRESS_BG);
    obs_properties_add_int(p, S_PROGRESS_CX, T_PROGRESS_CX, 2, UINT16_MAX, 1);
    obs_properties_add_int(p, S_PROGRESS_CY, T_PROGRESS_CY, 2, UINT16_MAX, 1);
    return p;
}


void register_progress()
{
    obs_source_info si{};
    si.id = S_PROGRESS_ID;
    si.type = OBS_SOURCE_TYPE_INPUT;
    si.output_flags = OBS_SOURCE_VIDEO | OBS_SOURCE_CUSTOM_DRAW;
    si.get_properties = get_properties_for_progress;
    si.get_name = [](void *) { return T_PROGRESS_NAME; };
    si.create = [](obs_data_t *d, obs_source_t *s) {
        return static_cast<void *>(new progress_source(s, d));
    };
    si.destroy = [](void *data) { delete reinterpret_cast<progress_source *>(data); };
    si.get_width = [](void *data) { return reinterpret_cast<progress_source *>(data)->get_width(); };
    si.get_height = [](void *data) { return reinterpret_cast<progress_source *>(data)->get_height(); };
    si.get_defaults = [](obs_data_t *settings) {
        obs_data_set_default_int(settings, S_PROGRESS_FG, 0xFF10BC40);
        obs_data_set_default_int(settings, S_PROGRESS_BG, 0xFF323232);
        obs_data_set_default_int(settings, S_PROGRESS_CX, 300);
        obs_data_set_default_int(settings, S_PROGRESS_CY, 30);
    };

	si.update = [](void *data, obs_data_t *settings) { reinterpret_cast<progress_source *>(data)->update(settings); };
	si.video_tick = [](void *data, float seconds) { reinterpret_cast<progress_source *>(data)->tick(seconds); };
	si.video_render = [](void *data, gs_effect_t *effect) {
		reinterpret_cast<progress_source *>(data)->render(effect);
	};

	obs_register_source(&si);
}
}
