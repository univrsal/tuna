/*************************************************************************
 * This file is part of tuna
 * git.vrsal.xyz/alex/tuna
 * Copyright 2023 univrsal <uni@vrsal.xyz>.
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
#include "../query/music_source.hpp"
#include "config.hpp"
#include "utility.hpp"
#include <algorithm>
#include <obs-module.h>
#include <util/platform.h>

namespace tuna_thread {
std::atomic<bool> thread_flag { false };
song copy;
std::mutex thread_mutex;
std::mutex copy_mutex;
std::thread thread_handle;

bool start()
{
    if (thread_flag)
        return true;
    std::lock_guard<std::mutex> lock(thread_mutex);
    thread_handle = std::thread(thread_method);
    return thread_flag = thread_handle.native_handle();
}

void stop()
{
    if (!thread_flag)
        return;
    bdebug("Stopping query thread...");
    thread_flag = false;
    thread_handle.join();
    bdebug("Query thread stopped.");

    bdebug("Resetting song information...");
    /* Set status to nothing before stopping */
    auto src = music_sources::selected_source();
    src->reset_info();
    util::handle_outputs(src->song_info());
    bdebug("Song information reset.");
}

void thread_method()
{
    util::set_thread_name("tuna-query");

    while (thread_flag) {
        const uint64_t start = os_gettime_ns() / 1000000;
        {
            auto ref = music_sources::selected_source();
            if (ref) {
                {
                    // We don't want to hold the lock while waiting
                    std::lock_guard<std::mutex> lock(thread_mutex);
                    ref->refresh();
                    ref->post_refresh();
                }
                auto s = ref->song_info();

                /* Make a copy for the progress bar source, because it can't
                 * wait for the other processes to finish, otherwise it'll block
                 * the video thread
                 */
                copy_mutex.lock();
                copy = s;
                copy_mutex.unlock();

                /* Process song data */
                util::handle_outputs(s);
                if (config::download_cover)
                    ref->handle_cover();
                if (config::download_lyrics)
                    ref->handle_lyrics();
            }
        }

        /* Calculate how long refresh took and only wait the remaining time
         * but we wait at least 10 ms otherwise we will immediately lock the mutex
         * again which can stall other threads that are waiting to lock it
         */
        const uint64_t end = os_gettime_ns() / 1000000;
        uint64_t delta = std::clamp<uint64_t>(end - start, 10ul, config::refresh_rate - 10);
        int64_t wait = config::refresh_rate - delta;
        const int64_t step = 50;

        bdebug("Query thread sleeping for %ims", int(wait)); // macOS doesn't like %lu so we'll just cast to int, who cares

        // We only wait int 50 ms steps so we can catch the thread stopping faster
        while (wait > 0 && thread_flag) {
            os_sleep_ms(step);
            wait -= step;
        }
    }
    binfo("Query thread stopped.");
}
}
