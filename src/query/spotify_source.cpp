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

#include "spotify_source.hpp"
#include "../gui/tuna_gui.hpp"
#include "../util/config.hpp"
#include "../util/constants.hpp"
#include "../util/creds.hpp"
#include "../util/utility.hpp"
#include <QString>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonDocument>
#include <curl/curl.h>
#include <string>
#include <util/config-file.h>
#include <util/platform.h>

#define TOKEN_URL "https://accounts.spotify.com/api/token"
#define PLAYER_URL "https://api.spotify.com/v1/me/player"
#define REDIRECT_URI "https%3A%2F%2Funivrsal.github.io%2Fauth%2Ftoken"
#define valid(s) (s && strlen(s) > 0)

spotify_source::spotify_source()
{
    /* builds credentials for spotify api */
    QString str = SPOTIFY_CREDENTIALS;
    QString str2(str.toUtf8().toBase64());
    m_creds = str2.toStdString();

    m_capabilities = CAP_TITLE | CAP_ARTIST | CAP_ALBUM | CAP_RELEASE | CAP_COVER | CAP_LENGTH | CAP_NEXT_SONG | CAP_PREV_SONG | CAP_PLAY_PAUSE | CAP_VOLUME_UP | CAP_VOLUME_DOWN | CAP_VOLUME_MUTE | CAP_PREV_SONG | CAP_STATUS;
}

void spotify_source::load()
{
    CDEF_BOOL(CFG_SPOTIFY_LOGGEDIN, false);
    CDEF_STR(CFG_SPOTIFY_TOKEN, "");
    CDEF_STR(CFG_SPOTIFY_AUTH_CODE, "");
    CDEF_STR(CFG_SPOTIFY_REFRESH_TOKEN, "");
    CDEF_INT(CFG_SPOTIFY_TOKEN_TERMINATION, 0);

    m_logged_in = CGET_BOOL(CFG_SPOTIFY_LOGGEDIN);
    m_token = CGET_STR(CFG_SPOTIFY_TOKEN);
    m_refresh_token = CGET_STR(CFG_SPOTIFY_REFRESH_TOKEN);
    m_auth_code = CGET_STR(CFG_SPOTIFY_AUTH_CODE);
    m_token_termination = CGET_INT(CFG_SPOTIFY_TOKEN_TERMINATION);

    /* Token handling */
    if (m_logged_in) {
        if (util::epoch() > m_token_termination) {
            QString log;
            bool result = do_refresh_token(log);
            if (result) {
                bdebug("Successfully renewed Spotify token");
            }
            save();
        }
    }
}

void spotify_source::load_gui_values()
{
    tuna_dialog->set_spotify_auth_code(m_auth_code.c_str());
    tuna_dialog->set_spotify_auth_token(m_token.c_str());
    tuna_dialog->set_spotify_refresh_token(m_refresh_token.c_str());
}

void spotify_source::save()
{
    CSET_BOOL(CFG_SPOTIFY_LOGGEDIN, m_logged_in);
    CSET_STR(CFG_SPOTIFY_TOKEN, m_token.c_str());
    CSET_STR(CFG_SPOTIFY_AUTH_CODE, m_auth_code.c_str());
    CSET_STR(CFG_SPOTIFY_REFRESH_TOKEN, m_refresh_token.c_str());
    CSET_INT(CFG_SPOTIFY_TOKEN_TERMINATION, m_token_termination);
}

/* implementation further down */
void execute_command(const char* auth_token, const char* url,
    std::string& response_header, QJsonDocument& result);

void extract_timeout(const std::string header, uint64_t& timeout)
{
    static const std::string what = "Retry-After: ";
    timeout = 0;
    size_t pos = header.find(what);
    size_t end;

    if (pos != std::string::npos) {
        pos += what.length();
        end = pos;
        std::string tmp;
        while (header.at(end) != '\n')
            end++;
        tmp = header.substr(pos, end - pos);
        timeout = std::stoi(tmp);
    }
}

