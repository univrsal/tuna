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

#include "format.hpp"
#include "../query/music_source.hpp"
#include "../query/song.hpp"
#include "../util/config.hpp"
#include <memory>

namespace format {

std::vector<specifier> specifiers;

const specifier* get_matching_specifier(char c)
{
    for (const auto& s : specifiers) {
        if (s.get_id() == c)
            return &s;
    }
    return nullptr;
}

/* Find the number in between '[]' in
 * a string like t[123] abc
 * and remove the '[123] part
 */
int get_truncate_arg(QString& str)
{
    int number = 0;
    if (str[1] != '[')
        return 0;
    while (str[number] != ']') {
        number++;
        if (number >= str.length())
            return 0;
    }

    /* Cut the string down */
    QStringRef r(&str, 1, number);
    bool ok = false;
    number = r.toInt(&ok);

    if (ok) {
        /* cut [123] */
        str = str[0] + str.right(number);
    } else {
        number = 0;
    }
    return number;
}

QString time_format(uint32_t ms)
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
    auto s = config::selected_source->song_info();
    specifiers.emplace_back(specifier_string('t', CAP_TITLE, s->title()));
    specifiers.emplace_back(specifier_string('a', CAP_ALBUM, s->album()));
    specifiers.emplace_back(specifier_string('y', 1, s->year()));
    specifiers.emplace_back(specifier_string_list('m', CAP_ARTIST, s->artists()));
    specifiers.emplace_back(specifier_date('r', CAP_RELEASE));
    specifiers.emplace_back(specifier_int('d', CAP_DISC_NUMBER, s->disc()));
    specifiers.emplace_back(specifier_int('n', CAP_TRACK_NUMBER, s->track()));
    specifiers.emplace_back(specifier_int('p', CAP_PROGRESS, s->progress()));
    specifiers.emplace_back(specifier_int('l', CAP_DURATION, s->duration()));
}

void execute(QString& q)
{
    auto splits = q.split("%");

    for (auto& split : splits) {
        auto sp = get_matching_specifier(split[0].toLower().toLatin1());
        if (sp)
            sp->do_format(split, config::selected_source->song_info());
    }
}

bool specifier::replace(QString& slice, const song* s, const QString& data) const
{
    if (!(s->data() & m_tag_id))
        return false; /* We do not have the information needed for this specifier */

    /* get truncation, if specified */
    int max_length = get_truncate_arg(slice);
    slice.replace(m_id, data);
    if (slice.length() > max_length) {
        slice.truncate(max_length);
        slice.append("...");
    }
    return true;
}

bool specifier_int::do_format(QString& slice, const song* s) const
{
    return replace(slice, s, QString::number(*m_data));
}

bool specifier_string::do_format(QString& slice, const song* s) const
{
    return replace(slice, s, *m_data);
}

bool specifier_string_list::do_format(QString& slice, const song* s) const
{
    QString concatenated_list;
    for (auto str : *m_data) {
        concatenated_list += str + ", ";
    }
    concatenated_list.truncate(2);
    return replace(slice, s, concatenated_list);
}

bool specifier_date::do_format(QString& slice, const song* s) const
{
    QString data;
    if (s->release_precision() == prec_day) {
        data.append(s->year()).append(".").append(s->month()).append(".").append(s->day());
    } else if (s->release_precision() == prec_month) {
        data.append(s->year()).append(".").append(s->month()).append(".");
    } else {
        data.append(s->year());
    }
    return replace(slice, s, data);
}

}
