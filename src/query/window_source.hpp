/*************************************************************************
 * This file is part of tuna
 * github.com/univrsal/tuna
 * Copyright 2022 univrsal <uni@vrsal.xyz>.
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
#include <utility>
#include <vector>

class window_source : public music_source {
    QString m_title = "";
    QString m_process_name = "";
    QString m_search = "", m_replace = "", m_pause = "";
    uint16_t m_cut_begin = 0, m_cut_end;
    bool m_regex = false, m_use_process_name;

    QString get_title(const std::vector<std::string>& windows);
    QString get_title(const std::vector<std::pair<std::string, std::string>>& processes);

public:
    window_source();

    void load() override;
    void refresh() override;
    bool execute_capability(capability c) override;
    bool enabled() const override;
};
