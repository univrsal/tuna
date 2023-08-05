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

#include "utility.hpp"
#include "../query/music_source.hpp"
#include "config.hpp"
#include "constants.hpp"
#include "format.hpp"
#include <QGuiApplication>
#include <QScreen>

#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QTextStream>
#include <ctime>
#include <curl/curl.h>
#include <obs-module.h>
#include <sstream>
#include <stdio.h>
#include <util/platform.h>
#if _WIN32
#    include <windows.h>
const DWORD MS_VC_EXCEPTION = 0x406D1388;
#    pragma pack(push, 8)
typedef struct tagTHREADNAME_INFO {
    DWORD dwType;     // Must be 0x1000.
    LPCSTR szName;    // Pointer to name (in user addr space).
    DWORD dwThreadID; // Thread ID (-1=caller thread).
    DWORD dwFlags;    // Reserved for future use, must be zero.
} THREADNAME_INFO;
#    pragma pack(pop)

inline void os_set_thread_name(const char* name)
{
    THREADNAME_INFO info;
    info.dwType = 0x1000;
    info.szName = name;
    info.dwThreadID = GetCurrentThreadId();
    info.dwFlags = 0;

    __try {
        RaiseException(0x406D1388, 0, sizeof(info) / sizeof(DWORD), (const ULONG_PTR*)&info);
    } __except (EXCEPTION_CONTINUE_EXECUTION) {
    }
}
#else
#    include <util/threading.h>
#endif
#include <util/util.hpp>

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
            berr("Couldn't fetch file from %s to %s, curl error: %s (%i)", url, path, curl_easy_strerror(res), res);
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

    auto l = song.get(meta::LYRICS);
    if (!l.isEmpty() && l != last_lyrics) {
        last_lyrics = l;
        if (!curl_download(qt_to_utf8(l), qt_to_utf8(config::lyrics_path))) {
            berr("Couldn't dowload lyrics from '%s' to '%s'", qt_to_utf8(l), qt_to_utf8(config::lyrics_path));
        }
    }
}

bool download_cover(const QString& url)
{
    if (url == "n/a")
        return false;
    bool result = false;
    auto output_path = config::cover_path;
    auto tmp = output_path + ".tmp";

    static const int prefix_length =
#if _WIN32
        8;
#else
        7;
#endif
    if (url.startsWith("file://")) {
        // Don't use curl for local files
        QString new_cover_path = QUrl::fromPercentEncoding(url.mid(prefix_length).toUtf8());
        QFile cover(new_cover_path);
        if (cover.exists()) {
            QFile current(output_path);
            current.remove();
            if (QFile::copy(new_cover_path, output_path))
                return true;
            berr("Couldn't copy cover file from '%s' to '%s'", qt_to_utf8(new_cover_path), qt_to_utf8(output_path));
            return false;
        }
        berr("Cover file '%s' does not exist", qt_to_utf8(new_cover_path));
        return false;
    }

    result = curl_download(qt_to_utf8(url), qt_to_utf8(tmp));

    /* Replace cover only after download is done */
    QFile current(output_path);
    current.remove();

    if (result && !QFile::rename(tmp, output_path)) {
        berr("Couldn't rename temporary cover file");
        result = false;
    }
    return result;
}

void reset_cover()
{
    auto path = config::cover_path;
    QFile current(path);
    current.remove();
    if (!QFile::copy(config::cover_placeholder, path))
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
#if QT_VERSION_MAJOR < 6
        stream.setCodec("UTF-8");
#else
        stream.setEncoding(QStringConverter::Utf8);
#endif
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

        if (tmp_text.isEmpty() || s.get<int>(meta::STATUS) >= state_paused) {
            tmp_text = config::placeholder;
            /* OBS seems to cut leading and trailing spaces
             * when loading the config file so this workaround
             * allows users to still use them */
            tmp_text.replace("%s", " ");
            tmp_text.replace("%e", "\n");
        }
        if (s.get<int>(meta::STATUS) >= state_paused && o.log_mode)
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
        berr("Error reading curl, exception: %s", e.what());
        return 0;
    }
    return new_length;
}

QJsonDocument curl_get_json(const char* url)
{
    CURL* curl = curl_easy_init();
    if (curl) {
        std::string response {};
        curl_easy_setopt(curl, CURLOPT_URL, url);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
#ifdef DEBUG
        curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
#endif
        CURLcode res = curl_easy_perform(curl);

        if (res != CURLE_OK) {
            berr("Couldn't fetch json from %s curl error: %s (%i)", url, curl_easy_strerror(res), res);
        } else {
            QJsonParseError err;
            auto doc = QJsonDocument::fromJson(response.c_str(), &err);
            if (doc.isNull())
                berr("Couldn't parse json from url %s: %s", url, err.errorString().toStdString().c_str());
            else
                return doc;
        }
        curl_easy_cleanup(curl);
    }
    berr("curl_easy_init() failed when receiving json from %s", url);
    return {};
}

void set_thread_name(const char* name)
{
    os_set_thread_name(name);
}

