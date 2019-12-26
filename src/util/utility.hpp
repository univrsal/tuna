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

#pragma once

#include <stdint.h>
#include <string>

#define SECOND_TO_NS 1000000000
struct song_t;

namespace util {
extern bool vlc_loaded;

void load_vlc();

void unload_vlc();

bool curl_download(const char* url, const char* path);

void handle_cover_art(const song_t* song);

void handle_lyrics(const song_t* song);

void handle_outputs(const song_t* song);

bool move_file(const char* src, const char* dest);

void replace_all(std::string& str,
    const std::string& find,
    const std::string& replace);

int64_t epoch();
} // namespace util
