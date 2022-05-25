/*************************************************************************
 * This file is part of tuna
 * github.com/univrsal/tuna
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

#include "song.hpp"
#include "../util/config.hpp"
#include "../util/format.hpp"
#include "music_source.hpp"
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>

song::song()
{
    clear();
}

void song::clear()
{
    m_data.fill(false);
    m_title = "";
    m_album = "";
    m_cover = "n/a";
    m_lyrics = "n/a";
    m_artists.clear();
    m_disc_number = 0;
    m_track_number = 0;
    m_duration_ms = 0;
    m_progress_ms = 0;
    m_playing_state = state_unknown;
    m_is_explicit = false;
    m_release_precision = prec_unknown;
    m_day = "";
    m_month = "";
    m_year = "";
    m_full_release = "";
    m_file_name = "";
}

bool song::has_cover_lookup_information() const
{
    return m_artists.size() > 0 && !m_album.isEmpty();
}

void song::update_release_precision()
{
    if (!m_day.isEmpty() && !m_month.isEmpty() && !m_year.isEmpty()) {
        m_release_precision = prec_day;
        m_full_release = m_year + "." + m_month + "." + m_day;
    } else if (!m_month.isEmpty() && !m_year.isEmpty()) {
        m_release_precision = prec_month;
        m_full_release = m_year + "." + m_month;
    } else if (!m_year.isEmpty()) {
        m_release_precision = prec_year;
        m_full_release = m_year;
    } else {
        m_release_precision = prec_unknown;
        m_full_release = "";
    }
}

void song::append_artist(const QString& a)
{
    if (!a.isEmpty())
        m_artists.append(a);
    m_data[meta::ARTIST] = m_artists.isEmpty();
}

void song::set_file_name(const QString& f)
{
    m_data[meta::FILE_NAME] = true;
    m_file_name = f;
}

void song::set_label(const QString& l)
{
    m_data[meta::LABEL] = !l.isEmpty();
    m_label = l;
}

void song::set_cover_link(const QString& link)
{
    m_data[meta::COVER] = !link.isEmpty();
    m_cover = link;
}

void song::set_title(const QString& title)
{
    m_data[meta::TITLE] = !title.isEmpty();
    m_title = util::remove_extensions(title);
}

void song::set_duration(int ms)
{
    m_data[meta::DURATION] = ms > 0;
    m_duration_ms = ms;
}

void song::set_progress(int ms)
{
    m_data[meta::PROGRESS] = ms > 0;
    m_progress_ms = ms;
}

void song::set_album(const QString& album)
{
    m_data[meta::ALBUM] = !album.isEmpty();
    m_album = album;
}

void song::set_explicit(bool e)
{
    m_data[meta::EXPLICIT] = true;
    m_is_explicit = e;
}

void song::set_state(play_state p)
{
    m_data[meta::STATUS] = true;
    m_playing_state = p;
}

void song::set_disc_number(int i)
{
    m_data[meta::DISC_NUMBER] = i > 0;
    m_disc_number = i;
}

void song::set_track_number(int i)
{
    m_data[meta::TRACK_NUMBER] = i > 0;
    m_track_number = i;
}

void song::set_year(const QString& y)
{
    m_data[meta::RELEASE] = true;
    m_year = y;
    update_release_precision();
}

void song::set_month(const QString& m)
{
    m_data[meta::RELEASE] = true;
    m_month = m;
    update_release_precision();
}

void song::set_day(const QString& d)
{
    m_data[meta::RELEASE] = true;
    m_day = d;
    update_release_precision();
}

bool song::operator==(const song& other) const
{
    /* basically compare all data that shouldn't change in between
     * updates, unless the song changes
     */
    /* clang-format off */
    return state() == other.state() && data() == other.data() && cover() == other.cover() && label() == other.label() &&
           disc_number() == other.disc_number() && track_number() == other.track_number() &&
           duration_ms() == other.duration_ms() && title() == other.title() && album() == other.album() &&
           year() == other.year() && label() == other.label() && m_full_release == other.m_full_release;
    /* clang-format on */
}

bool song::operator!=(const song& other) const
{
    return !((*this) == other);
}

