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

#include "music_source.hpp"

class window_source : public music_source {
    QString m_title = "";
    QString m_search = "", m_replace = "", m_pause = "";
    uint16_t m_cut_begin = 0, m_cut_end;
    bool m_regex = false;

public:
    window_source();

    void load() override;

    void save() override;

    void refresh() override;

    bool execute_capability(capability c) override;

    void load_gui_values() override;

    bool valid_format(const QString& str) override;
};
