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

#include "config.hpp"
#include "utility.hpp"
#include "../query/mpd_source.hpp"
#include "../query/spotify_source.hpp"
#include "../query/vlc_obs_source.hpp"
#include "../query/window_source.hpp"
#include "../util/tuna_thread.hpp"
#include "constants.hpp"
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
music_source* selected_source = nullptr;
spotify_source* spotify = nullptr;
vlc_obs_source* vlc_obs = nullptr;
window_source* window = nullptr;
mpd_source* mpd = nullptr;

uint16_t refresh_rate = 1000;
const char* placeholder = nullptr;
const char* cover_path = nullptr;
const char* lyrics_path = nullptr;
QList<QPair<QString, QString>> outputs;
const char* cover_placeholder = nullptr;
bool download_cover = true;

void init_default()
{
    QDir home = QDir::homePath();
    QString path_song_file = QDir::toNativeSeparators(home.absoluteFilePath("song.txt"));
    QString path_cover_art = QDir::toNativeSeparators(home.absoluteFilePath("cover.png"));
    QString path_lyrics = QDir::toNativeSeparators(home.absoluteFilePath("lyrics.txt"));

    CDEF_STR(CFG_SONG_PATH, path_song_file.toStdString().c_str());
    CDEF_STR(CFG_COVER_PATH, path_cover_art.toStdString().c_str());
    CDEF_STR(CFG_LYRICS_PATH, path_lyrics.toStdString().c_str());
    CDEF_UINT(CFG_SELECTED_SOURCE, src_spotify);

    CDEF_BOOL(CFG_RUNNING, false);
    CDEF_BOOL(CFG_DOWNLOAD_COVER, true);
    CDEF_UINT(CFG_REFRESH_RATE, refresh_rate);
    CDEF_STR(CFG_SONG_FORMAT, T_FORMAT);
    CDEF_STR(CFG_SONG_PLACEHOLDER, T_PLACEHOLDER);

    if (!cover_placeholder)
        cover_placeholder = obs_module_file("placeholder.png");
}

void select_source(source s)
{
    thread::mutex.lock();
    switch (s) {
    default:
    case src_spotify:
        selected_source = spotify;
        break;
#ifdef LINUX
    case src_mpd:
        selected_source = mpd;
        break;
#endif
    case src_vlc_obs:
        selected_source = vlc_obs;
        break;
    case src_window_title:
        selected_source = window;
        break;
    }
    thread::mutex.unlock();
}

void load()
{
    if (!instance)
        instance = obs_frontend_get_global_config();
    init_default();
    bool run = CGET_BOOL(CFG_RUNNING);

    cover_path = CGET_STR(CFG_COVER_PATH);
    lyrics_path = CGET_STR(CFG_LYRICS_PATH);
    load_outputs(outputs);
    refresh_rate = CGET_UINT(CFG_REFRESH_RATE);
    placeholder = CGET_STR(CFG_SONG_PLACEHOLDER);
    download_cover = CGET_BOOL(CFG_DOWNLOAD_COVER);

    /* Sources */
    if (!spotify)
        spotify = new spotify_source;
#ifdef LINUX
    if (!mpd)
        mpd = new mpd_source;
#endif
    if (!window)
        window = new window_source;
    if (!vlc_obs)
        vlc_obs = new vlc_obs_source;

    spotify->load();
    vlc_obs->load();
#ifdef LINUX
    mpd->load();
#endif
    window->load();

    if (run && !thread::start())
         berr("Couldn't start thread");

    auto src = CGET_UINT(CFG_SELECTED_SOURCE);
    if (src < src_count)
        select_source((source)src);
}

void load_gui_values()
{
    spotify->load_gui_values();
#ifdef LINUX
    mpd->load_gui_values();
#endif
    vlc_obs->load_gui_values();
    window->load_gui_values();
}

void save()
{
    spotify->save();
#ifdef LINUX
    mpd->save();
#endif
    window->save();
    vlc_obs->save();
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

    delete spotify;
#ifdef LINUX
    delete mpd;
#endif
    delete window;
    window = nullptr;
    spotify = nullptr;
    mpd = nullptr;
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

        for (const auto& val : array) {
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
                                "wrote %i bytes out of %i",
                    path.toStdString().c_str(),
                    wrote, data.length());
            }
            save_file.close();
        } else {
            berr("Couldn't write outputs to %s", path.toStdString().c_str());
        }
    }
}

} // namespace config
