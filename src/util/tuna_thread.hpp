/*************************************************************************
 * This file is part of tuna
 * github.com/univrsal/tuna
 * Copyright 2021 univrsal <uni@vrsal.de>.
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

#include "src/query/song.hpp"
#include <QString>
#include <mutex>
#include <thread>

namespace tuna_thread {
extern std::atomic<bool> thread_flag;
extern std::mutex thread_mutex;
extern std::mutex copy_mutex;
extern std::thread thread_handle;
extern song copy;

bool start();

void stop();

void thread_method();
} // namespace thread
