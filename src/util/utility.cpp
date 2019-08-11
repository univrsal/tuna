/**
 * This file is part of tuna
 * which is licensed under the GPL v2.0
 * See LICENSE or http://www.gnu.org/licenses
 * github.com/univrsal/tuna
 */
#include "utility.hpp"
#include "config.hpp"
#include "../query/music_source.hpp"
#include <curl/curl.h>
#include <stdio.h>
#include <obs-module.h>
#include <fstream>
#include <ctime>

namespace util {

    size_t write_data(void *ptr, size_t size, size_t nmemb, FILE *stream)
    {
        size_t written;
        written = fwrite(ptr, size, nmemb, stream);
        return written;
    }

    bool curl_download(const char* url, const char* path)
    {
        CURL* curl = curl_easy_init();
        FILE* fp = fopen(path, "wb");
        bool result = false;
        if (fp && curl) {
            curl_easy_setopt(curl, CURLOPT_URL, url);
            curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data);
            curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);
#ifdef DEBUG
            curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
#endif
            CURLcode res = curl_easy_perform(curl);

            if (res != CURLE_OK) {
                blog(LOG_ERROR, "[tuna] Couldn't fetch file from %s to %s", url, path);
            }
#ifdef DEBUG
            else {
                blog(LOG_DEBUG, "[tuna] Fetched %s to %s", url, path);
            }
#endif
            fclose(fp);
            result = true;
        }

        if (curl)
            curl_easy_cleanup(curl);
        return true;
    }

    bool move_file(const char* src, const char* dest)
    {
        std::ifstream in(src, std::ios::binary);
        std::ofstream out(dest, std::ios::binary);
        bool result = false;
        if (in.good() && out.good()) {
            out << in.rdbuf();
            result = true;
        }

        in.close();
        out.close();
        return result;
    }

    void handle_lyrics(const song_t* song)
    {
        static std::string last_lyrics = "";

        if (song->data & CAP_LYRICS && last_lyrics != song->lyrics) {
            last_lyrics = song->lyrics;
            if (!curl_download(song->lyrics.c_str(), config::lyrics_path))
                blog(LOG_ERROR, "[tuna] Couldn't dowload lyrics from '%s' to '%s'",
                     song->lyrics.c_str(), config::lyrics_path);
        }
    }

    void handle_cover_art(const song_t* song)
    {
        static std::string last_cover = "";
        bool is_url = song->cover.find("http") != std::string::npos;
        bool found_cover = song->data & CAP_COVER;

        if (!song->cover.empty() && song->cover != last_cover) {
            if (is_url && config::download_cover) {
                last_cover = song->cover;
                found_cover = curl_download(song->cover.c_str(), config::cover_path);
            } else if (!is_url) {
                last_cover = song->cover;
                found_cover = move_file(song->cover.c_str(), config::cover_path);
            }
        }

        if (!found_cover && last_cover != "n/a") {
            last_cover = "n/a";
            /* no cover => use place placeholder */
            if (!move_file(config::cover_placeholder, config::cover_path))
                blog(LOG_ERROR, "[tuna] couldn't move placeholder cover");
        }

    }

    void replace_all(std::string& str, const std::string& find, const std::string& replace)
    {
        if(find.empty())
            return;
        size_t start_pos = 0;
        while((start_pos = str.find(find, start_pos)) != std::string::npos) {
            str.replace(start_pos, find.length(), replace);
            start_pos += replace.length();
        }
    }

    int64_t epoch() {
        return time(nullptr);
    }
}
