/*************************************************************************
 * This file is part of tuna
 * github.con/univrsal/tuna
 * Copyright 2020 univrsal <universailp@web.de>.
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

#ifdef _WIN32
#include <Windows.h>
#endif

#include <QString>
#include <mutex>

#include "src/query/song.hpp"

namespace thread {
extern volatile bool thread_flag;
extern volatile bool thread_running;
extern std::mutex thread_mutex;
extern std::mutex copy_mutex;
extern song copy;

bool start();

void stop();

#ifdef _WIN32
DWORD WINAPI thread_method(LPVOID arg);
#else

void* thread_method(void*);

#endif
} // namespace thread
