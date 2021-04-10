/*************************************************************************
 * This file is part of tuna
 * github.com/univrsal/tuna
 * Copyright 2020 univrsal <uni@vrsal.cf>.
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

const specifier *get_matching_specifier(char c)
{
	for (const auto &s : specifiers) {
		if (s->get_id() == c)
			return s.get();
	}
	return nullptr;
}

/* Find the number in between '[]' in
 * a string like t[123] abc
 * and remove the '[123]' part
 */
int get_truncate_arg(QString &str)
{
	int number = 0;
	QString tmp = "", copy = str;
	bool ok = false;

	if (str.length() < 4 || str[1] != '[')
		return number;
	copy.remove(1, 1); /* remove '[' */

	bool flag = true;
	while (!copy.isEmpty() && flag) {
		if (copy[1].isNumber())
			tmp.append(copy[1]);
		else if (copy[1] == ']')
			flag = false; /* We're done */
		else
			break;         /* Unknown character -> stop */
		copy.remove(1, 1); /* consume character */
	}

	number = tmp.toInt(&ok);

	if (ok)
		str = copy;
	else
		number = 0;

	return number;
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
	specifiers.emplace_back(new specifier_string('t', CAP_TITLE));
	specifiers.emplace_back(new specifier_string('a', CAP_ALBUM));
	specifiers.emplace_back(new specifier_string('y', CAP_RELEASE));
	specifiers.emplace_back(new specifier_string('b', CAP_LABEL));
	specifiers.emplace_back(new specifier_string_list('m', CAP_ARTIST));
	specifiers.emplace_back(new specifier_date('r', CAP_RELEASE));
	specifiers.emplace_back(new specifier_int('d', CAP_DISC_NUMBER));
	specifiers.emplace_back(new specifier_int('n', CAP_TRACK_NUMBER));
	specifiers.emplace_back(new specifier_time('p', CAP_PROGRESS));
	specifiers.emplace_back(new specifier_time('l', CAP_DURATION));
	specifiers.emplace_back(new specifier_time('o', CAP_TIME_LEFT));
	specifiers.emplace_back(new specifier_static('e', "\n"));
	specifiers.emplace_back(new specifier_static('s', " "));
}

void execute(QString &q)
{
	auto splits = q.split("%");
	auto src_ref = music_sources::selected_source_unsafe();
	bool first = !q.startsWith("%");
	for (auto &split : splits) {
		if (first) {
			first = false;
			continue;
		}

		if (split.isEmpty())
			continue;

		auto sp = get_matching_specifier(split[0].toLower().toLatin1());
		if (sp) {
			sp->do_format(split, src_ref->song_info());
		}
	}
	q = splits.join("");
}

bool specifier::replace(QString &slice, const song &s, const QString &data) const
{
	if (!(s.data() & m_tag_id))
		return false; /* We do not have the information needed for this specifier */

	/* get truncation, if specified */
	int max_length = get_truncate_arg(slice);
	QString copy = data;
	if (slice[0].isUpper())
		copy = copy.toUpper();
	if (max_length > 0 && copy.length() > max_length) {
		copy.truncate(max_length);
		copy.append("...");
	}
	slice = slice.remove(0, 1);
	slice.prepend(copy);

	return true;
}

bool specifier::do_format(QString &slice, const song &s) const
{
	return replace(slice, s);
}

bool specifier_string::do_format(QString &slice, const song &s) const
{
	return replace(slice, s, s.get_string_value(m_id));
}

bool specifier_static::do_format(QString &slice, const song &s) const
{
	UNUSED_PARAMETER(s);
	slice = slice.remove(0, 1);
	slice.prepend(m_static_value);
	return true;
}

bool specifier_time::do_format(QString &slice, const song &s) const
{
	QString value = time_format(s.get_int_value(m_id));
	return replace(slice, s, value);
}

bool specifier_int::do_format(QString &slice, const song &s) const
{
	QString value = QString::number(s.get_int_value(m_id));
	return replace(slice, s, value);
}

bool specifier_string_list::do_format(QString &slice, const song &s) const
{
	QString concatenated_list;
	concatenated_list = s.artists().join(", ");

	if (concatenated_list.isEmpty())
		concatenated_list = "n/a";
	return replace(slice, s, concatenated_list);
}

bool specifier_date::do_format(QString &slice, const song &s) const
{
	QString data;
	if (s.release_precision() == prec_day) {
		data.append(s.year()).append(".").append(s.month()).append(".").append(s.day());
	} else if (s.release_precision() == prec_month) {
		data.append(s.year()).append(".").append(s.month()).append(".");
	} else {
		data.append(s.year());
	}
	return replace(slice, s, data);
}

const std::vector<std::unique_ptr<specifier>> &get_specifiers()
{
	return specifiers;
}

}
