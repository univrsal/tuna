/**
 * This file is part of tuna
 * which is licensed under the GPL v2.0
 * See LICENSE or http://www.gnu.org/licenses
 * github.com/univrsal/tuna
 */
#pragma once
#ifdef LINUX
#include "music_source.hpp"
#include <mpd/client.h>

class mpd_source : public music_source
{
    struct mpd_connection* m_connection = nullptr;
    struct mpd_status* m_status = nullptr;
    struct mpd_song* m_mpd_song = nullptr;
    enum mpd_state m_mpd_state;
    const char* m_address;
    bool m_connected;
    uint16_t m_port;
    bool m_local;
public:
    mpd_source();
    ~mpd_source() override;

    void load() override;
    void save() override;
    void refresh() override;
    void load_gui_values() override;
    bool execute_capability(capability c) override;

private:
    void connect();
    void disconnect();

};
#endif