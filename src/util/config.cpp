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
const char *selected_source = nullptr;
QList<QPair<QString, QString>> outputs;
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
    thread::mutex.lock();
    source::load();
    thread::mutex.unlock();

    if (run && !thread::start())
        berr("Couldn't start thread");

    source::select(selected_source);
}

void save()
{
    source::save();
    save_outputs(outputs);
}

void close()
{
    thread::mutex.lock();
    save();
    thread::stop();
    thread::mutex.unlock();

    /* Wait for thread to exit to delete resources */
    while (thread::thread_state)
        os_sleep_ms(5);
    bfree((void*)cover_placeholder);
    source::deinit();
}

void load_outputs(QList<QPair<QString, QString>>& table_content)
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
            table_content.push_back(QPair<QString, QString>(
                obj[JSON_FORMAT_ID].toString(),
                obj[JSON_OUTPUT_PATH_ID].toString()));
        }
        binfo("Loaded %i outputs", array.size());
    } else {
        /* Nothing to load, add default */
        binfo("No config exists, creating default");
        QDir home = QDir::homePath();
        QString default_output = QDir::toNativeSeparators(home.absoluteFilePath("song.txt"));
        table_content.push_back(QPair<QString, QString>(T_SONG_FORMAT_DEFAULT, default_output));
    }
}

void save_outputs(const QList<QPair<QString, QString>>& table_content)
{
    QDir home = QDir::homePath();
    QString path = QDir::toNativeSeparators(home.absoluteFilePath(OUTPUT_FILE));
    QFileInfo check(path);

    QJsonArray output_array;

    for (const auto& pair : table_content) {
        QJsonObject output;
        output[JSON_FORMAT_ID] = pair.first;
        output[JSON_OUTPUT_PATH_ID] = QDir::toNativeSeparators(pair.second);
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
                    path.toStdString().c_str(),
                    wrote, data.length());
            }
            save_file.close();
        } else {
            berr("Couldn't write outputs to %s", qt_to_utf8(path));
        }
    }
}

} // namespace config
