/**
 * This file is part of tuna
 * which is licensed under the GPL v2.0
 * See LICENSE or http://www.gnu.org/licenses
 * github.com/univrsal/tuna
 */
#pragma once
#include "music_source.hpp"
#include <jansson.h>
#include <string>
#include <QString>

class spotify_source : public music_source
{
    bool m_logged_in = false;
    std::string m_token = "";
    std::string m_creds = "";
    std::string m_auth_code = "";
    std::string m_refresh_token = "";

    /* system time in ms */
    uint64_t m_token_termination = 0;

    void parse_track_json(json_t* track);
public:
    spotify_source();

    void load() override;
    void save() override;
    void refresh() override;
    void load_gui_values() override;
    bool execute_capability(capability c) override;

    bool do_refresh_token(QString& log);
    bool new_token(QString& log);

    void set_auth_code(const std::string& auth_code)
    {
        m_auth_code = auth_code;
    }

    const std::string& auth_code() const { return m_auth_code; }
    const std::string& token() const { return m_token; }
    const std::string& refresh_token() const { return m_refresh_token; }
};

