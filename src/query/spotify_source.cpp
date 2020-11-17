/*************************************************************************
 * This file is part of tuna
 * github.com/univrsal/tuna
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

#include "spotify_source.hpp"
#include "../gui/music_control.hpp"
#include "../gui/tuna_gui.hpp"
#include "../gui/widgets/spotify.hpp"
#include "../util/config.hpp"
#include "../util/constants.hpp"
#include "../util/creds.hpp"
#include "../util/utility.hpp"
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QString>
#include <curl/curl.h>
#include <util/config-file.h>
#include <util/platform.h>

#define TOKEN_URL "https://accounts.spotify.com/api/token"
#define PLAYER_URL "https://api.spotify.com/v1/me/player"
#define PLAYER_PAUSE_URL (PLAYER_URL "/pause")
#define PLAYER_PLAY_URL (PLAYER_URL "/play")
#define PLAYER_NEXT_URL (PLAYER_URL "/next")
#define PLAYER_PREVIOUS_URL (PLAYER_URL "/previous")
#define PLAYER_VOLUME_URL (PLAYER_URL "/volume")
#define CURL_DEBUG 0L
#define REDIRECT_URI "https%3A%2F%2Funivrsal.github.io%2Fauth%2Ftoken"

spotify_source::spotify_source()
    : music_source(S_SOURCE_SPOTIFY, T_SOURCE_SPOTIFY, new spotify)
{
    build_credentials();
    m_capabilities = CAP_TITLE | CAP_ARTIST | CAP_ALBUM | CAP_RELEASE | CAP_COVER | CAP_DURATION | CAP_NEXT_SONG | CAP_PREV_SONG | CAP_PLAY_PAUSE | CAP_VOLUME_MUTE | CAP_PREV_SONG | CAP_STATUS;
}

bool spotify_source::enabled() const
{
    return true;
}

void spotify_source::build_credentials()
{
    auto client_id = utf8_to_qt(CGET_STR(CFG_SPOTIFY_CLIENT_ID));
    auto client_secret = utf8_to_qt(CGET_STR(CFG_SPOTIFY_CLIENT_SECRET));

    if (!client_id.isEmpty() && !client_secret.isEmpty()) {
        m_creds = (client_id + ":" + client_secret).toUtf8().toBase64();
    } else {
        QString str = SPOTIFY_CREDENTIALS;
        m_creds = str.toUtf8().toBase64();
    }
}

void spotify_source::load()
{
    CDEF_BOOL(CFG_SPOTIFY_LOGGEDIN, false);
    CDEF_STR(CFG_SPOTIFY_TOKEN, "");
    CDEF_STR(CFG_SPOTIFY_AUTH_CODE, "");
    CDEF_STR(CFG_SPOTIFY_REFRESH_TOKEN, "");
    CDEF_INT(CFG_SPOTIFY_TOKEN_TERMINATION, 0);
    CDEF_STR(CFG_SPOTIFY_CLIENT_ID, "");
    CDEF_STR(CFG_SPOTIFY_CLIENT_SECRET, "");

    m_logged_in = CGET_BOOL(CFG_SPOTIFY_LOGGEDIN);
    m_token = utf8_to_qt(CGET_STR(CFG_SPOTIFY_TOKEN));
    m_refresh_token = utf8_to_qt(CGET_STR(CFG_SPOTIFY_REFRESH_TOKEN));
    m_auth_code = utf8_to_qt(CGET_STR(CFG_SPOTIFY_AUTH_CODE));
    m_token_termination = CGET_INT(CFG_SPOTIFY_TOKEN_TERMINATION);
    build_credentials();
    music_source::load();

    /* Token handling */
    if (m_logged_in) {
        if (util::epoch() > m_token_termination) {
            binfo("Refreshing Spotify token");
            QString log;
            const auto result = do_refresh_token(log);
            if (result)
                binfo("Successfully renewed Spotify token");
            save();
            music_source::load(); // Reload token stuff etc.
        }
    }
}

bool spotify_source::valid_format(const QString&)
{
    /* Supports all specifiers */
    return true;
}

