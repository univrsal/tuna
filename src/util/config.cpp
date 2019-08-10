/**
 * This file is part of tuna
 * which is licensed under the GPL v2.0
 * See LICENSE or http://www.gnu.org/licenses
 * github.com/univrsal/tuna
 */
#include "config.hpp"
#include "constants.hpp"
#include "../query/spotify_source.hpp"
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

    uint16_t refresh_rate = 1000;
    const char* format_string = "";
    const char* placeholder = "";
    const char* cover_path = "";
    const char* lyrics_path = "";
    const char* song_path = "";

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
        config_set_default_uint(instance, CFG_REGION, CFG_REFRESH_RATE, refresh_rate);
        config_set_default_string(instance, CFG_REGION, CFG_SONG_FORMAT,
                                  T_FORMAT);
        config_set_default_string(instance, CFG_REGION, CFG_SONG_PLACEHOLDER,
                                  T_PLACEHOLDER);
    }

    void select_source(source s)
    {
        switch(s) {
        case src_spotify:
            selected_source = spotify;
            break;
        case src_mpd:
            break;
        case src_window_title:
            break;
        }
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

        /* Sources */
        spotify = new spotify_source;

        spotify->load();

        if (run && !thread::start())
            blog(LOG_ERROR, "[tuna] Couldn't start thread");

        auto src = config_get_uint(instance, CFG_REGION, CFG_SELECTED_SOURCE);
        if (src < src_count)
            select_source((source) src);
    }

    void close()
    {
        thread::stop();
        spotify->save();

        delete spotify;
        spotify = nullptr;
    }
}
