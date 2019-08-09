/**
 * This file is part of tuna
 * which is licensed under the GPL v2.0
 * See LICENSE or http://www.gnu.org/licenses
 * github.com/univrsal/tuna
 */
#include "utility.hpp"
#include <curl/curl.h>
#include <stdio.h>

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
            fclose(fp);
            result = true;
        }

        if (curl)
            curl_easy_cleanup(curl);
        return true;
    }
}