QString remove_extensions(QString const& str)
{
    QString result = str;
    if (CGET_BOOL(CFG_REMOVE_EXTENSIONS)) {
        /* that's every single format supported by vlc, i think */
        auto exts = { ".aac", ".ac3", ".adts", ".aif", ".aifc", ".aiff", ".amr", ".amv",
            ".aob", ".aqt", ".asf", ".ass", ".asx", ".au", ".avc", ".avchd",
            ".avi", ".ax", ".b4s", ".bdmv", ".cda", ".cdg", ".clpi", ".cue",
            ".dash", ".div", ".divx", ".dts", ".dv", ".dvdmedia", ".f4v", ".flac",
            ".flh", ".flv", ".gsm", ".gvi", ".gvp", ".h264", ".hdmov", ".ifo",
            ".iso", ".it", ".jss", ".kmv", ".lrv", ".m1v", ".m2a", ".m2p",
            ".m2t", ".m2ts", ".m3u", ".m3u8", ".m4a", ".m4b", ".m4p", ".m4v",
            ".mid", ".mka", ".mkv", ".mlp", ".mod", ".moi", ".moov", ".mov",
            ".mp1", ".mp2", /* yeah, as if anybody still uses mp2 */
            ".mp2v", ".mp3", ".mp4", ".mp4.infovid", ".mp4v", ".mpa", ".mpc", ".mpe",
            ".mpeg", ".mpeg1", ".mpeg4", ".mpg", ".mpg2", ".mpls", ".mpsub", ".mpv",
            ".mpv2", ".mts", ".mxf", ".nsv", ".nuv", ".oga", ".ogg", ".ogm",
            ".ogv", ".ogx", ".oma", ".opus", ".pjs", ".pss", ".ra", ".ram",
            ".rec", ".rm", ".rmi", ".rmvb", ".rt", ".s3m", ".s3z", ".smi",
            ".snd", ".spx", ".srt", ".sub", ".svi", ".tod", ".trp", ".ts",
            ".tta", ".usf", ".vlc", ".vlt", ".vob", ".voc", ".vp6", ".vqf",
            ".vro", ".vse", ".w64", ".wav", ".webm", ".wma", ".wmv", ".wv",
            ".xa", ".xm", ".xspf", ".xvid", ".3g2", ".3ga", ".3gp", ".3gp2",
            ".3gpp", ".3p2", ".261", ".3gp_128x96", ".axa", ".axv", ".cache-2", ".cache-3",
            ".eac3", ".flvat", ".h260", ".mbv", ".mks", ".ml20", ".mp3a", ".mp4a",
            ".mpeg2", ".mpg4", ".mpgv", ".thd", ".vfo", ".xavc", ".xwm", ".zab" };

        for (const auto& ext : exts) {
            if (result.endsWith(ext, Qt::CaseInsensitive)) {
                result.chop(strlen(ext));
                break;
            }
        }
    }
    return result;
}

QString get_config_file_path_legacy(const char* name)
{
#ifdef UNIX
    QDir home = QDir::home();
    home.cd(CONFIG_FOLDER);

    if (!home.exists() && !home.mkdir(".")) {
        berr("Couldn't create config folder");
        return "";
    }
#else
    QDir home = QDir::homePath();
#endif
    return QDir::toNativeSeparators(home.absoluteFilePath(utf8_to_qt(name)));
}

QString get_config_file_path(QString const& name)
{
    BPtr<char> path = obs_module_config_path(qt_to_utf8(name));
    return utf8_to_qt(path.Get());
}

bool open_config(const char* name, QJsonDocument& doc)
{
    auto new_path = get_config_file_path(name);
    QFile new_file(new_path);

    if (new_file.exists() && new_file.open(QIODevice::ReadWrite)) {
        doc = QJsonDocument::fromJson(new_file.readAll());
        return true;
    }
    // Try the old location
    auto old_path = get_config_file_path_legacy(name);
    QFile file(old_path);
    binfo("Could not find '%s' trying old location '%s'", qt_to_utf8(new_path), qt_to_utf8(old_path));

    if (file.exists() && file.open(QIODevice::ReadWrite)) {
        doc = QJsonDocument::fromJson(file.readAll());
        return true;
    }
    return false;
}

bool save_config(const char* name, const QJsonDocument& doc)
{
    auto path = get_config_file_path(name);
    QFile save_file(path);
    auto result = false;
    if (save_file.open(QIODevice::WriteOnly)) {
        auto data = doc.toJson();
        auto wrote = save_file.write(data);
        if (data.length() == wrote) {
            result = true;
        } else {
            berr("Couldn't write config file to %s, only"
                 "wrote %lli bytes out of %i",
                qt_to_utf8(path), wrote, int(data.length()));
        }
        save_file.close();
    } else {
        berr("Couldn't write config to %s", qt_to_utf8(path));
    }
    return result;
}

QString file_from_path(const QString& path)
{
    auto splits = path.split("/");
    if (splits.empty() || splits.last().isEmpty())
        return "unknown";
    return splits.last();
}

void create_config_folder()
{
    BPtr<char> path = obs_module_config_path("");
    if (os_mkdirs(path) == MKDIR_ERROR)
        berr("Failed to create config directory %s", path.Get());
}

void reset_lyrics()
{
    QFile out(config::lyrics_path);
    if (out.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream stream(&out);
#if QT_VERSION_MAJOR < 6
        stream.setCodec("UTF-8");
#else
        stream.setEncoding(QStringConverter::Utf8);
#endif
        stream << "\n";
        stream.flush();
        out.close();
    } else {
        berr("Failed to reset lyrics file at %s", qt_to_utf8(config::lyrics_path));
    }
}

bool write_lyrics(const QString& lyrics)
{
    QFile out(config::lyrics_path);
    if (out.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream stream(&out);
#if QT_VERSION_MAJOR < 6
        stream.setCodec("UTF-8");
#else
        stream.setEncoding(QStringConverter::Utf8);
#endif
        stream << lyrics;
        stream.flush();
        out.close();
        return true;
    }
    berr("Failed to write lyrics file at %s", qt_to_utf8(config::lyrics_path));
    return false;
}

} // namespace util
