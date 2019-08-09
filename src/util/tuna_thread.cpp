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
        thread_state = true;
        if (thread_state)
            return result;

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

#ifdef _WIN32
    DWORD WINAPI thread_method(LPVOID arg);
#else
    void* thread_method(void*)
#endif
    {
        while (thread_state) {
            mutex.lock();
            if (config::selected_source) {
                config::selected_source->refresh();
                auto* s = config::selected_source->song();
                if (s->data & CAP_COVER) {

                }

                if (s->data & CAP_TITLE) {
                    tuna_dialog->set_output_preview(s->title.c_str());
                }
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
