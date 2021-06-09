/*************************************************************************
 * This file is part of tuna
 * github.com/univrsal/tuna
 * Copyright 2021 univrsal <uni@vrsal.de>.
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
	const char *m_target_source_name = nullptr;
	obs_weak_source_t *m_weak_src = nullptr;
	/* Only log conversion issues once per file */
	bool reload();

	void load_vlc_source();

public:
	vlc_obs_source();
	~vlc_obs_source();

	void load() override;
	void refresh() override;
	bool execute_capability(capability c) override;
	bool valid_format(const QString &str) override;
	bool enabled() const override;
};