void spotify_source::refresh()
{
    if (!m_logged_in)
        return;

    if (util::epoch() > m_token_termination) {
        QString log;
        bool result = do_refresh_token(log);
        tuna_dialog->apply_login_state(result, log);
        save();
    }

    if (m_timout_start) {
        if (os_gettime_ns() - m_timout_start >= m_timeout_length) {
            m_timout_start = 0;
            m_timeout_length = 0;
        } else {
            binfo("Waiting for Spotify-API timeout");
            return;
        }
    }

    std::string header = "";
    QJsonDocument response;
    execute_command(m_token.c_str(), PLAYER_URL, header, response);

    if (response["error"].isNull()) {
        const auto& progress = response["progress_ms"];
        const auto& device = response["device"];
        const auto& playing = response["is_playing"];

        if (!device.isNull() && !playing.isNull() && !playing.isNull()) {
            if (device["is_private"].toBool()) {
                berr("Spotify session is private! Can't read track");
            } else {
                parse_track_json(response["item"]);
                m_current.set_playing(playing.toBool());
            }
            m_current.set_progress(progress.toInt());
        } else {
            QString str(response.toJson());
            berr("Couldn't fetch song data from spotify json: %s", str.toStdString().c_str());
        }
    } else {
        int code = response["status"].toInt(-1);
        if (code == STATUS_RETRY_AFTER && !header.empty()) {
            extract_timeout(header, m_timeout_length);
            if (m_timeout_length) {
                bwarn("Spotify-API Rate limit hit, waiting %s seconds\n", m_timeout_length);
                m_timeout_length *= SECOND_TO_NS;
                m_timout_start = os_gettime_ns();
            }
        }
    }
}

void spotify_source::parse_track_json(const QJsonValue& track)
{
    const auto& album = track["album"];
    const auto& artists = track["artists"];
    size_t index;
    if (!album.isNull() && !artists.isNull()) {
        m_current.clear();

        /* Get All artists */
        for (const auto artist : artists.toArray())
            m_current.append_artist(artist.toString());

        /* Cover link */
        const auto& covers = album["images"];
        if (covers.isArray())
            m_current.set_cover_link(covers.toArray()[0].toString());

        /* Other stuff */
        m_current.set_title(track["name"].toString());
        m_current.set_duration(track["duration_ms"].toInt());
        m_current.set_album(album["name"].toString());
        m_current.set_explicit(track["explicit"].toBool());
        m_current.set_disc_number(track["disc_number"].toInt());
        m_current.set_track_number(track["track_number"].toInt());

        /* Release date */
        const auto& date = album["release_date"].toString();
        if (date.length() > 0) {
            QStringList list = date.split("-");
            switch (list.length()) {
            case 3:
                m_current.set_day(list[2]);
            case 2: /* Fallthrough */
                m_current.set_month(list[1]);
            case 1: /* Fallthrough */
                m_current.set_day(list[0]);
            default:;
            }
        }
    }
}

bool spotify_source::execute_capability(capability c)
{
    bool result = true;
    switch (c) {
    case CAP_NEXT_SONG:
        break;
    case CAP_PREV_SONG:
        break;
    case CAP_PLAY_PAUSE:
        break;
    case CAP_VOLUME_UP:
        break;
    case CAP_VOLUME_DOWN:
        break;
    case CAP_VOLUME_MUTE:
        break;
    default:;
    }
    return result;
}

/* === CURL/Spotify API handling === */

size_t write_callback(char* ptr, size_t size, size_t nmemb, QString* str)
{
    size_t new_length = size * nmemb;
    try {
        str->resize(new_length)
        str->append(ptr);
    } catch (std::bad_alloc& e) {
        berr("Error reading curl response: %s", e.what());
        return 0;
    }
    return new_length;
}

size_t header_callback(char* ptr, size_t size, size_t nmemb, QString* str)
{
    size_t new_length = size * nmemb;
    try {
        str->resize(new_length);
        str->append(ptr);
    } catch (std::bad_alloc& e) {
        berr("Error reading curl header: %s", e.what());
        return 0;
    }
    return new_length;
}

CURL* prepare_curl(struct curl_slist* header, QString* response, QString* response_header,
    const char* request)
{
    CURL* curl = curl_easy_init();
    curl_easy_setopt(curl, CURLOPT_POST, 1L);
    curl_easy_setopt(curl, CURLOPT_URL, TOKEN_URL);
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, header);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, strlen(request));
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, request);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, response);
    curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, header_callback);
    curl_easy_setopt(curl, CURLOPT_HEADERDATA, response_header);
#ifdef DEBUG
    curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
#endif
    return curl;
}

/* Requests an access token via request body
 * over a POST request to spotify */
