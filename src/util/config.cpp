/*************************************************************************
 * This file is part of tuna
 * github.com/univrsal/tuna
 * Copyright 2021 univrsal <uni@vrsal.de>.
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

#include "config.hpp"
#include "../query/music_source.hpp"
#include "constants.hpp"
#include "tuna_thread.hpp"
#include "utility.hpp"
#include "web_server.hpp"
#include <QDir>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <obs-frontend-api.h>
#include <obs-module.h>
#include <util/config-file.h>
#include <util/platform.h>

namespace config {

bool post_load = false;
QList<output> outputs;
config_t* instance = nullptr;
uint16_t refresh_rate = 1000;
uint16_t webserver_port = 1608;
QString placeholder = {};
QString cover_path = {};
QString lyrics_path = {};
QString cover_placeholder = {};
QString selected_source = {};
bool webserver_enabled = false;
bool download_cover = true;
bool download_missing_cover = true;
bool placeholder_when_paused = true;
bool remove_file_extensions = true;

void init()
{
    if (!instance)
        instance = obs_frontend_get_global_config();

    QDir home = QDir::homePath();
    QString path_song_file = QDir::toNativeSeparators(home.absoluteFilePath("song.txt"));
    QString path_cover_art = QDir::toNativeSeparators(home.absoluteFilePath("cover.png"));
    QString path_lyrics = QDir::toNativeSeparators(home.absoluteFilePath("lyrics.txt"));

    CDEF_STR(CFG_SONG_PATH, qt_to_utf8(path_song_file));
    CDEF_STR(CFG_COVER_PATH, qt_to_utf8(path_cover_art));
    CDEF_STR(CFG_LYRICS_PATH, qt_to_utf8(path_lyrics));
    CDEF_STR(CFG_SELECTED_SOURCE, S_SOURCE_SPOTIFY);
    CDEF_STR(CFG_SPOTIFY_CLIENT_ID, "847d7cf0c5dc4ff185161d1f000a9d0e");

    CDEF_BOOL(CFG_REMOVE_EXTENSIONS, config::remove_file_extensions);
    CDEF_BOOL(CFG_PLACEHOLDER_WHEN_PAUSED, config::placeholder_when_paused);
    CDEF_BOOL(CFG_RUNNING, false);
    CDEF_BOOL(CFG_DOWNLOAD_COVER, config::download_cover);
    CDEF_BOOL(CFG_DOWNLOAD_MISSING_COVER, config::download_cover);
    CDEF_UINT(CFG_REFRESH_RATE, config::refresh_rate);
    CDEF_UINT(CFG_SERVER_PORT, config::webserver_port);
    CDEF_STR(CFG_SONG_PLACEHOLDER, T_PLACEHOLDER);

    CDEF_BOOL(CFG_DOCK_VISIBLE, false);
    CDEF_BOOL(CFG_DOCK_INFO_VISIBLE, true);
    CDEF_BOOL(CFG_DOCK_VOLUME_VISIBLE, true);
    CDEF_BOOL(CFG_SERVER_ENABLED, false);

    auto tmp = obs_module_file("placeholder.png");
    cover_placeholder = tmp;
    bfree((void*)tmp);
}

void load()
{
    if (!instance)
        init();
    bool run = CGET_BOOL(CFG_RUNNING);

    load_outputs();
    cover_path = CGET_STR(CFG_COVER_PATH);
    lyrics_path = CGET_STR(CFG_LYRICS_PATH);
    refresh_rate = CGET_UINT(CFG_REFRESH_RATE);
    placeholder = CGET_STR(CFG_SONG_PLACEHOLDER);
    download_cover = CGET_BOOL(CFG_DOWNLOAD_COVER);
    placeholder_when_paused = CGET_BOOL(CFG_PLACEHOLDER_WHEN_PAUSED);
    remove_file_extensions = CGET_BOOL(CFG_REMOVE_EXTENSIONS);
    webserver_enabled = CGET_BOOL(CFG_SERVER_ENABLED);
    webserver_port = CGET_UINT(CFG_SERVER_PORT);
    selected_source = CGET_STR(CFG_SELECTED_SOURCE);

    /* Sources */
    tuna_thread::thread_mutex.lock();
    music_sources::load();
    tuna_thread::thread_mutex.unlock();

    if (run && !tuna_thread::start())
        berr("Couldn't start query thread");

    if (webserver_enabled && !web_thread::start())
        berr("Couldn't start web server thread");

    music_sources::select(qt_to_utf8(selected_source));
}

void save()
{
    tuna_thread::thread_mutex.lock();
    save_outputs();
    util::reset_cover();
    tuna_thread::thread_mutex.unlock();
    music_sources::deinit();
    web_thread::stop();
    CSET_STR(CFG_COVER_PATH, qt_to_utf8(cover_path));
    CSET_STR(CFG_LYRICS_PATH, qt_to_utf8(lyrics_path));
    CSET_UINT(CFG_REFRESH_RATE, refresh_rate);
    CSET_STR(CFG_SONG_PLACEHOLDER, qt_to_utf8(placeholder));
    CSET_BOOL(CFG_DOWNLOAD_COVER, download_cover);
    CSET_BOOL(CFG_PLACEHOLDER_WHEN_PAUSED, placeholder_when_paused);
    CSET_BOOL(CFG_REMOVE_EXTENSIONS, remove_file_extensions);
    CSET_BOOL(CFG_SERVER_ENABLED, webserver_enabled);
    CSET_UINT(CFG_SERVER_PORT, webserver_port);
    CSET_STR(CFG_SELECTED_SOURCE, qt_to_utf8(selected_source));
}

void load_outputs()
{
    std::lock_guard<std::mutex> lock(tuna_thread::thread_mutex);
    outputs.clear();

    QJsonDocument doc;
    if (util::open_config(OUTPUT_FILE, doc)) {
        QJsonArray array;
        if (doc.isArray())
            array = doc.array();

        for (const auto& val : qAsConst(array)) {
            QJsonObject obj = val.toObject();
            output tmp;
            tmp.format = obj[JSON_FORMAT_ID].toString();
            tmp.path = obj[JSON_OUTPUT_PATH_ID].toString();
            if (obj[JSON_FORMAT_LOG_MODE].isBool())
                tmp.log_mode = obj[JSON_FORMAT_LOG_MODE].toBool();
            else
                tmp.log_mode = false;

            if (obj[JSON_LAST_OUTPUT].isString())
                tmp.last_output = obj[JSON_LAST_OUTPUT].isString();
            else
                tmp.last_output = "";
            outputs.push_back(tmp);
        }
        binfo("Loaded %i outputs", array.size());
    }
}

void save_outputs()
{
    QJsonArray output_array;
    for (const auto& o : qAsConst(outputs)) {
        QJsonObject output;
        output[JSON_FORMAT_ID] = o.format;
        output[JSON_OUTPUT_PATH_ID] = QDir::toNativeSeparators(o.path);
        output[JSON_FORMAT_LOG_MODE] = o.log_mode;
        output[JSON_LAST_OUTPUT] = o.last_output;
        output_array.append(output);
    }
    util::save_config(OUTPUT_FILE, QJsonDocument(output_array));
}

}
