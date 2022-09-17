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

#include "format.hpp"
#include "../query/music_source.hpp"
#include "../query/song.hpp"
#include "../util/config.hpp"
#include "../util/tuna_thread.hpp"
#include <QJsonDocument>
#include <QLocale>

namespace format {

std::vector<std::unique_ptr<specifier>> specifiers;

const specifier* get_specifier_by_id(QString const& id, bool& upper)
{
    for (const auto& s : specifiers) {
        if (s->get_id() == id.toLower()) {
            upper = id == s->get_id().toUpper();
            return s.get();
        }
    }
    return nullptr;
}

QString time_format(int32_t ms)
{
    int secs = (ms / 1000) % 60;
    int minute = (ms / 1000) / 60 % 60;
    int hour = (ms / 1000) / 60 / 60 % 60;
    QTime t(hour, minute, secs);

    return t.toString(hour > 0 ? "h:mm:ss" : "m:ss");
}

void init()
{
#define int_specifier(name, meta)                                                    \
    specifiers.emplace_back(new specifier(name, meta, [](song const& s) -> QString { \
        return QString::number(s.get<int>(meta));                                    \
    }));

    /* Register format specifiers with their data */
    specifiers.emplace_back(new specifier("title", meta::TITLE, [](song const& s) -> QString {
        auto title = s.get(meta::TITLE);
        if (config::remove_file_extensions)
            title = util::remove_extensions(title);
        return title;
    }));
    specifiers.emplace_back(new specifier("album", meta::ALBUM));
    specifiers.emplace_back(new specifier("label", meta::LABEL));
    specifiers.emplace_back(new specifier("file_name", meta::FILE_NAME));
    specifiers.emplace_back(new specifier("url", meta::URL));

    int_specifier("release_day", meta::RELEASE_DAY);
    int_specifier("release_month", meta::RELEASE_MONTH);
    int_specifier("release_year", meta::RELEASE_YEAR);
    int_specifier("track_number", meta::TRACK_NUMBER);
    int_specifier("disc_number", meta::DISC_NUMBER);

    specifiers.emplace_back(new specifier("progress", meta::PROGRESS, [](song const& s) -> QString {
        return time_format(s.get<int>(meta::PROGRESS));
    }));
    specifiers.emplace_back(new specifier("duration", meta::DURATION, [](song const& s) -> QString {
        return time_format(s.get<int>(meta::DURATION));
    }));
    specifiers.emplace_back(new specifier("time_left", { meta::PROGRESS, meta::DURATION }, [](song const& s) -> QString {
        return time_format(s.get<int>(meta::DURATION) - s.get<int>(meta::PROGRESS));
    }));

    specifiers.emplace_back(new specifier("release_date", meta::RELEASE, [](song const& s) -> QString {
        auto day = s.has(meta::RELEASE_DAY);
        auto month = s.has(meta::RELEASE_MONTH);
        auto year = s.has(meta::RELEASE_YEAR);

        if (day && month && year) {
            QDate d;
            d.setDate(s.get<int>(meta::RELEASE_YEAR), s.get<int>(meta::RELEASE_MONTH), s.get<int>(meta::RELEASE_DAY));
            return QLocale::system().toString(d, QLocale::ShortFormat);
        } else if (month && year) {
            return QString("%1.%2").arg(s.get<int>(meta::RELEASE_YEAR), s.get<int>(meta::RELEASE_MONTH));
        } else if (year) {
            return QString::number(s.get<int>(meta::RELEASE_YEAR));
        }
        return "";
    }));

    specifiers.emplace_back(new specifier("first_artist", meta::ARTIST, [](song const& s) -> QString {
        auto l = s.get<QStringList>(meta::ARTIST);
        if (!l.isEmpty())
            return l[0];
        return "";
    }));
    specifiers.emplace_back(new specifier("artists", meta::ARTIST, [](song const& s) -> QString {
        return s.get<QStringList>(meta::ARTIST).join(", ");
    }));
    specifiers.emplace_back(new static_specifier("line_break", [](song const&) -> QString {
        return "\n";
    }));
    specifiers.emplace_back(new static_specifier("json_compact", [](song const& s) -> QString {
        QJsonObject obj;
        s.to_json(obj);
        QJsonDocument doc(obj);
        return QString(doc.toJson(QJsonDocument::Compact));
    }));
    specifiers.emplace_back(new static_specifier("json_formatted", [](song const& s) -> QString {
        QJsonObject obj;
        s.to_json(obj);
        QJsonDocument doc(obj);
        return QString(doc.toJson(QJsonDocument::Indented));
    }));

    // VLC Stuff
    specifiers.emplace_back(new specifier("genre", meta::GENRE));
    specifiers.emplace_back(new specifier("copyright", meta::COPYRIGHT));
    specifiers.emplace_back(new specifier("description", meta::DESCRIPTION));
    specifiers.emplace_back(new specifier("rating", meta::RATING));
    specifiers.emplace_back(new specifier("setting", meta::SETTING));
    specifiers.emplace_back(new specifier("language", meta::LANGUAGE));
    specifiers.emplace_back(new specifier("now_playing", meta::NOW_PLAYING));
    specifiers.emplace_back(new specifier("encoded_by", meta::ENCODED_BY));
    specifiers.emplace_back(new specifier("track_id", meta::TRACK_ID));
    specifiers.emplace_back(new specifier("director", meta::DIRECTOR));
    specifiers.emplace_back(new specifier("season", meta::SEASON));
    specifiers.emplace_back(new specifier("episode", meta::EPISODE));
    specifiers.emplace_back(new specifier("show_name", meta::SHOW_NAME));
    specifiers.emplace_back(new specifier("actors", meta::ACTORS));
    specifiers.emplace_back(new specifier("album_artist", meta::ALBUM_ARTIST));
    int_specifier("disc_total", meta::DISC_TOTAL);
    int_specifier("track_total", meta::TRACK_TOTAL);

    std::sort(specifiers.begin(), specifiers.end(), [](auto const& a, auto const& b) {
        return a->get_id()[0] < b->get_id()[1];
    });
}

bool execute(QString& q)
{
    auto src_ref = music_sources::selected_source_unsafe();
    auto copy = q;
    auto result = true;
    q = "";

    auto handle_specifier = [&copy](QString::Iterator& it, int& truncate, bool& uppercase, bool& proper_formatting) -> specifier const* {
        QString id = "";

        while (it != copy.end() && *it != '}' && *it != ':') {
            id += *it;
            ++it;
        }

        if (*it == ':') {
            ++it;
            QString tr = "";
            while (it != copy.end() && *it != '}') {
                tr += *it;
                ++it;
            }
            truncate = tr.toInt();
        }

        if (*it == '}') {
            proper_formatting = true;
            return get_specifier_by_id(id, uppercase);
        }
        return nullptr;
    };

    for (auto it = copy.begin();; ++it) {
        if (*it == '\\') {
            ++it;
            if (it == copy.end())
                break;
            q += *it;
            continue;
        }

        if (*it == '{') {
            ++it;
            if (it == copy.end())
                return result;
            int truncate = 0;
            bool uppercase = false;
            bool formatting = false;
            if (auto* spec = handle_specifier(it, truncate, uppercase, formatting)) {
                auto data = spec->get_data(src_ref->song_info());
                if (!src_ref->provides_metadata(spec->get_required_caps()))
                    result = false;
                if (truncate > 0 && data.length() > truncate) {
                    data.truncate(truncate);
                    data.append("...");
                }
                if (uppercase)
                    data = data.toUpper();
                q += data;
            } else if (formatting) {
                // We only tell the user that the selected formatting specifier
                // isn't supported if the formatting is correct eg. {test}
                // but not with {test
                result = false;
            }
        } else {
            if (it != copy.end())
                q += *it;
        }
        if (it == copy.end())
            return result;
    }
    return result;
}

const std::vector<std::unique_ptr<specifier>>& get_specifiers()
{
    return specifiers;
}
}
