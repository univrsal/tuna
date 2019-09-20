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
#include "../query/mpd_source.hpp"
#include "../query/spotify_source.hpp"
#include "../query/window_source.hpp"
#include "../util/tuna_thread.hpp"
#include "constants.hpp"
#include <jansson.h>
#include <QDir>
#include <obs-frontend-api.h>
#include <obs-module.h>
#include <util/config-file.h>
#include <util/platform.h>

namespace config {
config_t* instance = nullptr;
music_source* selected_source = nullptr;
spotify_source* spotify = nullptr;
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

    CDEF_STR(CFG_SONG_PATH, qPrintable(path_song_file));
    CDEF_STR(CFG_COVER_PATH, qPrintable(path_cover_art));
    CDEF_STR(CFG_LYRICS_PATH, qPrintable(path_lyrics));
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

    spotify->load();
#ifdef LINUX
    mpd->load();
#endif
    window->load();

    if (run && !thread::start())
        blog(LOG_ERROR, "[tuna] Couldn't start thread");

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
    window->load_gui_values();
}

void save()
{
    spotify->save();
#ifdef LINUX
    mpd->save();
#endif
    window->save();
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

void load_outputs(QList<QPair<QString, QString>> &table_content)
{
    table_content.clear();
    QString path = obs_get_module_data_path(obs_current_module());
    if (!path.endsWith(QDir::separator()))
            path += QDir::separator();
    path.append(OUTPUT_FILE);
    QFileInfo check(path);

    if (check.exists() && check.isFile()) {
        json_error_t error;
        json_t *file = json_load_file(path.toStdString().c_str(), 0, &error);

        if (file) {
            size_t index;
            json_t *val;
            json_array_foreach(file, index, val) {
                char *format, *path;
                if (json_unpack_ex(val, &error, 0, "{ssss}", JSON_FORMAT_ID, &format,
                               JSON_OUTPUT_PATH_ID, &path) < 0) {
                    blog(LOG_WARNING, "[tuna] failed to unpack json: %s", error.text);
                } else {
                    table_content.push_back(QPair<QString, QString>(format, path));
                }
            }
        } else {
            blog(LOG_WARNING, "[tuna] Error loading output json at line "
                              "%i (col: %i): %s", error.line, error.column, error.text);
        }
    } else {
        /* Nothing to load, add default */
        QDir home = QDir::homePath();
        QString default_output = QDir::toNativeSeparators(home.absoluteFilePath("song.txt"));
        table_content.push_back(QPair<QString, QString>(T_SONG_FORMAT_DEFAULT, default_output));
    }
}

void save_outputs(const QList<QPair<QString, QString>> &table_content)
{
    QString path = obs_get_module_data_path(obs_current_module());
    if (!path.endsWith(QDir::separator()))
            path += QDir::separator();
    path.append(OUTPUT_FILE);

    json_t* output_array = json_array();
    json_error_t error;

    for (const auto& pair : table_content) {
        json_t* obj = json_pack_ex(&error, 0, "{ssss}", JSON_FORMAT_ID,
                                   pair.first.toStdString().c_str(),
                                   JSON_OUTPUT_PATH_ID, pair.second.toStdString().c_str());

        if (obj) {
            json_array_append_new(output_array, obj);
        } else {
            blog(LOG_WARNING, "[tuna] Error encoding json: %s", error.text);
        }
    }

    if (json_dump_file(output_array, path.toStdString().c_str(), JSON_INDENT(4)) < 0) {
        blog(LOG_WARNING, "[tuna] Error writing json at line "
                          "%i (col: %i): %s", error.line, error.column, error.text);
    }
    json_array_clear(output_array);
    json_decref(output_array);
}
} // namespace config
