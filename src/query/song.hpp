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
#include <QJsonArray>
#include <QJsonObject>
#include <QList>
#include <QMap>
#include <QString>
#include <QVariant>
#include <array>
#include <stdint.h>

class QJsonObject;

enum date_precision { prec_day,
    prec_month,
    prec_year,
    prec_unknown };

enum play_state { state_playing,
    state_paused,
    state_stopped,
    state_unknown };

namespace meta {
static const char* ids[] = {
    "none",
    "title",
    "artists",
    "album",
    "release_date",
    "release_day",
    "release_month",
    "release_year",
    "cover_path",
    "lyrics",
    "duration",
    "explicit",
    "disc_number",
    "track_number",
    "progress",
    "status_id",
    "label",
    "file_name",

    /* VLC source specific */
    "genre",
    "copyright",
    "description",
    "rating",
    "date",
    "setting",
    "url",
    "language",
    "now_playing",
    "publisher",
    "encoded_by",
    "artwork_url",
    "track_id",
    "track_total",
    "director",
    "season",
    "episode",
    "show_name",
    "actors",
    "album_artist",
    "disc_total",

    /* Misc */
    "playback_date",
    "playback_time",

    /* Spotify specific */
    "context",
    "context_url",
    "context_external_url",
    "playlist_name",

    "count"
};

enum type : uint8_t {
    NONE,
    TITLE,
    ARTIST,
    ALBUM,
    RELEASE,
    RELEASE_DAY,
    RELEASE_MONTH,
    RELEASE_YEAR,
    COVER,
    LYRICS,
    DURATION,
    EXPLICIT,
    DISC_NUMBER,
    TRACK_NUMBER,
    PROGRESS,
    STATUS,
    LABEL,
    FILE_NAME,

    /* VLC source specific */
    GENRE,
    COPYRIGHT,
    DESCRIPTION,
    RATING,
    DATE,
    SETTING,
    URL,
    LANGUAGE,
    NOW_PLAYING,
    PUBLISHER,
    ENCODED_BY,
    ARTWORK_URL,
    TRACK_ID,
    TRACK_TOTAL,
    DIRECTOR,
    SEASON,
    EPISODE,
    SHOW_NAME,
    ACTORS,
    ALBUM_ARTIST,
    DISC_TOTAL,

    /* misc */
    PLAYBACK_DATE,
    PLAYBACK_TIME,

    /* Spotify specific */
    CONTEXT,
    CONTEXT_URL,
    CONTEXT_EXTERNAL_URL,
    PLAYLIST_NAME,

    COUNT
};
static_assert(sizeof(ids) / sizeof(char*) - 1 == COUNT, "");
}

class song {
    date_precision m_release_precision;
    QJsonObject m_data;

public:
    song();
    void update_release_precision();
    void clear();

    bool has_cover_lookup_information() const;

    template<meta::type T>
    void reset()
    {
        m_data[meta::ids[T]] = QJsonValue();
    }

    bool has(meta::type id) const
    {
        return m_data.contains(meta::ids[id]) && !m_data[meta::ids[id]].isNull();
    }

    template<class T = QString>
    T get(meta::type id, T const& def = {}) const;

    template<class T>
    void set(meta::type id, T const& v);

    template<class T>
    bool is(meta::type id) const;

    QJsonObject const& data() const { return m_data; }
    date_precision release_precision() const { return m_release_precision; }

    bool operator==(const song& other) const;
    bool operator!=(const song& other) const;

    void to_json(QJsonObject& obj) const;
    void from_json(const QJsonObject& obj);
};

template<>
inline QString song::get(meta::type id, QString const& def) const
{
    if (has(id)) {
        auto const& data = m_data[meta::ids[id]];
        Q_ASSERT(data.isString());
        return data.toString();
    }
    return def;
}

template<>
inline int song::get(meta::type id, int const& def) const
{
    if (has(id)) {
        auto const& data = m_data[meta::ids[id]];
        Q_ASSERT(data.isDouble());
        return data.toInt();
    }
    return def;
}

template<>
inline QStringList song::get(meta::type id, QStringList const& def) const
{
    if (has(id)) {
        auto const& data = m_data[meta::ids[id]];
        Q_ASSERT(data.isArray());
        QStringList l;
        auto const& arr = data.toArray();
        for (auto const& v : arr) {
            if (v.isString())
                l.append(v.toString());
        }
        return l;
    }
    return def;
}

template<>
inline bool song::get(meta::type id, bool const& def) const
{
    if (has(id)) {
        auto const& data = m_data[meta::ids[id]];
        Q_ASSERT(data.isBool());
        return data.toBool();
    }
    return def;
}

template<>
inline bool song::is<bool>(meta::type id) const
{
    return has(id) && m_data[meta::ids[id]].isBool();
}

template<>
inline bool song::is<QString>(meta::type id) const
{
    return has(id) && m_data[meta::ids[id]].isString();
}

template<>
inline bool song::is<int>(meta::type id) const
{
    return has(id) && m_data[meta::ids[id]].isDouble();
}

template<>
inline void song::set(meta::type id, QString const& v)
{
    // This _needs_ to be a qstringlist
    Q_ASSERT(id != meta::ARTIST);
    m_data[meta::ids[id]] = v;
}

template<>
inline void song::set(meta::type id, play_state const& v)
{
    m_data[meta::ids[id]] = (int)v;
}

template<>
inline void song::set(meta::type id, int const& v)
{
    m_data[meta::ids[id]] = v;
}

template<>
inline void song::set(meta::type id, bool const& v)
{
    m_data[meta::ids[id]] = v;
}

template<>
inline void song::set(meta::type id, QStringList const& v)
{
    QJsonArray a;
    for (auto const& l : v)
        a.append(l);
    m_data[meta::ids[id]] = a;
}
