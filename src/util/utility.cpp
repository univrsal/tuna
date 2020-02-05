/*************************************************************************
 * This file is part of tuna
 * github.con/univrsal/tuna
 * Copyright 2019 univrsal <universailp@web.de>.
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

#include "utility.hpp"
#include "../query/music_source.hpp"
#include "config.hpp"
#include "vlc_internal.h"
#include "constants.hpp"
#include <QFile>
#include <QTextStream>
#include <ctime>
#include <curl/curl.h>
#include <fstream>
#include <obs-module.h>
#include <stdio.h>
#include <util/platform.h>
#include <QMessageBox>

namespace util {

bool vlc_loaded = true;

void load_vlc()
{
    auto ver = obs_get_version();
    if (obs_get_version() != LIBOBS_API_VER) {
        int major = ver >> 24 & 0xFF;
        int minor = ver >> 16 & 0xFF;
        int patch = ver & 0xFF;
        bwarn("libobs version %d.%d.%d is "
             "invalid. Tuna expects %d.%d.%d for"
             " VLC sources to work",
            major, minor, patch, LIBOBS_API_MAJOR_VER,
            LIBOBS_API_MINOR_VER, LIBOBS_API_PATCH_VER);
        bool result = QMessageBox::question(nullptr, T_ERROR_TITLE, T_VLC_VERSION_ISSUE)
                     == QMessageBox::StandardButton::Yes;
        if (result) {
            bwarn("User force enabled VLC support");
        }
    }

    if (vlc_loaded) {
        if (!load_libvlc_module() || !load_vlc_funcs() || !load_libvlc()) {
            bwarn("Couldn't load libVLC,"
                              " VLC source support disabled");
            vlc_loaded = false;
        }
    }
}

void unload_vlc()
{
    unload_libvlc();
}

size_t
write_data(void* ptr, size_t size, size_t nmemb, FILE* stream)
{
    size_t written;
    written = fwrite(ptr, size, nmemb, stream);
    return written;
}

bool curl_download(const char* url, const char* path)
{
    CURL* curl = curl_easy_init();
    FILE* fp = nullptr;
#ifdef _WIN32
    wchar_t* wstr = NULL;
    os_utf8_to_wcs_ptr(path, strlen(path), &wstr);
    fp = _wfopen(wstr, L"wb");
    bfree(wstr);
#else
    fp = fopen(path, "wb");
#endif

    bool result = false;
    if (fp && curl) {
        curl_easy_setopt(curl, CURLOPT_URL, url);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);
#ifdef DEBUG
        curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
#endif
        CURLcode res = curl_easy_perform(curl);

        if (res != CURLE_OK)
            berr("Couldn't fetch file from %s to %s", url, path);
        else
            bdebug("Fetched %s to %s", url, path);
        fclose(fp);
        result = true;
    }

    if (curl)
        curl_easy_cleanup(curl);
    return true;
}

bool move_file(const char* src, const char* dest)
{
#ifdef _WIN32
    wchar_t* in_path = nullptr;
    wchar_t* out_path = nullptr;
    os_utf8_to_wcs_ptr(dest, strlen(dest), &out_path);
    os_utf8_to_wcs_ptr(src, strlen(src), &in_path);

    std::wifstream in(in_path, std::ios::binary);
    std::wofstream out(out_path, std::ios::binary);
    bfree(in_path);
    bfree(out_path);
#else
    std::ifstream in(src, std::ios::binary);
    std::ofstream out(dest, std::ios::binary);
#endif

    bool result = false;
    if (in.good() && out.good()) {
        out << in.rdbuf();
        result = true;
    }

    in.close();
    out.close();
    return result;
}

void handle_lyrics(const song* song)
{
    static std::string last_lyrics = "";

    if (song->data & CAP_LYRICS && last_lyrics != song->lyrics) {
        last_lyrics = song->lyrics;
        if (!curl_download(song->lyrics.c_str(), config::lyrics_path)) {
            berr("Couldn't dowload lyrics from '%s' to '%s'",
                  song->lyrics.c_str(), config::lyrics_path);
        }
    }
}

void handle_cover_art(const song* song)
{
    static std::string last_cover = "";
    bool is_url = song->cover.find("http") != std::string::npos || song->cover.find("file://") != std::string::npos;

    bool found_cover = song->data & CAP_COVER && song->is_playing;

    if (found_cover && !song->cover.empty() && song->cover != last_cover) {
        if (is_url && config::download_cover) {
            std::string tmp = config::cover_path;
            tmp += ".tmp";
            last_cover = song->cover;
            found_cover = curl_download(song->cover.c_str(), tmp.c_str());
            /* Replace cover only after download is done */
            if (found_cover) {
                move_file(tmp.c_str(), config::cover_path);
            }
        } else if (!is_url) {
            last_cover = song->cover;
            found_cover = move_file(song->cover.c_str(), config::cover_path);
        }
    }

    if (!found_cover && last_cover != "n/a") {
        last_cover = "n/a";
        /* no cover => use place placeholder */
        if (!move_file(config::cover_placeholder, config::cover_path))
            berr("Couldn't move placeholder cover");
    }
}

QString time_format(uint32_t ms)
{
    int secs = (ms / 1000) % 60;
    int minute = (ms / 1000) / 60 % 60;
    int hour = (ms / 1000) / 60 / 60 % 60;
    QTime t(hour, minute, secs);

    return t.toString(hour > 0 ? "h:mm:ss" : "m:ss");
}

void format_string(QString& out, const song* song)
{
    out.replace("%t", song->title.c_str());
    out.replace("%T", QString::fromStdString(song->title).toUpper());
    out.replace("%m", song->artists.c_str());
    out.replace("%M", QString::fromStdString(song->artists).toUpper());
    out.replace("%a", song->album.c_str());
    out.replace("%A", QString::fromStdString(song->album).toUpper());
    out.replace("%d", QString::number(song->disc_number));
    out.replace("%n", QString::number(song->track_number));
    out.replace("%p", time_format(song->progress_ms));
    out.replace("%l", time_format(song->duration_ms));
    out.replace("%b", "\n");
    QString full_date = "";

    switch (song->release_precision) {
    case prec_day:
        full_date.append(song->day.c_str()).append('.');
    case prec_month:
        full_date.append(song->month.c_str()).append('.');
    case prec_year:
        full_date.append(song->year.c_str());
    default:;
    }
    out.replace("%y", song->year.c_str());
    out.replace("%r", full_date);
}

void write_song(const QString& str, const QString& output)
{
    QFile out(output);
    if (out.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream stream(&out);
        stream.setCodec("UTF-8");
        stream << str;
        stream.flush();
        out.close();
    } else {
        berr("Couldn't open song output file %s", output.toStdString().c_str());
    }
}

void handle_outputs(const song* s)
{
    static QString tmp_text = "";

    for (const auto& output : config::outputs) {
        tmp_text.clear();
        tmp_text = output.first;
        format_string(tmp_text, s);

        if (tmp_text.length() < 1 || !s->is_playing)
            tmp_text = config::placeholder;
        write_song(tmp_text, output.second);
    }
}

void replace_all(std::string& str,
    const std::string& find,
    const std::string& replace)
{
    if (find.empty())
        return;
    size_t start_pos = 0;
    while ((start_pos = str.find(find, start_pos)) != std::string::npos) {
        str.replace(start_pos, find.length(), replace);
        start_pos += replace.length();
    }
}

int64_t
epoch()
{
    return time(nullptr);
}
} // namespace util
