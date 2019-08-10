/**
 * This file is part of tuna
 * which is licensed under the GPL v2.0
 * See LICENSE or http://www.gnu.org/licenses
 * github.com/univrsal/tuna
 */
#include "config.hpp"
#include "constants.hpp"
#include "../query/spotify_source.hpp"
#include "../query/mpd_source.hpp"
#include "../util/tuna_thread.hpp"
#include <util/config-file.h>
#include <util/platform.h>
#include <obs-module.h>
#include <obs-frontend-api.h>
#include <QDir>

namespace config
{
    config_t* instance = nullptr;
    music_source* selected_source = nullptr;
    spotify_source* spotify = nullptr;
    mpd_source* mpd = nullptr;

    uint16_t refresh_rate = 1000;
    const char* format_string = "";
    const char* placeholder = "";
    const char* cover_path = "";
    const char* lyrics_path = "";
    const char* song_path = "";
    const char* cover_placeholder = "";
    bool download_cover = true;

    void init_default()
    {
        QString home = QDir::homePath().append(QDir::separator());
        QString path_song_file = home + "song.txt";
        QString path_cover_art = home + "cover.png";
        QString path_lyrics = home + "lyrics.txt";

        config_set_default_string(instance, CFG_REGION, CFG_SONG_PATH,
                                  qPrintable(path_song_file));
        config_set_default_string(instance, CFG_REGION, CFG_COVER_PATH,
                                  qPrintable(path_cover_art));
        config_set_default_string(instance, CFG_REGION, CFG_LYRICS_PATH,
                                  qPrintable(path_lyrics));
        config_set_default_uint(instance, CFG_REGION, CFG_SELECTED_SOURCE,
                               src_spotify);
        config_set_default_bool(instance, CFG_REGION, CFG_RUNNING, false);
        config_set_default_bool(instance, CFG_REGION, CFG_DOWNLOAD_COVER, true);
        config_set_default_uint(instance, CFG_REGION, CFG_REFRESH_RATE, refresh_rate);
        config_set_default_string(instance, CFG_REGION, CFG_SONG_FORMAT,
                                  T_FORMAT);
        config_set_default_string(instance, CFG_REGION, CFG_SONG_PLACEHOLDER,
                                  T_PLACEHOLDER);
        cover_placeholder = obs_module_file("placeholder.png");
    }

    void select_source(source s)
    {
        thread::mutex.lock();
        switch(s) {
        default:
        case src_spotify:
            selected_source = spotify;
            break;
        case src_mpd:
            selected_source = mpd;
            break;
        case src_window_title:
            break;
        }
        thread::mutex.unlock();
    }

    void load()
    {
        instance = obs_frontend_get_global_config();
        init_default();
        bool run = config_get_bool(instance, CFG_REGION, CFG_RUNNING);

        cover_path = config_get_string(instance, CFG_REGION, CFG_COVER_PATH);
        lyrics_path = config_get_string(instance, CFG_REGION, CFG_LYRICS_PATH);
        song_path = config_get_string(instance, CFG_REGION, CFG_SONG_PATH);
        refresh_rate = config_get_uint(instance, CFG_REGION, CFG_REFRESH_RATE);
        format_string = config_get_string(instance, CFG_REGION, CFG_SONG_FORMAT);
        placeholder = config_get_string(instance, CFG_REGION, CFG_SONG_PLACEHOLDER);
        download_cover = config_get_bool(instance, CFG_REGION, CFG_DOWNLOAD_COVER);

        /* Sources */
        spotify = new spotify_source;
        mpd = new mpd_source;

        spotify->load();
        mpd->load();

        if (run && !thread::start())
            blog(LOG_ERROR, "[tuna] Couldn't start thread");

        auto src = config_get_uint(instance, CFG_REGION, CFG_SELECTED_SOURCE);
        if (src < src_count)
            select_source((source) src);
    }

    void load_gui_values()
    {
        spotify->load_gui_values();
        mpd->load_gui_values();
    }

    void save()
    {
        config_set_bool(instance, CFG_REGION, CFG_RUNNING, thread::thread_state);
        spotify->save();
        mpd->save();
    }

    void close()
    {
        thread::mutex.lock();
        save();
        thread::stop();
        bfree((void*)cover_placeholder);

        delete spotify;
        delete mpd;
        spotify = nullptr;
        mpd = nullptr;
        thread::mutex.unlock();
    }
}
