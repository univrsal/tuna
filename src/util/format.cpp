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

#include "format.hpp"
#include "../query/music_source.hpp"
#include "../query/song.hpp"
#include "../util/config.hpp"
#include "../util/tuna_thread.hpp"

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
    /* Register format specifiers with their data */
    specifiers.emplace_back(new specifier("title", CAP_TITLE, [](song const& s) -> QString {
        return s.title();
    }));
    specifiers.emplace_back(new specifier("album", CAP_ALBUM, [](song const& s) -> QString {
        return s.album();
    }));
    specifiers.emplace_back(new specifier("release_date", CAP_RELEASE, [](song const& s) -> QString {
        if (s.release_precision() == prec_day)
            return QString("%1.%2.%3").arg(s.year(), s.month(), s.day());
        if (s.release_precision() == prec_month)
            return QString("%1.%2").arg(s.year(), s.month());
        if (s.release_precision() == prec_year)
            return s.year();
        return "";
    }));
    specifiers.emplace_back(new specifier("release_year", CAP_RELEASE, [](song const& s) -> QString {
        return s.year();
    }));
    specifiers.emplace_back(new specifier("release_month", CAP_RELEASE, [](song const& s) -> QString {
        return s.month();
    }));
    specifiers.emplace_back(new specifier("release_day", CAP_RELEASE, [](song const& s) -> QString {
        return s.day();
    }));
    specifiers.emplace_back(new specifier("label", CAP_LABEL, [](song const& s) -> QString {
        return s.label();
    }));
    specifiers.emplace_back(new specifier("file_name", CAP_FILE_NAME, [](song const& s) -> QString {
        return s.file_name();
    }));
    specifiers.emplace_back(new specifier("first_artist", CAP_ARTIST, [](song const& s) -> QString {
        return s.artists().count() > 0 ? s.artists()[0] : "";
    }));
    specifiers.emplace_back(new specifier("artists", CAP_ARTIST, [](song const& s) -> QString {
        return s.artists().count() > 0 ? s.artists().join(", ") : "";
    }));
    specifiers.emplace_back(new specifier("track_number", CAP_TRACK_NUMBER, [](song const& s) -> QString {
        return QString::number(s.track_number());
    }));
    specifiers.emplace_back(new specifier("disc_number", CAP_DISC_NUMBER, [](song const& s) -> QString {
        return QString::number(s.disc_number());
    }));
    specifiers.emplace_back(new specifier("progress", CAP_PROGRESS, [](song const& s) -> QString {
        return time_format(s.progress_ms());
    }));
    specifiers.emplace_back(new specifier("duration", CAP_DURATION, [](song const& s) -> QString {
        return time_format(s.duration_ms());
    }));
    specifiers.emplace_back(new specifier("time_left", CAP_TIME_LEFT, [](song const& s) -> QString {
        return time_format(s.duration_ms() - s.progress_ms());
    }));
    specifiers.emplace_back(new static_specifier("line_break", [](song const& s) -> QString {
        return "\n";
    }));
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
                if (!(src_ref->get_capabilities() & spec->get_required_caps()))
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