void song::to_json(QJsonObject& obj) const
{
    obj = QJsonObject();
    obj["album"] = album();
    obj["disc_number"] = disc_number();
    obj["duration"] = duration_ms();
    obj["is_explicit"] = is_explicit();
    obj["label"] = label();
    obj["lyrics"] = lyrics();
    obj["progress"] = progress_ms();
    obj["time_left"] = duration_ms() - progress_ms();
    obj["title"] = title();
    obj["track_number"] = track_number();

    /* Special cases: Status, Cover link, Artists as list, release as year, month, day */
    QString status = "unknown";
    switch (m_playing_state) {
    case state_playing:
        status = "playing";
        break;
    case state_paused:
        status = "paused";
        break;
    case state_stopped:
        status = "stopped";
        break;
    default:
    case state_unknown:;
    }
    obj["status"] = status;

    if (m_data[meta::COVER]) {
        // Actual file:// url (can't be used in the browser source anymore)
        // also this can be the place holder when there's no cover
        // even if fetching from itunes is enabled so cover_url should be used
        obj["cover_path"] = m_cover;
        // Just points to the /cover.png end point
        obj["cover_url"] = QString("http://localhost:%1/cover.png").arg(QString::number(config::webserver_port));
    }

    if (has<meta::LYRICS>())
        obj["lyrics_url"] = m_lyrics;

    if (has<meta::ARTIST>()) {
        QJsonArray list;
        for (const auto& artist : m_artists) {
            list.append(artist);
        }
        obj["artists"] = list;
    }

    if (has<meta::RELEASE>()) {
        QJsonObject release;
        QString precision = "unknown";
        switch (m_release_precision) {
        case prec_day:
            release["day"] = m_day;
            precision = "day";
            /* Fallthrough */
        case prec_month:
            release["month"] = m_month;
            if (m_release_precision == prec_month)
                precision = "month";
            /* Fallthrough */
        default:
        case prec_unknown:
        case prec_year:
            release["full"] = m_full_release;
            release["year"] = m_year;
            if (m_release_precision == prec_year)
                precision = "year";
            else if (m_release_precision == prec_unknown)
                precision = "unkown";
        }
        release["date_precision"] = precision;
        obj["release_date"] = release;
    }
}

void song::from_json(const QJsonObject& obj)
{
    /* This is currently only used for POSTing info from the web browser
     * so we only parse supported options */
    clear();

    auto state = obj["status"];
    if (state.isNull())
        set_state(state_unknown);
    else if (state.toString() == "playing")
        set_state(state_playing);
    else if (state.toString() == "stopped")
        set_state(state_stopped);

    auto cover = obj["cover_url"];
    if (cover.isString())
        set_cover_link(cover.toString());

    auto title = obj["title"];
    if (title.isString())
        set_title(title.toString());

    auto artists = obj["artists"];
    if (artists.isArray()) {
        const auto arr = artists.toArray();
        for (auto const& a : arr) {
            if (a.isString())
                append_artist(a.toString());
        }
    }

    auto is_explicit = obj["is_explicit"];
    if (is_explicit.isBool())
        set_explicit(is_explicit.toBool());

    auto label = obj["label"];
    if (label.isString())
        set_label(label.toString());

    auto album = obj["album"];
    if (album.isString())
        set_album(album.toString());

    auto progress = obj["progress"];
    if (progress.isDouble())
        set_progress(progress.toInt());

    auto duration = obj["duration"];
    if (duration.isDouble())
        set_duration(duration.toInt());

    auto track = obj["track"];
    if (track.isDouble())
        set_duration(track.toInt());

    auto disc = obj["disc"];
    if (disc.isDouble())
        set_duration(disc.toInt());

    auto release = obj["release_date"];
    if (release.isObject()) {
        if (release["precision"].isString()) {
            auto prec = release["precision"].toString();
            if (prec == "year")
                m_release_precision = prec_year;
            else if (prec == "month")
                m_release_precision = prec_month;
            else if (prec == "day")
                m_release_precision = prec_day;
            else
                m_release_precision = prec_unknown;

            switch (m_release_precision) {
            case prec_day:
                if (release["day"].isString())
                    set_day(release["day"].toString());
                /* fallthrough */
            case prec_month:
                if (release["month"].isString())
                    set_day(release["month"].toString());
                /* fallthrough */
            case prec_year:
                if (release["year"].isString())
                    set_day(release["year"].toString());
                break;
            default:
            case prec_unknown:
                if (release["full"].isString())
                    m_full_release = release["full"].toString();
            }
        }
    }
}
