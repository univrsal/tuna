/**
 * This file is part of tuna
 * which is licensed under the GPL v2.0
 * See LICENSE or http://www.gnu.org/licenses
 * github.com/univrsal/tuna
 */
#pragma once

#ifdef _WIN32
#include <Windows.h>
#endif
#include <QString>
#include <mutex>

namespace thread {
    extern QString song_text;
    extern volatile bool thread_state;
    extern std::mutex mutex;
    bool start();
    void stop();

#ifdef _WIN32
    DWORD WINAPI thread_method(LPVOID arg);
#else
    void* thread_method(void*);
#endif
}
