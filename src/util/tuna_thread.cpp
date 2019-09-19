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

#include "tuna_thread.hpp"
#include "../gui/tuna_gui.hpp"
#include "../query/music_source.hpp"
#include "config.hpp"
#include "utility.hpp"
#include <obs-module.h>
#include <util/platform.h>

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


bool start()
{
    bool result = true;
    if (thread_state)
        return result;
    thread_state = true;

#ifdef _WIN32
    thread_handle = CreateThread(nullptr,
        0,
        static_cast<LPTHREAD_START_ROUTINE>(thread_method),
        nullptr,
        0,
        nullptr);
    result = thread_handle;
#else
    result = pthread_create(&thread_handle, nullptr, thread_method, nullptr) == 0;
#endif
    thread_state = result;
    return result;
}

void stop()
{
    thread_state = false;
}

#ifdef _WIN32
DWORD WINAPI
thread_method(LPVOID arg)
#else

void* thread_method(void*)
#endif
{
    while (thread_state) {
        mutex.lock();
        if (config::selected_source) {
            config::selected_source->refresh();
            auto* s = config::selected_source->song();

            /* Process song data */
            util::handle_cover_art(s);
            util::handle_lyrics(s);
            util::handle_outputs(s);
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
} // namespace thread
