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

#include "utility.hpp"
#include "../query/music_source.hpp"
#include "config.hpp"
#include "constants.hpp"
#include "format.hpp"
#include <QGuiApplication>
#include <QScreen>

#include <QFile>
#include <QMessageBox>
#include <QTextStream>
#include <ctime>
#include <curl/curl.h>
#include <obs-module.h>
#include <stdio.h>
#include <util/platform.h>
#include <util/threading.h>

namespace util {

bool have_vlc_source = false;

size_t write_data(void* ptr, size_t size, size_t nmemb, FILE* stream)
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

        if (res != CURLE_OK) {
            berr("Couldn't fetch file from %s to %s", url, path);
        } else {
            result = true;
            bdebug("Fetched %s to %s", url, path);
        }
    }

    if (fp)
        fclose(fp);

    if (curl)
        curl_easy_cleanup(curl);
    return result;
}

void download_lyrics(const song& song)
{
    static QString last_lyrics = "";

    if (song.data() & CAP_LYRICS && last_lyrics != song.lyrics()) {
        last_lyrics = song.lyrics();
        if (!curl_download(qt_to_utf8(song.lyrics()), config::lyrics_path)) {
            berr("Couldn't dowload lyrics from '%s' to '%s'", qt_to_utf8(song.lyrics()), config::lyrics_path);
        }
    }
}

bool download_cover(const song& song)
{
    bool result = false;
    auto path = utf8_to_qt(config::cover_path);
    auto tmp = path + ".tmp";
    result = curl_download(qt_to_utf8(song.cover()), qt_to_utf8(tmp));

    /* Replace cover only after download is done */
    QFile current(path);
    current.remove();

    if (result && !QFile::rename(tmp, utf8_to_qt(config::cover_path))) {
        berr("Couldn't rename temporary cover file");
        result = false;
    }
    return result;
}

void reset_cover()
{
    auto path = utf8_to_qt(config::cover_path);
    QFile current(path);
    current.remove();
    if (!QFile::copy(utf8_to_qt(config::cover_placeholder), path))
        berr("Couldn't move placeholder cover");
}

void write_song(config::output& o, const QString& str)
{
    if (o.last_output == str)
        return;
    o.last_output = str;

    QFile out(o.path);
    bool success = false;
    if (o.log_mode)
        success = out.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Append);
    else
        success = out.open(QIODevice::WriteOnly | QIODevice::Text);

    if (success) {
        QTextStream stream(&out);
        stream.setCodec("UTF-8");
        stream << str;
        if (o.log_mode)
            stream << "\n";
        stream.flush();
        out.close();
    } else {
        berr("Couldn't open song output file %s", qt_to_utf8(o.path));
    }
}

void handle_outputs(const song& s)
{
    static QString tmp_text = "";

    for (auto& o : config::outputs) {
        tmp_text.clear();
        tmp_text = o.format;
        format::execute(tmp_text);

        if (tmp_text.isEmpty() || s.state() >= state_paused) {
            tmp_text = config::placeholder;
            /* OBS seems to cut leading and trailing spaces
             * when loading the config file so this workaround
             * allows users to still use them */
            tmp_text.replace("%s", " ");
        }
        if (s.state() >= state_paused && o.log_mode)
            continue; /* No song playing text doesn't make sense in the log */
        write_song(o, tmp_text);
    }
}

int64_t epoch()
{
    return time(nullptr);
}

bool window_pos_valid(QRect rect)
{
    for (const auto& screen : QGuiApplication::screens()) {
        if (screen->availableGeometry().intersects(rect))
            return true;
    }
    return false;
}

size_t write_callback(char* ptr, size_t size, size_t nmemb, std::string* str)
{
    size_t new_length = size * nmemb;
    try {
        str->append(ptr, new_length);
    } catch (std::bad_alloc& e) {
        berr("Error reading curl response: %s", e.what());
        return 0;
    }
    return new_length;
}

void set_thread_name(const char* name)
{
    os_set_thread_name(name);
}

void remove_extensions(QString& str)
{
    if (CGET_BOOL(CFG_REMOVE_EXTENSIONS)) {
        /* that's every single format supported by vlc, i think */
        auto exts = {
            ".aac", ".ac3", ".adts", ".aif", ".aifc", ".aiff", ".amr",
            ".amv", ".aob", ".aqt", ".asf", ".ass", ".asx", ".au", ".avc",
            ".avchd", ".avi", ".ax", ".b4s", ".bdmv", ".cda", ".cdg",
            ".clpi", ".cue", ".dash", ".div", ".divx", ".dts", ".dv",
            ".dvdmedia", ".f4v", ".flac", ".flh", ".flv", ".gsm", ".gvi",
            ".gvp", ".h264", ".hdmov", ".ifo", ".iso", ".it", ".jss",
            ".kmv", ".lrv", ".m1v", ".m2a", ".m2p", ".m2t", ".m2ts", ".m3u",
            ".m3u8", ".m4a", ".m4b", ".m4p", ".m4v", ".mid", ".mka", ".mkv",
            ".mlp", ".mod", ".moi", ".moov", ".mov", ".mp1", ".mp2", /* yeah, as if anybody still uses mp2 */
            ".mp2v", ".mp3", ".mp4", ".mp4.infovid", ".mp4v", ".mpa",
            ".mpc", ".mpe", ".mpeg", ".mpeg1", ".mpeg4", ".mpg", ".mpg2",
            ".mpls", ".mpsub", ".mpv", ".mpv2", ".mts", ".mxf", ".nsv",
            ".nuv", ".oga", ".ogg", ".ogm", ".ogv", ".ogx", ".oma", ".opus",
            ".pjs", ".pss", ".ra", ".ram", ".rec", ".rm", ".rmi", ".rmvb",
            ".rt", ".s3m", ".s3z", ".smi", ".snd", ".spx", ".srt", ".sub",
            ".svi", ".tod", ".trp", ".ts", ".tta", ".usf", ".vlc", ".vlt",
            ".vob", ".voc", ".vp6", ".vqf", ".vro", ".vse", ".w64", ".wav",
            ".webm", ".wma", ".wmv", ".wv", ".xa", ".xm", ".xspf", ".xvid",
            ".3g2", ".3ga", ".3gp", ".3gp2", ".3gpp", ".3p2", ".261",
            ".3gp_128x96", ".axa", ".axv", ".cache-2", ".cache-3", ".eac3",
            ".flvat", ".h260", ".mbv", ".mks", ".ml20", ".mp3a", ".mp4a",
            ".mpeg2", ".mpg4", ".mpgv", ".thd", ".vfo", ".xavc", ".xwm", ".zab"
        };

        for (const auto& ext : exts) {
            if (str.toLower().endsWith(ext)) {
                str = str.left(QString(ext).length());
                break;
            }
        }
    }
}
} // namespace util
