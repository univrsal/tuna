/*************************************************************************
 * This file is part of tuna
 * git.vrsal.xyz/alex/tuna
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

#include "song.hpp"
#include <QDate>
#include <QObject>
#include <cstdint>
#include <memory>
#include <vector>

#include "../gui/tuna_gui.hpp"

/* clang-format off */

enum capability : uint32_t {
    CAP_NEXT_SONG = 1 << 0,         /* Skip to next song        */
    CAP_PREV_SONG = 1 << 1,         /* Go to previous song      */
    CAP_PLAY_PAUSE = 1 << 2,        /* Toggle play/pause        */
    CAP_STOP_SONG = 1 << 3,         /* Stop playback            */
    CAP_VOLUME_UP = 1 << 4,         /* Increase volume          */
    CAP_VOLUME_DOWN = 1 << 5,       /* Decrease volume          */
    CAP_VOLUME_MUTE = 1 << 6,       /* Toggle mute              */
};

/* clang-format on */

class music_source : public QObject {
    Q_OBJECT
    const char *m_id, *m_name;

protected:
    std::array<bool, meta::COUNT> m_supported_metadata {};
    uint32_t m_capabilities = 0x0;
    song m_current = {}, m_prev = {};
    source_widget* m_settings_tab = nullptr;

    void begin_refresh() { m_prev = m_current; }

    bool download_missing_cover();

    void supported_metadata(std::vector<meta::type> data)
    {
        for (auto const& d : data)
            m_supported_metadata[d] = true;
    }

    template<class T>
    T* get_ui()
    {
        return static_cast<T*>(m_settings_tab);
    }

public:
    music_source(const char* id, const char* name, source_widget* w = nullptr);

    virtual ~music_source() { }

    /* util */
    uint32_t get_capabilities() const { return m_capabilities; }

    bool has_capability(capability c) const { return m_capabilities & ((uint16_t)c); }

    const song& song_info() const { return m_current; }
    virtual void reset_info()
    {
        m_current.clear();
        m_prev.clear();
    }
    const char* name() const { return m_name; }
    const char* id() const { return m_id; }

    /* Abstract stuff */
    virtual bool enabled() const = 0;
    /* Save/load config values */
    virtual void load();
    virtual void save();
    /* Perform information query */
    virtual void refresh() = 0;
    /* Execute and return true if successful */
    virtual bool execute_capability(capability c) = 0;
    virtual void set_gui_values();
    virtual void handle_cover();

    source_widget* get_settings_tab() { return m_settings_tab; }

    bool provides_metadata(std::vector<meta::type> const& m)
    {
        for (auto const& d : m) {
            if (!m_supported_metadata[d] && d != meta::NONE)
                return false;
        }
        return true;
    }
};

namespace music_sources {
extern QList<std::shared_ptr<music_source>> instances;
extern void init();
extern void load();
extern void save();
extern void set_gui_values();
extern void deinit();
extern void select(const char* id);
extern std::shared_ptr<music_source> selected_source_unsafe();
extern std::shared_ptr<music_source> selected_source();

template<class T>
std::shared_ptr<T> get(const char* id)
{
    for (const auto& src : qAsConst(instances)) {
        if (strcmp(src->id(), id) == 0) {
            return std::dynamic_pointer_cast<T>(src);
        }
    }
    return nullptr;
}
}
