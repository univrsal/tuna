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
#include <QString>
#include <QVector>
#include <stdint.h>

enum date_precision {
    prec_day,
    prec_month,
    prec_year,
    prec_unkown
};

class song {
    uint16_t m_data;
    QString m_title, m_album, m_cover, m_lyrics;
    QVector<QString> m_artists;
    QString m_year, m_month, m_day;
    int32_t m_disc_number, m_track_number, m_duration_ms, m_progress_ms;
    bool m_is_explicit, m_is_playing;
    date_precision m_release_precision;

public:
    song();
    void update_release_precision();
    void append_artist(const QString& a);
    void set_cover_link(const QString& link);
    void set_title(const QString& title);
    void set_duration(int ms);
    void set_progress(int ms);
    void set_album(const QString& album);
    void set_explicit(bool e);
    void set_playing(bool p);
    void set_disc_number(int i);
    void set_track_number(int i);
    void set_year(const QString& y);
    void set_month(const QString& m);
    void set_day(const QString& d);
    void clear();
};
