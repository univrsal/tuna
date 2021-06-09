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

class lastfm_source : public music_source {
	QString m_username, m_api_key;
	bool m_custom_api_key = false;
	uint64_t m_next_refresh = 0;
	void parse_song(const QJsonObject &s);

public:
	lastfm_source();

	void load() override;
	void refresh() override;
	bool execute_capability(capability c) override;
	bool enabled() const override;
};
