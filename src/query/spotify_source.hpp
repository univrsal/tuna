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
#include <QJsonValue>
#include <QString>

class spotify_source : public music_source {
    bool m_logged_in = false;
    QString m_token = "";
    QString m_creds = "";
    QString m_auth_code = "";
    QString m_refresh_token = "";

    /* epoch time in seconds */
    int64_t m_token_termination = 0;

    uint64_t m_timeout_length = 0, /* Rate limit timeout length */
        m_timout_start = 0; /* Timeout start */
    void parse_track_json(const QJsonValue& track);

public:
    spotify_source();

    void load() override;

    void save() override;

    void refresh() override;

    void load_gui_values() override;

    bool execute_capability(capability c) override;

    bool valid_format(const QString& str) override;

    bool do_refresh_token(QString& log);

    bool new_token(QString& log);

    void set_auth_code(const QString& auth_code) { m_auth_code = auth_code; }

    const QString& auth_code() const { return m_auth_code; }

    const QString& token() const { return m_token; }

    const QString& refresh_token() const { return m_refresh_token; }
};
