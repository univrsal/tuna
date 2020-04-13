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

#include "config.hpp"
#include "../query/music_source.hpp"
#include "../util/tuna_thread.hpp"
#include "constants.hpp"
#include "utility.hpp"
#include <QDir>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <jansson.h>
#include <obs-frontend-api.h>
#include <obs-module.h>
#include <util/config-file.h>
#include <util/platform.h>

namespace config {

config_t* instance = nullptr;
uint16_t refresh_rate = 1000;
const char* placeholder = nullptr;
const char* cover_path = nullptr;
const char* lyrics_path = nullptr;
const char* selected_source = nullptr;
QList<output> outputs;
const char* cover_placeholder = nullptr;
bool download_cover = true;

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

    CDEF_BOOL(CFG_RUNNING, false);
    CDEF_BOOL(CFG_DOWNLOAD_COVER, true);
    CDEF_BOOL(CFG_FORCE_VLC_DECISION, false);
    CDEF_BOOL(CFG_ERROR_MESSAGE_SHOWN, false);
    CDEF_UINT(CFG_REFRESH_RATE, refresh_rate);
    CDEF_STR(CFG_SONG_PLACEHOLDER, T_PLACEHOLDER);

    CDEF_BOOL(CFG_DOCK_VISIBLE, false);
    CDEF_BOOL(CFG_DOCK_INFO_VISIBLE, true);
    CDEF_BOOL(CFG_DOCK_VOLUME_VISIBLE, true);

    if (!cover_placeholder)
        cover_placeholder = obs_module_file("placeholder.png");
}

void load()
{
    if (!instance)
        init();
    bool run = CGET_BOOL(CFG_RUNNING);

    load_outputs(outputs);
    cover_path = CGET_STR(CFG_COVER_PATH);
    lyrics_path = CGET_STR(CFG_LYRICS_PATH);
    refresh_rate = CGET_UINT(CFG_REFRESH_RATE);
    placeholder = CGET_STR(CFG_SONG_PLACEHOLDER);
    download_cover = CGET_BOOL(CFG_DOWNLOAD_COVER);
    selected_source = CGET_STR(CFG_SELECTED_SOURCE);

    /* Sources */
    thread::thread_mutex.lock();
    music_sources::load();
    thread::thread_mutex.unlock();

    if (run && !thread::start())
        berr("Couldn't start thread");

    music_sources::select(selected_source);
}

void save()
{
    music_sources::save();
    save_outputs(outputs);
}

void close()
{
    thread::thread_mutex.lock();
    save();
    util::reset_cover();
    thread::stop();
    thread::thread_mutex.unlock();

    /* Wait for thread to exit to delete resources */
    while (thread::thread_running)
        os_sleep_ms(5);
    bfree((void*)cover_placeholder);
    thread::thread_mutex.lock();
    music_sources::deinit();
    thread::thread_mutex.unlock();
}

void load_outputs(QList<output>& table_content)
{
    table_content.clear();
    QDir home = QDir::homePath();
    QString path = QDir::toNativeSeparators(home.absoluteFilePath(OUTPUT_FILE));
    QFile file(path);

    if (file.open(QIODevice::ReadOnly)) {
        auto doc = QJsonDocument::fromJson(file.readAll());
        QJsonArray array;
        if (doc.isArray())
            array = doc.array();

        for (const auto val : array) {
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
            table_content.push_back(tmp);
        }
        binfo("Loaded %i outputs", array.size());
    } else {
        /* Nothing to load, add default */
        binfo("No config exists, creating default");
        QDir home = QDir::homePath();
        output tmp;
        tmp.format = T_SONG_FORMAT_DEFAULT;
        tmp.path = QDir::toNativeSeparators(home.absoluteFilePath("song.txt"));
        tmp.log_mode = false;
        table_content.push_back(tmp);
    }
}

void save_outputs(const QList<output>& outputs)
{
    QJsonArray output_array;
    QDir home = QDir::homePath();
    QString path = QDir::toNativeSeparators(home.absoluteFilePath(OUTPUT_FILE));

#ifdef UNIX
    QDir folder = QDir::home();
    folder.cd(OUTPUT_FOLDER);

    if (!folder.exists() && !folder.mkdir(".")) {
        berr("Couldn't create config folder");
        return;
    }
#endif

    for (const auto& o : outputs) {
        QJsonObject output;
        output[JSON_FORMAT_ID] = o.format;
        output[JSON_OUTPUT_PATH_ID] = QDir::toNativeSeparators(o.path);
        output[JSON_FORMAT_LOG_MODE] = o.log_mode;
        output[JSON_LAST_OUTPUT] = o.last_output;
        output_array.append(output);
    }

    if (output_array.empty()) {
        binfo("No ouputs to save");
    } else {
        QJsonDocument doc(output_array);
        QFile save_file(path);
        if (save_file.open(QIODevice::WriteOnly)) {
            auto data = doc.toJson();
            auto wrote = save_file.write(data);
            if (data.length() != wrote) {
                berr("Couldn't write outputs to %s only"
                     "wrote %lli bytes out of %i",
                    qt_to_utf8(path), wrote, data.length());
            }
            save_file.close();
        } else {
            berr("Couldn't write outputs to %s", qt_to_utf8(path));
        }
    }
}

} // namespace config