/* implementation further down */
long execute_command(const char* auth_token, const char* url, std::string& response_header,
    QJsonDocument& response_json, bool put = false);

void extract_timeout(const std::string& header, uint64_t& timeout)
{
    static const std::string what = "Retry-After: ";
    timeout = 0;
    auto pos = header.find(what);

    if (pos != std::string::npos) {
        pos += what.length();
        auto end = pos;
        while (header.at(end) != '\n')
            end++;
        const auto tmp = header.substr(pos, end - pos);
        timeout = std::stoi(tmp);
    }
}

void spotify_source::refresh()
{
    if (!m_logged_in)
        return;

    begin_refresh();

    if (util::epoch() > m_token_termination) {
        binfo("Refreshing Spotify token");
        QString log;
        const auto result = do_refresh_token(log);
        emit((spotify*)m_settings_tab)->login_state_changed(result, log);
        save();
    }

    if (m_timout_start) {
        if (os_gettime_ns() - m_timout_start >= m_timeout_length) {
            m_timout_start = 0;
            m_timeout_length = 0;
            binfo("API timeout of %li seconds is over", m_timeout_length);
        } else {
            bdebug("Waiting for Spotify-API timeout");
            return;
        }
    }

    std::string header = "";
    QJsonDocument response;
    QJsonObject obj;

    const auto http_code = execute_command(qt_to_utf8(m_token), PLAYER_URL, header, response);

    if (response.isObject())
        obj = response.object();
    const QString str(response.toJson());

    if (http_code == HTTP_OK) {
        const auto& progress = obj["progress_ms"];
        const auto& device = obj["device"];
        const auto& playing = obj["is_playing"];
        const auto& play_type = obj["currently_playing_type"];

        /* If an ad is playing we assume playback is paused */
        if (play_type.isString() && play_type.toString() == "ad") {
            m_current.set_state(state_paused);
            return;
        }

        if (device.isObject() && playing.isBool()) {
            if (device.toObject()["is_private"].toBool()) {
                berr("Spotify session is private! Can't read track");
            } else {
                parse_track_json(obj["item"]);
                m_current.set_state(playing.toBool() ? state_playing : state_stopped);
            }
            m_current.set_progress(progress.toInt());
        } else {
            QString str(response.toJson());
            berr("Couldn't fetch song data from spotify json: %s", str.toStdString().c_str());
        }
        m_last_state = m_current.state();
    } else if (http_code == HTTP_NO_CONTENT) {
        /* No session running */
        m_current.clear();
    } else {
        /* Don't reset cover or info here since
         * we're just waiting for the API to give a proper
         * response again
         */
        if (http_code == STATUS_RETRY_AFTER && !header.empty()) {
            extract_timeout(header, m_timeout_length);
            if (m_timeout_length) {
                bwarn("Spotify-API Rate limit hit, waiting %li seconds\n", m_timeout_length);
                m_timeout_length *= SECOND_TO_NS;
                m_timout_start = os_gettime_ns();
            }
        } else {
            bwarn("Unknown error occured when querying Spotify-API: %li (response: %s)", http_code, qt_to_utf8(str));
        }
    }
}

void spotify_source::parse_track_json(const QJsonValue& track)
{
    const auto& trackObj = track.toObject();
    const auto& album = trackObj["album"].toObject();
    const auto& artists = trackObj["artists"].toArray();

    m_current.clear();

    /* Get All artists */
    for (const auto& artist : qAsConst(artists))
        m_current.append_artist(artist.toObject()["name"].toString());

    /* Cover link */
    const auto& covers = album["images"];
    if (covers.isArray()) {
        const QJsonValue v = covers.toArray()[0];
        if (v.isObject() && v.toObject().contains("url"))
            m_current.set_cover_link(v.toObject()["url"].toString());
    }

    /* Other stuff */
    m_current.set_title(trackObj["name"].toString());
    m_current.set_duration(trackObj["duration_ms"].toInt());
    m_current.set_album(album["name"].toString());
    m_current.set_explicit(trackObj["explicit"].toBool());
    m_current.set_disc_number(trackObj["disc_number"].toInt());
    m_current.set_track_number(trackObj["track_number"].toInt());

    /* Release date */
    const auto& date = album["release_date"].toString();
    if (date.length() > 0) {
        QStringList list = date.split("-");
        switch (list.length()) {
        case 3:
            m_current.set_day(list[2]);
            [[clang::fallthrough]];
        case 2:
            m_current.set_month(list[1]);
            [[clang::fallthrough]];
        case 1:
            m_current.set_year(list[0]);
        }
    }
}

