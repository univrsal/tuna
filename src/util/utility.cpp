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

#include "utility.hpp"
#include "../query/music_source.hpp"
#include "config.hpp"
#include "constants.hpp"
#include "format.hpp"
#include "vlc_internal.h"
#include <QFile>
#include <QMessageBox>
#include <QTextStream>
#include <ctime>
#include <curl/curl.h>
#include <fstream>
#include <obs-module.h>
#include <stdio.h>
#include <util/platform.h>

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

bool curl_download(const QString& url, const QString& path)
{
    CURL* curl = curl_easy_init();
    FILE* fp = nullptr;
#ifdef _WIN32
    wchar_t* wstr = NULL;
    os_utf8_to_wcs_ptr(path, strlen(path), &wstr);
    fp = _wfopen(wstr, L"wb");
    bfree(wstr);
#else
    fp = fopen(qcstr(path), "wb");
#endif

    bool result = false;
    if (fp && curl) {
        curl_easy_setopt(curl, CURLOPT_URL, qcstr(url));
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);
#ifdef DEBUG
        curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
#endif
        CURLcode res = curl_easy_perform(curl);

        if (res != CURLE_OK)
            berr("Couldn't fetch file from %s to %s", qcstr(url), qcstr(path));
        else
            bdebug("Fetched %s to %s", qcstr(url), qcstr(path));
        fclose(fp);
        result = true;
    }

    if (curl)
        curl_easy_cleanup(curl);
    return true;
}

bool move_file(const QString& src, const QString& dest)
{
#ifdef _WIN32
    wchar_t* in_path = nullptr;
    wchar_t* out_path = nullptr;
    os_utf8_to_wcs_ptr(qcstr(dest), dest.length(), &out_path);
    os_utf8_to_wcs_ptr(qcstr(src), src.length(), &in_path);

    std::wifstream in(in_path, std::ios::binary);
    std::wofstream out(out_path, std::ios::binary);
    bfree(in_path);
    bfree(out_path);
#else
    std::ifstream in(qcstr(src), std::ios::binary);
    std::ofstream out(qcstr(dest), std::ios::binary);
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

void download_lyrics(const song* song)
{
    static QString last_lyrics = "";

    if (song->data() & CAP_LYRICS && last_lyrics != song->lyrics()) {
        last_lyrics = song->lyrics();
        if (!curl_download(song->lyrics(), config::lyrics_path)) {
            berr("Couldn't dowload lyrics from '%s' to '%s'",
                qcstr(song->lyrics()), config::lyrics_path);
        }
    }
}

void download_cover(const song* song)
{
    static QString last_cover = "";
    bool is_url = song->cover().startsWith("http")
        || song->cover().startsWith("file://");

    bool found_cover = song->data() & CAP_COVER && song->playing();

    if (found_cover && !song->cover().isEmpty() && song->cover() != last_cover) {
        if (is_url && config::download_cover) {
            QString tmp = config::cover_path;
            tmp += ".tmp";
            last_cover = song->cover();
            found_cover = curl_download(qcstr(song->cover()), qcstr(tmp));
            /* Replace cover only after download is done */
            if (found_cover) {
                move_file(tmp, config::cover_path);
            }
        } else if (!is_url) {
            last_cover = song->cover();
            found_cover = move_file(song->cover(), config::cover_path);
        }
    }

    if (!found_cover && last_cover != "n/a") {
        last_cover = "n/a";
        /* no cover => use place placeholder */
        if (!move_file(config::cover_placeholder, config::cover_path))
            berr("Couldn't move placeholder cover");
    }
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
        format::execute(tmp_text);

        if (tmp_text.isEmpty() || !s->playing())
            tmp_text = config::placeholder;
        write_song(tmp_text, output.second);
    }
}

int64_t epoch()
{
    return time(nullptr);
}

} // namespace util
