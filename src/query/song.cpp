/*************************************************************************
 * This file is part of tuna
 * git.vrsal.xyz/alex/tuna
 * Copyright 2023 univrsal <uni@vrsal.xyz>.
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
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QLocale>

song::song()
{
    clear();
}

void song::clear()
{
    m_data = QJsonObject();
    set(meta::COVER, QString("n/a"));
    set(meta::LYRICS, QString("n/a"));
    set(meta::STATUS, state_unknown);
    m_release_precision = prec_unknown;
}

bool song::has_cover_lookup_information() const
{
    auto has_meta = has(meta::ARTIST) && has(meta::ALBUM);
    auto artists = get<QStringList>(meta::ARTIST);
    auto album = get(meta::ALBUM);
    return has_meta && !artists.isEmpty() && !album.isEmpty();
}

void song::update_release_precision()
{
    auto day = has(meta::RELEASE_DAY);
    auto month = has(meta::RELEASE_MONTH);
    auto year = has(meta::RELEASE_YEAR);

    if (day && month && year) {
        m_release_precision = prec_day;
        QDate d;
        d.setDate(get<int>(meta::RELEASE_YEAR), get<int>(meta::RELEASE_MONTH), get<int>(meta::RELEASE_DAY));
        set(meta::RELEASE, QLocale::system().toString(d, QLocale::ShortFormat));
    } else if (month && year) {
        m_release_precision = prec_month;
        set(meta::RELEASE, QString::number(get<int>(meta::RELEASE_YEAR)) + "." + QString::number(get<int>(meta::RELEASE_MONTH)));
    } else if (year) {
        m_release_precision = prec_year;
        set(meta::RELEASE, QString::number(get<int>(meta::RELEASE_YEAR)));
    } else {
        m_release_precision = prec_unknown;
        reset<meta::RELEASE>();
    }
}

bool song::operator==(const song& other) const
{
    /* basically compare all data that shouldn't change in between
     * updates, unless the song changes
     */
    /* clang-format off */
    return get<int>(meta::STATUS) == other.get<int>(meta::STATUS) &&
            get(meta::COVER) == other.get(meta::COVER) && get(meta::LABEL) == other.get(meta::LABEL) &&
           get<int>(meta::DISC_NUMBER) == other.get<int>(meta::DISC_NUMBER) && get<int>(meta::TRACK_NUMBER) == other.get<int>(meta::TRACK_NUMBER) &&
           get<int>(meta::DURATION) == other.get<int>(meta::DURATION) && get(meta::TITLE) == other.get(meta::TITLE) && get(meta::ALBUM) == get(meta::ALBUM) &&
           get(meta::RELEASE) == other.get(meta::RELEASE);
    /* clang-format on */
}

bool song::operator!=(const song& other) const
{
    return !((*this) == other);
}

void song::to_json(QJsonObject& obj) const
{
    obj = m_data;

    /* Special cases: Status, Cover link, Artists as list, release as year, month, day */
    QString status = "unknown";
    switch (get<int>(meta::STATUS)) {
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

    if (config::remove_file_extensions)
        obj["title"] = util::remove_extensions(obj["title"].toString());

    if (has(meta::COVER)) {
        // Just points to the /cover.png end point
        obj["cover_url"] = QString("http://localhost:%1/cover.png").arg(QString::number(config::webserver_port));
    }

    // Technically deprecated, because the json object
    // now contains release_day etc., but for
    // backwards compatibility we'll keep this
    if (has(meta::RELEASE)) {
        QJsonObject release;
        QString precision = "unknown";
        switch (m_release_precision) {
        case prec_day:
            release["day"] = get<int>(meta::RELEASE_DAY);
            precision = "day";
            /* Fallthrough */
        case prec_month:
            release["month"] = get<int>(meta::RELEASE_MONTH);
            if (m_release_precision == prec_month)
                precision = "month";
            /* Fallthrough */
        default:
        case prec_unknown:
        case prec_year:
            release["full"] = get(meta::RELEASE);
            release["year"] = get<int>(meta::RELEASE_YEAR);
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
    m_data = obj;

    // TODO: Use only one of the three cover_path/cover_url/cover
    // currently sources use cover_path, the web browser widget uses cover_url
    // and the user script uses cover
    if (obj["cover"].isString())
        set(meta::COVER, obj["cover"].toString());
    else
        set(meta::COVER, obj["cover_url"].toString());

    auto status = play_state::state_unknown;
    if (obj["status"].toString() == "playing")
        status = play_state::state_playing;
    else if (obj["status"].toString() == "stopped")
        status = play_state::state_stopped;
    else if (obj["status"].toString() == "paused")
        status = play_state::state_stopped;
    set(meta::STATUS, status);

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
                if (release["day"].isDouble())
                    set(meta::RELEASE_DAY, release["day"].toInt());
                /* fallthrough */
            case prec_month:
                if (release["month"].isDouble())
                    set(meta::RELEASE_MONTH, release["month"].toInt());
                /* fallthrough */
            case prec_year:
                if (release["year"].isDouble())
                    set(meta::RELEASE_YEAR, release["year"].toInt());
                break;
            default:
            case prec_unknown:
                if (release["full"].isString())
                    set(meta::RELEASE, release["full"].toString());
            }
        }
    }
}
