/**
 * This file is part of tuna
 * which is licensed under the GPL v2.0
 * See LICENSE or http://www.gnu.org/licenses
 * github.com/univrsal/tuna
 */
#include "tuna_thread.hpp"
#include "config.hpp"
#include "../query/music_source.hpp"
#include "../gui/tuna_gui.hpp"
#include "utility.hpp"
#include <util/platform.h>
#include <obs-module.h>
#include <fstream>
#ifdef LINUX
#include <pthread.h>
#endif
namespace thread {
    volatile bool thread_state = false;
    std::mutex mutex;

#ifdef _WIN32
    static HANDLE thread_handle;
#else
    static pthread_t thread_handle;
#endif

    void write_song(const char* line)
    {
        std::ofstream file(config::song_path, std::ofstream::out | std::ofstream::trunc);
        if (file.good()) {
            file << line;
            file.close();
        } else {
            blog(LOG_ERROR, "[tuna] Couldn't open song output file %s", config::song_path);
        }
    }
    bool start()
    {
        bool result = true;
        if (thread_state)
            return result;
        thread_state = true;

#ifdef _WIN32
        thread_handle = CreateThread(nullptr, 0,
                                     static_cast<LPTHREAD_START_ROUTINE>(thread_method),
                                     nullptr, 0, nullptr);
        result = thread_handle;
#else
        result = pthread_create(&thread_handle, nullptr, thread_method, nullptr) == 0;
#endif
        thread_state = result;
        return result;
    }

    void stop()
    {
        if (!thread_state)
            return;
        thread_state = false;
    }

    void format_string(QString& out, const song_t* song)
    {
        out = config::format_string;
        if (song->data & CAP_TITLE)
            out.replace("%t", song->title.c_str());
        if (song->data & CAP_ARTIST)
            out.replace("%m", song->artists.c_str());
        if (song->data & CAP_ALBUM)
            out.replace("%a", song->album.c_str());
        if (song->data & CAP_RELEASE) {
            QString full_date = "";
            switch(song->release_precision) {
            case prec_day:
                full_date.append(song->day.c_str()).append('.');
            case prec_month:
                full_date.append(song->month.c_str()).append('.');
            case prec_year:
                full_date.append(song->year.c_str());
            default:;
            }
            out.replace("%y", song->year.c_str());
            out.replace("%r", full_date);
        }
    }

#ifdef _WIN32
    DWORD WINAPI thread_method(LPVOID arg);
#else
    void* thread_method(void*)
#endif
    {
        QString formatted;
        std::string last_cover_url = "";
        std::string last_lyrics_url = "";

        while (thread_state) {
            mutex.lock();
            if (config::selected_source) {
                config::selected_source->refresh();
                formatted = "";
                auto* s = config::selected_source->song();

                /* Process song data */
                if (s->data & CAP_COVER && last_cover_url != s->cover) {
                    last_cover_url = s->cover;
                    util::curl_download(s->cover.c_str(), config::cover_path);
                }

                if (s->data & CAP_LYRICS && last_lyrics_url != s->lyrics) {
                    last_lyrics_url = s->lyrics;
                    util::curl_download(s->lyrics.c_str(), config::lyrics_path);
                }

                format_string(formatted, s);
                if (formatted.length() < 1 || !s->is_playing) {
                    write_song(config::placeholder);
                } else {
                    write_song(qPrintable(formatted));
                }
                /* This adds 'Preview: ' to the string so do this last */
                tuna_dialog->set_output_preview(formatted);
            }

            mutex.unlock();
            os_sleep_ms(config::refresh_rate);
        }

#ifdef _WIN32
        return 0;
#else
        pthread_exit(nullptr);
#endif
    }
}
