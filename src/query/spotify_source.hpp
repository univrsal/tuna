/**
 * This file is part of tuna
 * which is licensed under the GPL v2.0
 * See LICENSE or http://www.gnu.org/licenses
 * github.com/univrsal/tuna
 */
#pragma once
#include "music_source.hpp"
#include <QString>

class spotify_source : public music_source
{
    bool m_logged_in = false;
    QString m_token = "";
    QString m_creds = nullptr;
    QString m_auth_code = "";
    QString m_refresh_token = "";

    /* system time in ms */
    uint64_t m_token_termination = 0;
public:
    spotify_source();

    void load() override;
    void refresh() override;
    bool execute_capability(capability c) override;

    bool do_refresh_token();
    bool new_token();

    void set_auth_code(const QString& auth_code)
    {
        m_auth_code = auth_code;
    }

    const QString& auth_code() const { return m_auth_code; }
    const QString& token() const { return m_token; }
    const QString& refresh_token() const { return m_refresh_token; }
private:
    bool request_token(const char* grant_type, const char* code_id,
                       const char* code);
};

