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

#include "lastfm_source.hpp"
#include "../gui/widgets/lastfm.hpp"
#include "../util/config.hpp"
#include "../util/utility.hpp"
#include "util/platform.h"
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QString>
#include <QUrl>
#include <curl/curl.h>

long lastfm_request(QJsonDocument& response_json, const QString& url);

lastfm_source::lastfm_source()
    : music_source(S_SOURCE_LAST_FM, T_SOURCE_LASTFM, new lastfm)
{
    supported_metadata({ meta::ALBUM, meta::COVER, meta::TITLE, meta::ARTIST, meta::DURATION });
}

void lastfm_source::load()
{
    music_source::load();
    m_username = utf8_to_qt(CGET_STR(CFG_LASTFM_USERNAME));
    m_api_key = utf8_to_qt(CGET_STR(CFG_LASTFM_API_KEY));
    if (m_api_key.isEmpty()) {
        m_custom_api_key = false;
        m_api_key = utf8_to_qt(LASTFM_CREDENTIALS);
    } else {
        m_custom_api_key = true;
    }
}

void lastfm_source::refresh()
{
    if (m_api_key.isEmpty()) {
        berr("No lastfm api key");
        return;
    }

    if (m_username.isEmpty())
        return;

    /* last.fm doesn't want apps to constantly send requets
     * to their API points
     * so this source uses slower refresh than the user might configure
     * in the gui if the shared api key is used
     */
    if (!m_custom_api_key && os_gettime_ns() < m_next_refresh)
        return;

    begin_refresh();
    m_current.clear();
    QString track_request = "https://ws.audioscrobbler.com/2.0/?method=user.getrecenttracks&user=" + m_username + "&api_key=" + m_api_key + "&limit=1&format=json";
    QJsonDocument response;
    auto code = lastfm_request(response, track_request);
    if (code == HTTP_OK) {
        auto recent_tracks = response.object()["recenttracks"].toObject();

        if (recent_tracks["track"].isArray()) {
            auto track_arr = recent_tracks["track"].toArray();
            if (track_arr.size() > 0) {
                auto song = track_arr[0].toObject();
                if (!song.isEmpty())
                    parse_song(song);
            }
        }

        /* Since we don't know the progress of the song there's no way to
         * determine when the next request would be due, so a query every
         * five seconds should be slow enough, unless a custom api key is
         * used.
         */
        m_next_refresh = os_gettime_ns() + 5000000000;
    } else {
        berr("Received error code from last.fm request: %i", int(code));
        m_next_refresh = os_gettime_ns() + 1500000000;
    }
}

void lastfm_source::parse_song(const QJsonObject& s)
{
    if (s["@attr"].isObject()) {
        auto attr_obj = s["@attr"].toObject();
        m_current.set(meta::STATUS, attr_obj["nowplaying"].toString() == "true" ? state_playing : state_stopped);

        if (m_current.get<int>(meta::STATUS) == state_playing) {
            auto covers = s["image"];
            if (covers.isArray() && covers.toArray().size() > 0) {
                auto cover_array = covers.toArray();
                auto cover = cover_array[cover_array.size() - 1];
                if (cover.isObject())
                    m_current.set(meta::COVER, cover.toObject()["#text"].toString());
            }
        }
        util::download_cover(m_current.get(meta::COVER));
    }

    if (s["artist"].isObject())
        m_current.set(meta::ARTIST, QStringList(s["artist"].toObject()["#text"].toString()));

    if (s["album"].isObject())
        m_current.set(meta::ALBUM, s["album"].toObject()["#text"].toString());

    if (s["name"].isString())
        m_current.set(meta::TITLE, s["name"].toString());

    if (m_current.has(meta::ARTIST) && m_current.has(meta::TITLE)) {
        /* Try and get song duration */
        QString artist = QUrl::toPercentEncoding(m_current.get<QStringList>(meta::ARTIST).at(0));
        QString track = QUrl::toPercentEncoding(m_current.get(meta::TITLE));
        QString track_request = "https://ws.audioscrobbler.com/2.0/?method="
                                "track.getInfo&api_key="
            + m_api_key + "&artist=" + artist + "&track=" + track + "&format=json";

        QJsonDocument response;
        auto code = lastfm_request(response, track_request);
        if (code == HTTP_OK) {
            auto track_obj = response.object()["track"];
            if (track_obj.isObject()) {
                auto duration = track_obj.toObject()["duration"];
                if (duration.isString()) {
                    bool ok = false;
                    auto str = duration.toString();
                    int i = str.toInt(&ok);
                    if (ok)
                        m_current.set(meta::DURATION, i);
                }
            }
        }
    }
}

bool lastfm_source::execute_capability(capability)
{
    return true;
}

bool lastfm_source::enabled() const
{
    return true;
}

/* === cURL stuff == */

long lastfm_request(QJsonDocument& response_json, const QString& url)
{
    CURL* curl = curl_easy_init();
    std::string response;
    long http_code = -1;
    // curl_easy_setopt(curl, CURLOPT_POST, 1L);
    curl_easy_setopt(curl, CURLOPT_URL, qt_to_utf8(url));
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, util::write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
#ifdef DEBUG
    curl_easy_setopt(curl, CURLOPT_VERBOSE, 1);
#endif
    CURLcode res = curl_easy_perform(curl);

    if (res == CURLE_OK) {
        binfo("Curl response: %i", (int)response.length());
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
        QJsonParseError err;
        response_json = QJsonDocument::fromJson(response.c_str(), &err);
        if (response_json.isNull() && !response.empty())
            berr("Failed to parse json response: %s, Error: %s", response.c_str(), qt_to_utf8(err.errorString()));
    } else {
        berr("CURL failed while sending spotify command");
    }

    curl_easy_cleanup(curl);
    return http_code;
}
