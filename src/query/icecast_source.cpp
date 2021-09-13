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

#include "icecast_source.hpp"
#include "../gui/widgets/icecast.hpp"
#include "../util/config.hpp"
#include <QDateTime>
#include <QJsonDocument>
#include <QJsonObject>
#include <curl/curl.h>

icecast_source::icecast_source()
    : music_source(S_SOURCE_ICECAST, T_SOURCE_ICECAST, new icecast)
{
    m_capabilities = CAP_TITLE;
}

void icecast_source::load()
{
    music_source::load();
    CDEF_STR(CFG_ICECAST_URL, "");
    m_url = utf8_to_qt(CGET_STR(CFG_ICECAST_URL)) + "/status-json.xsl";
    m_logged_response_too_big = false;
}

void icecast_source::refresh()
{
    static char error_buffer[CURL_ERROR_SIZE];

    if (m_logged_response_too_big || m_url.isEmpty())
        return;

    begin_refresh();
    auto* curl = curl_easy_init();
    if (curl) {
        error_buffer[0] = '\0';
        std::string response;
        curl_easy_setopt(curl, CURLOPT_URL, qt_to_utf8(m_url));
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, util::write_callback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
        curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, error_buffer);
        auto result = curl_easy_perform(curl);
        curl_easy_cleanup(curl);

        if (result == CURLE_OK) {
            // Pretty arbitrary, but I have tested this with some stations
            // and they respond with ~1MB of data which we will not parse
            if (response.length() > 1024 * 512) {
                m_logged_response_too_big = true;
                berr("The IceCast server at %s responded with %zu bytes of data "
                     "which is too long and therefore will not be processed",
                    qt_to_utf8(m_url), response.length());
                return;
            }
            QJsonParseError err;
            auto doc = QJsonDocument::fromJson(response.c_str(), &err);

            if (doc.isNull() || !doc.isObject()) {
                berr("Failed to parse json response from IceCast server: %s", qt_to_utf8(err.errorString()));
            } else {
                auto stats = doc.object()["icestats"].toObject();
                if (!stats.isEmpty()) {
                    auto source = stats["source"].toObject();
                    if (source["title"].isString()) {
                        m_current.set_title(source["title"].toString());
                        m_current.set_state(state_playing);
                    }
                }
            }
        } else {
            auto epoch = QDateTime::currentSecsSinceEpoch();
            if (m_last_log == 0 || m_last_log - epoch > 10) {
                m_last_log = epoch;
                berr("Failed to retrieve information from IceCast server %s: cURL error '%s' (%i)",
                    qt_to_utf8(m_url), curl_easy_strerror(result), result);
                if (strlen(error_buffer) > 0)
                    berr("Additional curl error message: %s", error_buffer);
            }
        }
    }
}
