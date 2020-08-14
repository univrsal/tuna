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

#include "song.hpp"
#include <QDate>
#include <QObject>
#include <memory>
#include <stdint.h>

#include "src/util/utility.hpp"

/* clang-format off */

enum capability {
    CAP_TITLE = 1 << 0,         /* Song title               */
    CAP_ARTIST = 1 << 1,        /* Song artitst             */
    CAP_ALBUM = 1 << 2,         /* Album name               */
    CAP_RELEASE = 1 << 3,       /* Release date             */
    CAP_COVER = 1 << 4,         /* Cover image link         */
    CAP_LYRICS = 1 << 5,        /* Lyrics text link         */
    CAP_DURATION = 1 << 6,      /* Get song length in ms    */
    CAP_EXPLICIT = 1 << 7,      /* don't say swears         */
    CAP_DISC_NUMBER = 1 << 8,   /* Disc number              */
    CAP_TRACK_NUMBER = 1 << 9,  /* Track number on disk     */
    CAP_PROGRESS = 1 << 10,     /* Get play progress in ms  */
    CAP_STATUS = 1 << 11,       /* Get song playing satus   */
    CAP_LABEL = 1 << 12,        /* Song production label    */

    /* Control stuff */
    CAP_NEXT_SONG = 1 << 16,    /* Skip to next song        */
    CAP_PREV_SONG = 1 << 17,    /* Go to previous song      */
    CAP_PLAY_PAUSE = 1 << 18,   /* Toggle play/pause        */
    CAP_STOP_SONG = 1 << 19,    /* Stop playback            */
    CAP_VOLUME_UP = 1 << 20,    /* Increase volume          */
    CAP_VOLUME_DOWN = 1 << 21,  /* Decrease volume          */
    CAP_VOLUME_MUTE = 1 << 22,  /* Toggle mute              */
};

/* clang-format on */

class music_source : public QObject {
    Q_OBJECT
    const char *m_id, *m_name;

protected:
    uint32_t m_capabilities = 0x0;
    song m_current = {};

public:
    music_source(const char* id, const char* name);

    virtual ~music_source() { }

    /* util */
    uint32_t get_capabilities() const { return m_capabilities; }

    bool has_capability(capability c) const { return m_capabilities & ((uint16_t)c); }

    const song& song_info() const { return m_current; }
    void reset_info() { m_current.clear(); }
    const char* name() const { return m_name; }
    const char* id() const { return m_id; }

    /* Abstract stuff */
    virtual bool enabled() const = 0;
    /* Save/load config values */
    virtual void load() = 0;
    virtual void save() = 0;
    /* Perform information query */
    virtual void refresh() = 0;
    /* Execute and return true if successful */
    virtual bool execute_capability(capability c) = 0;
    virtual void set_gui_values() = 0;
    virtual bool valid_format(const QString& str) = 0;
};

namespace music_sources {
extern QList<std::shared_ptr<music_source>> instances;
extern void init();
extern void load();
extern void save();
extern void set_gui_values();
extern void deinit();
extern void select(const char* id);
extern std::shared_ptr<music_source> selected_source();

template <class T>
std::shared_ptr<T> get(const char* id)
{
    for (const auto src : instances) {
        if (strcmp(src->id(), id) == 0) {
            return std::dynamic_pointer_cast<T>(src);
        }
    }

    throw std::invalid_argument("Couldn't get music source for provided id");
}
}