bool spotify_source::execute_capability(capability c)
{
    std::string header;
    long http_code = -1;
    QJsonDocument response;

    switch (c) {
    case CAP_PLAY_PAUSE:
        if (m_current.state()) {
            [[clang::fallthrough]];
        case CAP_STOP_SONG:
            http_code = execute_command(qt_to_utf8(m_token), PLAYER_PAUSE_URL, header, response, true);
        } else {
            http_code = execute_command(qt_to_utf8(m_token), PLAYER_PLAY_URL, header, response, true);
        }
        break;
    case CAP_PREV_SONG:
        http_code = execute_command(qt_to_utf8(m_token), PLAYER_PREVIOUS_URL, header, response, true);
        break;
    case CAP_NEXT_SONG:
        http_code = execute_command(qt_to_utf8(m_token), PLAYER_NEXT_URL, header, response, true);
        break;
    case CAP_VOLUME_UP:
        /* TODO? */
        break;
    case CAP_VOLUME_DOWN:
        /* TODO? */
        break;
    default:;
    }

    /* Parse response */
    if (http_code != HTTP_NO_CONTENT) {
        QString r(response.toJson());
        binfo("Couldn't run spotify command! HTTP code: %li", http_code);
        binfo("Spotify controls only work for premium users!");
        binfo("Response: %s", qt_to_utf8(r));
    }

    return http_code == HTTP_NO_CONTENT;
}

/* === CURL/Spotify API handling === */

size_t header_callback(char* ptr, size_t size, size_t nmemb, std::string* str)
{
    size_t new_length = size * nmemb;
    try {
        str->append(ptr, new_length);
    } catch (std::bad_alloc& e) {
        berr("Error reading curl header: %s", e.what());
        return 0;
    }
    return new_length;
}

CURL* prepare_curl(struct curl_slist* header, std::string* response, std::string* response_header,
    const std::string& request)
{
    CURL* curl = curl_easy_init();

    curl_easy_setopt(curl, CURLOPT_POST, 1L);
    curl_easy_setopt(curl, CURLOPT_URL, TOKEN_URL);
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, header);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, strlen(request.c_str()));
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, request.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, util::write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, response);
    curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, header_callback);
    curl_easy_setopt(curl, CURLOPT_HEADERDATA, response_header);
#ifdef DEBUG
    curl_easy_setopt(curl, CURLOPT_VERBOSE, CURL_DEBUG);
#endif
    return curl;
}

/* Requests an access token via request body
 * over a POST request to spotify */
void request_token(const std::string& request, const std::string& credentials, QJsonDocument& response_json)
{
    if (request.empty() || credentials.empty()) {
        berr("Cannot request token without valid credentials"
             " and/or auth code!");
        return;
    }

    std::string response, response_header;
    std::string header = "Authorization: Basic ";
    header.append(credentials);

    auto* list = curl_slist_append(nullptr, header.c_str());
    CURL* curl = prepare_curl(list, &response, &response_header, request);
    CURLcode res = curl_easy_perform(curl);

    if (res == CURLE_OK) {
        QJsonParseError err;
        response_json = QJsonDocument::fromJson(response.c_str(), &err);
        if (response_json.isNull()) {
            berr("Couldn't parse response to json: %s", err.errorString().toStdString().c_str());
        } else {
            /* Log response without tokens */
            auto obj = response_json.object();
            if (obj["access_token"].isString())
                obj["access_token"] = "REDACTED";
            if (obj["refresh_token"].isString())
                obj["refresh_token"] = "REDACTED";
            auto doc = QJsonDocument(obj);
            QString str(doc.toJson());
            binfo("Spotify response: %s", qt_to_utf8(str));
        }
    } else {
        berr("Curl returned error code %i", res);
    }

    curl_slist_free_all(list);
    curl_easy_cleanup(curl);
}

