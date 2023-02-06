/**
 ** This file is part of the tuna project.
 ** Copyright 2023 univrsal <uni@vrsal.xyz>.
 **
 ** This program is free software: you can redistribute it and/or modify
 ** it under the terms of the GNU General Public License as published by
 ** the Free Software Foundation, either version 3 of the License, or
 ** (at your option) any later version.
 **
 ** This program is distributed in the hope that it will be useful,
 ** but WITHOUT ANY WARRANTY; without even the implied warranty of
 ** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 ** GNU General Public License for more details.
 **
 ** You should have received a copy of the GNU General Public License
 ** along with this program.  If not, see <http://www.gnu.org/licenses/>.
 **/

#include "music_source.hpp"
#include <atomic>
#include <dbus/dbus.h>
#include <mutex>
#include <thread>

class mpris_source : public music_source {
    friend class mpris;
    bool m_stopped = false;
    DBusConnection* m_dbus_connection {};

    bool init_dbus_session();
    bool dbus_add_matches();
    bool dbus_register_names();
    char* dbus_get_name_owner(const char*);

    DBusHandlerResult handle_dbus(DBusMessage*);
    DBusHandlerResult handle_mpris(DBusMessage*);

    void parse_array(DBusMessageIter* iter, QString const& player, int level = 0);
    void parse_metadata(DBusMessageIter* iter, QString const& player, int level = 0);

    inline void ensure_entry(QString const& player)
    {
        if (!m_info.contains(player))
            m_info[player] = {};
    }

    std::mutex m_internal_mutex;
    std::thread m_internal_thread;
    std::atomic<bool> m_thread_flag;

    struct SongInfo {
        song metadata {};
        int64_t update_time {};
    };

    QMap<QString, QString> m_players {};
    QMap<QString, SongInfo> m_info {};
    QString m_selected_player {};
    bool init_dbus();

public:
    mpris_source();
    ~mpris_source();

    void load() override;
    void refresh() override;
    void internal_refresh();
    bool execute_capability(capability) override { return false; }
    bool enabled() const override { return true; }

    DBusHandlerResult handle_message(DBusConnection*, DBusMessage*);
};
