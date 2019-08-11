/**
 * This file is part of tuna
 * which is licensed under the GPL v2.0
 * See LICENSE or http://www.gnu.org/licenses
 * github.com/univrsal/tuna
 */
#pragma once
#include <stdint.h>

struct song_t;

namespace util {
    bool curl_download(const char* url, const char* path);

    void handle_cover_art(const song_t* song);
    void handle_lyrics(const song_t* song);
    bool move_file(const char* src, const char* dest);

    int64_t epoch();
}
