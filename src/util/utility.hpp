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

#include <QRect>
#include <QString>
#include <obs-module.h>
#include <stdint.h>

#define utf8_to_qt(_str) QString::fromUtf8(_str)
#define qt_to_utf8(_str) _str.toUtf8().constData()

#define write_log(log_level, format, ...) blog(log_level, "[tuna] " format, ##__VA_ARGS__)

#define bdebug(format, ...) write_log(LOG_DEBUG, format, ##__VA_ARGS__)
#define binfo(format, ...) write_log(LOG_INFO, format, ##__VA_ARGS__)
#define bwarn(format, ...) write_log(LOG_WARNING, format, ##__VA_ARGS__)
#define berr(format, ...) write_log(LOG_ERROR, format, ##__VA_ARGS__)

#define SECOND_TO_NS 1000000000

#define UTIL_MAX(a, b) ((a) > (b) ? (a) : (b))

class song;

class QJsonDocument;

namespace util {

extern bool have_vlc_source;

extern bool curl_download(const char* url, const char* path);

QJsonDocument curl_get_json(const char* url);

extern bool download_cover(const QString& url);

extern void reset_cover();

extern void download_lyrics(const song& song);

extern void handle_outputs(const song& song);

extern int64_t epoch();

extern bool window_pos_valid(QRect rect);

extern size_t write_callback(char* ptr, size_t size, size_t nmemb, std::string* str);

/* Redirected from util/threading.h because it clashes with mongoose */
extern void set_thread_name(const char* name);

extern QString remove_extensions(QString const& str);

extern QString file_from_path(QString const& file);

extern bool open_config(const char* name, QJsonDocument&);
extern bool save_config(const char* name, const QJsonDocument&);

} // namespace util