void request_token(const char* request, const char* credentials, QJsonDocument& response_json)
{
    if (!valid(request) || !valid(credentials)) {
        berr("Cannot request token without valid credentials"
                        " and/or auth code!");
        return;
    }

    QString response, response_header;
    QString header = "Authorization: Basic ";
    header.append(credentials);

    auto* list = curl_slist_append(nullptr, qPrintable(header));
    CURL* curl = prepare_curl(list, &response, &response_header, request);
    CURLcode res = curl_easy_perform(curl);

    if (res == CURLE_OK) {
        QJsonParseError err;
        response_json = QJsonDocument::fromJson(response.toUtf8(), &err);
        if (response_json.isNull()) {
            berr("Couldn't parse response to json: %s",
                err.errorString().toStdString().c_str());
        } else {
            /* Log response without tokens */
            auto obj = response_json.object();
            obj["access_token"] = "REDACTED";
            obj["refresh_token"] = "REDACTED";
            auto doc = QJsonDocument(obj);
            QString str(doc.toJson());
            binfo("Spotify response: %s", str.toStdString().c_str());
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
    static std::string request;
    bool result = true;
    QJsonDocument response;
    request = "grant_type=refresh_token&refresh_token=";
    request.append(m_refresh_token);
    request_token(request.c_str(), m_creds.c_str(), response);

    if (response) {
        json_t* token = json_object_get(response, "access_token");
        json_t* expires = json_object_get(response, "expires_in");
        json_t* refresh_token = json_object_get(response, "refresh_token");

        /* Dump the json into the log textbox */
        const char* json_pretty = json_dumps(response, JSON_INDENT(4));
        log = json_pretty;
        free((void*)json_pretty);

        if (token && expires) {
            m_token = json_string_value(token);
            m_token_termination = util::epoch() + json_integer_value(expires);
            m_logged_in = true;
            save();
        } else {
            berr("Couldn't parse json response");
            result = false;
        }

        /* Refreshing the token can return a new refresh token */
        if (refresh_token) {
            m_refresh_token = json_string_value(refresh_token);
        }
        json_decref(response);
    } else {
        result = false;
    }

    m_logged_in = result;
    return result;
}

/* Gets the first token from the access code */
bool spotify_source::new_token(QString& log)
{
    static std::string request;
    bool result = true;
    request = "grant_type=authorization_code&code=";
    request.append(m_auth_code).append("&redirect_uri=").append(REDIRECT_URI);
    auto* response = request_token(request.c_str(), m_creds.c_str());

    if (response) {
        json_t* token = json_object_get(response, "access_token");
        json_t* refresh = json_object_get(response, "refresh_token");
        json_t* expires = json_object_get(response, "expires_in");
        /* Dump the json into the log textbox */
        const char* json_pretty = json_dumps(response, JSON_INDENT(4));
        log = json_pretty;
        free((void*)json_pretty);

        if (token && refresh && expires) {
            m_token = json_string_value(token);
            m_refresh_token = json_string_value(refresh);
            m_token_termination = util::epoch() + json_integer_value(expires);
            result = true;
        } else {
            berr("Couldn't parse json response!");
            result = false;
        }
        json_decref(response);
    } else {
        result = false;
    }

    m_logged_in = result;
    save();
    return result;
}

/* Sends commands to spotify api via url */
json_t* execute_command(const char* auth_token, const char* url, std::string& response_header)
{
    std::string response;
    QString header = "Authorization: Bearer ";
    header.append(auth_token);
    auto* list = curl_slist_append(nullptr, qPrintable(header));

    CURL* curl = curl_easy_init();
    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, list);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
    curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, header_callback);
    curl_easy_setopt(curl, CURLOPT_HEADERDATA, &response_header);
    if (!response_header.empty())
        bdebug("Response header: %s", response_header.c_str());

#ifdef DEBUG
    curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
#endif
    CURLcode res = curl_easy_perform(curl);
    json_t* result = nullptr;

    if (res == CURLE_OK) {
        json_error_t error;
        result = json_loads(response.c_str(), 0, &error);
        if (!result && !response.empty())
            berr("Failed to parse json response: %s", response.c_str());
    } else {
        berr("CURL failed while sending spotify command");
    }

    curl_slist_free_all(list);
    curl_easy_cleanup(curl);
    return result;
}
