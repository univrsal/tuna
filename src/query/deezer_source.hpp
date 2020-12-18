/*************************************************************************
 * This file is part of tuna
 * github.con/univrsal/tuna
 * Copyright 2020 univrsal <uni@vrsal.cf>.
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

class deezer_source : public music_source {
    bool m_logged_in = false;
    QString m_api_client_id;
    QString m_api_secret;
    QString m_auth_code;
    QString m_token;

public:
    deezer_source();

    void load() override;
    void refresh() override;
    bool execute_capability(capability) override { return false; }
    bool enabled() const override { return true; }

    QString auth_code() const { return m_auth_code; }
    QString token() const { return m_token; }
    bool is_logged_in() const { return m_logged_in; }
};
