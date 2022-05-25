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
#include <QList>
#include <QString>
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
enum type : uint8_t {
    NONE,
    TITLE,
    ARTIST,
    ALBUM,
    RELEASE,
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
    TRACK_TOTAL,
    DIRECTOR,
    SEASON,
    EPISODE,
    SHOW_NAME,
    ACTORS,
    ALBUM_ARTIST,
    DISC_TOTAL,

    COUNT
};
}

class song {
    std::array<bool, meta::COUNT> m_data;
    QString m_title, m_album, m_cover, m_lyrics, m_label;
    QList<QString> m_artists;
    QString m_year, m_month, m_day, m_full_release, m_file_name;
    int32_t m_disc_number, m_track_number, m_duration_ms, m_progress_ms;
    bool m_is_explicit;
    play_state m_playing_state;
    date_precision m_release_precision;

public:
    song();
    void update_release_precision();
    void append_artist(const QString& a);
    void set_file_name(QString const&);
    void set_cover_link(const QString& link);
    void set_title(const QString& title);
    void set_duration(int ms);
    void set_progress(int ms);
    void set_album(const QString& album);
    void set_explicit(bool e);
    void set_state(play_state p);
    void set_disc_number(int i);
    void set_track_number(int i);
    void set_year(const QString& y);
    void set_month(const QString& m);
    void set_day(const QString& d);
    void set_label(const QString& l);
    void clear();

    play_state state() const { return m_playing_state; }
    bool is_playing() const { return m_playing_state == state_playing; }
    bool is_paused() const { return !is_playing(); }
    bool is_explicit() const { return m_is_explicit; }

    bool has_cover_lookup_information() const;
    const QString& file_name() const { return m_file_name; }
    const QString& album() const { return m_album; }
    const QString& cover() const { return m_cover; }
    const QString& lyrics() const { return m_lyrics; }
    const QString& year() const { return m_year; }
    const QString& month() const { return m_month; }
    const QString& day() const { return m_day; }
    const QString& title() const { return m_title; }
    const QString& label() const { return m_label; }
    const QList<QString>& artists() const { return m_artists; }
    int32_t duration_ms() const { return m_duration_ms; }
    int32_t progress_ms() const { return m_progress_ms; }
    int32_t track_number() const { return m_track_number; }
    int32_t disc_number() const { return m_disc_number; }

    std::array<bool, meta::COUNT> const& data() const { return m_data; }
    date_precision release_precision() const { return m_release_precision; }

    bool operator==(const song& other) const;
    bool operator!=(const song& other) const;

    void to_json(QJsonObject& obj) const;
    void from_json(const QJsonObject& obj);

    template<uint8_t T>
    bool has() const
    {
        return m_data[T];
    }
};
