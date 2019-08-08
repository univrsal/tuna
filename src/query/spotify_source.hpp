/**
 * This file is part of tuna
 * which is licensed under the GPL v2.0
 * See LICENSE or http://www.gnu.org/licenses
 * github.com/univrsal/tuna
 */
#pragma once
#include "music_source.hpp"

class spotify_source : public music_source
{
    bool m_logged_in = false;
    const char* m_token = "";
public:
    spotify_source();

    void load() override;
    void refresh() override;
    bool execute_capability(capability c) override;
};