/* Gets a new token using the refresh token */
bool spotify_source::do_refresh_token(QString& log)
{
    build_credentials();
    static std::string request;
    bool result = true;
    QJsonDocument response;

    if (m_refresh_token.isEmpty()) {
        berr("Refresh token is empty!");
        return false;
    }

    request = "grant_type=refresh_token&refresh_token=";
    request.append(m_refresh_token.toStdString());
    request_token(request, m_creds.toStdString(), response);

    if (response.isNull()) {
        berr("Couldn't refresh Spotify token, response was null");
        return false;
    } else {
        const auto& response_obj = response.object();
        const auto& token = response_obj["access_token"];
        const auto& expires = response_obj["expires_in"];

        const auto& refresh_token = response_obj["refresh_token"];

        /* Dump the json into the log text */
        log = QString(response.toJson(QJsonDocument::Indented));
        if (!token.isNull() && !expires.isNull()) {
            m_token = token.toString();
            m_token_termination = util::epoch() + expires.toInt();
            m_logged_in = true;
            binfo("Successfully logged in");
        } else {
            berr("Couldn't parse json response");
            result = false;
        }

        /* Refreshing the token can return a new refresh token */
        if (refresh_token.isString()) {
            QString tmp = refresh_token.toString();
            if (!tmp.isEmpty()) {
                binfo("Received a new fresh token");
                m_refresh_token = refresh_token.toString();
            }
        }
    }

    m_logged_in = result;
    return result;
}

/* Gets the first token from the access code */
bool spotify_source::new_token(QString& log)
{
    build_credentials();
    static std::string request;
    bool result = true;
    QJsonDocument response;
    request = "grant_type=authorization_code&code=";
    request.append(m_auth_code.toStdString());
    request.append("&redirect_uri=").append(REDIRECT_URI);
    request_token(request, m_creds.toStdString(), response);

    if (response.isObject()) {
        const auto& response_obj = response.object();
        const auto& token = response_obj["access_token"];
        const auto& refresh = response_obj["refresh_token"];
        const auto& expires = response_obj["expires_in"];

        /* Dump the json into the log textbox */
        log = QString(response.toJson(QJsonDocument::Indented));

        if (token.isString() && refresh.isString() && expires.isDouble()) {
            m_token = token.toString();
            m_refresh_token = refresh.toString();
            m_token_termination = util::epoch() + expires.toInt();
            result = true;
        } else {
            berr("Couldn't parse json response!");
            result = false;
        }
    } else {
        result = false;
    }

    m_logged_in = result;
    save();
    return result;
}

/* Sends commands to spotify api via url */

long execute_command(const char* auth_token, const char* url, std::string& response_header,
    QJsonDocument& response_json, bool put)
{
    std::string response;
    std::string header = "Authorization: Bearer ";
    long http_code = -1;
    header.append(auth_token);
    auto* list = curl_slist_append(nullptr, header.c_str());

    CURL* curl = curl_easy_init();
    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, list);
    if (put) {
        curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "PUT");
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, util::write_callback);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
    } else {
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, util::write_callback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
        curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, header_callback);
        curl_easy_setopt(curl, CURLOPT_HEADERDATA, &response_header);
    }

    if (!response_header.empty())
        bdebug("Response header: %s", response_header.c_str());

#ifdef DEBUG
    curl_easy_setopt(curl, CURLOPT_VERBOSE, CURL_DEBUG);
#endif
    CURLcode res = curl_easy_perform(curl);

    if (res == CURLE_OK) {
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
        QJsonParseError err;

        response_json = QJsonDocument::fromJson(response.c_str(), &err);
        if (response_json.isNull() && !response.empty())
            berr("Failed to parse json response: %s, Error: %s", response.c_str(), qt_to_utf8(err.errorString()));
    } else {
        berr("CURL failed while sending spotify command");
    }

    curl_slist_free_all(list);
    curl_easy_cleanup(curl);
    return http_code;
}
