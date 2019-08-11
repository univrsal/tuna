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
#include "../query/window_source.hpp"
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
    window_source* window = nullptr;
    mpd_source* mpd = nullptr;

    uint16_t refresh_rate = 1000;
    const char* format_string = nullptr;
    const char* placeholder = nullptr;
    const char* cover_path = nullptr;
    const char* lyrics_path = nullptr;
    const char* song_path = nullptr;
    const char* cover_placeholder = nullptr;
    bool download_cover = true;

    void init_default()
    {
        QString home = QDir::homePath().append(QDir::separator());
        QString path_song_file = home + "song.txt";
        QString path_cover_art = home + "cover.png";
        QString path_lyrics = home + "lyrics.txt";

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
        switch(s) {
        default:
        case src_spotify:
            selected_source = spotify;
            break;
        case src_mpd:
            selected_source = mpd;
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
        song_path = CGET_STR(CFG_SONG_PATH);
        refresh_rate = CGET_UINT(CFG_REFRESH_RATE);
        format_string = CGET_STR(CFG_SONG_FORMAT);
        placeholder = CGET_STR(CFG_SONG_PLACEHOLDER);
        download_cover = CGET_BOOL(CFG_DOWNLOAD_COVER);

        /* Sources */
        spotify = new spotify_source;
        mpd = new mpd_source;
        window = new window_source();

        spotify->load();
        mpd->load();
        window->load();

        if (run && !thread::start())
            blog(LOG_ERROR, "[tuna] Couldn't start thread");

        auto src = CGET_UINT(CFG_SELECTED_SOURCE);
        if (src < src_count)
            select_source((source) src);
    }

    void load_gui_values()
    {
        spotify->load_gui_values();
        mpd->load_gui_values();
        window->load_gui_values();
    }

    void save()
    {
        CSET_BOOL(CFG_RUNNING, thread::thread_state);
        spotify->save();
        mpd->save();
        window->save();
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
        delete mpd;
        delete window;
        window = nullptr;
        spotify = nullptr;
        mpd = nullptr;
    }
}
