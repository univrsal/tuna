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

#pragma once
#include "../query/song.hpp"
#include <atomic>
#include <mutex>
#include <thread>

/* This thread runs a server that hosts music information in a JSON file */
namespace web_thread {
extern std::thread thread_handle;
extern std::mutex current_song_mutex;
extern song current_song;
extern std::atomic<bool> thread_flag;
bool start();
void stop();
void thread_method();
}
