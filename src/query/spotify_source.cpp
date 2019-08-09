/**
 * This file is part of tuna
 * which is licensed under the GPL v2.0
 * See LICENSE or http://www.gnu.org/licenses
 * github.com/univrsal/tuna
 */
#include <string>
#include <QString>
#include <util/config-file.h>
#include <util/platform.h>
#include <curl/curl.h>
#include <jansson.h>
#include "spotify_source.hpp"
#include "../util/creds.hpp"
#include "../util/constants.hpp"
#include "../util/config.hpp"
#include "../gui/tuna_gui.hpp"

#define TOKEN_URL		"https://accounts.spotify.com/api/token"
#define PLAYER_URL		"https://api.spotify.com/v1/me/player"
#define REDIRECT_URI	"https%3A%2F%2Funivrsal.github.io%2Fauth%2Ftoken"
#define os_gettime_ms()	(static_cast<uint64_t>(os_gettime_ns() / 10e6))
#define valid(s)		(s && strlen(s) > 0)

spotify_source::spotify_source()
{
    /* builds credentials for spotify api */
    QString str = SPOTIFY_CLIENT_ID;
    str.append(":").append(SPOTIFY_CLIENT_SECRET);
    QString str2(str.toUtf8().toBase64());
    m_creds = str2.toStdString();

    m_capabilities = CAP_TITLE | CAP_ARTIST | CAP_ALBUM | CAP_RELEASE
            | CAP_COVER | CAP_LENGTH | CAP_NEXT_SONG | CAP_PREV_SONG
            | CAP_PLAY_PAUSE | CAP_VOLUME_UP | CAP_VOLUME_DOWN
            | CAP_VOLUME_MUTE | CAP_PREV_SONG | CAP_STATUS;
}

void spotify_source::load()
{
    config_set_default_bool(config::instance, CFG_REGION,
                            CFG_SPOTIFY_LOGGEDIN, false);
    config_set_default_string(config::instance, CFG_REGION,
                              CFG_SPOTIFY_TOKEN, "");
    config_set_default_string(config::instance, CFG_REGION,
                              CFG_SPOTIFY_AUTH_CODE, "");
    config_set_default_uint(config::instance, CFG_REGION,
                            CFG_SPOTIFY_TOKEN_TERMINATION, 0);
    config_set_default_string(config::instance, CFG_REGION,
                              CFG_SPOTIFY_REFRESH_TOKEN, "");

    m_logged_in = config_get_bool(config::instance, CFG_REGION,
                                  CFG_SPOTIFY_LOGGEDIN);
    m_token = config_get_string(config::instance, CFG_REGION,
                                CFG_SPOTIFY_TOKEN);
    m_refresh_token = config_get_string(config::instance, CFG_REGION,
                                CFG_SPOTIFY_REFRESH_TOKEN);
    m_auth_code = config_get_string(config::instance, CFG_REGION,
                                    CFG_SPOTIFY_AUTH_CODE);
    m_token_termination = config_get_uint(config::instance, CFG_REGION,
                                   CFG_SPOTIFY_TOKEN_TERMINATION);

    /* Token handling */
    if (m_logged_in) {
        if (os_gettime_ms() > m_token_termination) {
            QString log;
            bool result = do_refresh_token(log);
            tuna_dialog->apply_login_state(result, log);
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
    config_set_bool(config::instance, CFG_REGION, CFG_SPOTIFY_LOGGEDIN,
                    m_logged_in);
    config_set_string(config::instance, CFG_REGION, CFG_SPOTIFY_TOKEN,
                      m_token.c_str());
    config_set_string(config::instance, CFG_REGION, CFG_SPOTIFY_AUTH_CODE,
                      m_auth_code.c_str());
    config_set_string(config::instance, CFG_REGION, CFG_SPOTIFY_REFRESH_TOKEN,
                      m_refresh_token.c_str());
    config_set_uint(config::instance, CFG_REGION, CFG_SPOTIFY_TOKEN_TERMINATION,
                    m_token_termination);
}

/* implementation further down */
json_t* execute_command(const char* auth_token, const char* url);

void spotify_source::refresh()
{
    if (!m_logged_in)
        return;
    if (os_gettime_ms() > m_token_termination) {
        QString log;
        bool result = do_refresh_token(log);
        tuna_dialog->apply_login_state(result, log);
        save();
    }

    json_t* song_info = execute_command(m_token.c_str(), PLAYER_URL);

    if (song_info) {
        json_t* progress = json_object_get(song_info, "progress_ms");
        json_t* device = json_object_get(song_info, "device");

        if (device && progress) {
            json_t* is_private = json_object_get(device, "is_private_session");
            if (is_private && json_integer_value(is_private)) {
                blog(LOG_ERROR, "[tuna] Spotify session is private! Can't read track");
            } else {
                json_t* track = json_object_get(song_info, "item");
                if (track) {
                    parse_track_json(track);
                } else {
                    blog(LOG_ERROR, "[tuna] Couldn't get spotify track json");
                }
            }
            m_current.progress_ms = json_integer_value(progress);
        }
        json_decref(song_info);
    }
}

void spotify_source::parse_track_json(json_t* track)
{
    json_t* album = json_object_get(track, "album");
    json_t* artists = json_object_get(track, "artists");
    size_t index;
    json_t* curr, *name;
    if (album && artists) {
        /* Get All artists */
        m_current.artists.clear();
        m_current.data = 0x0;
        json_array_foreach(artists, index, curr) {
            name = json_object_get(curr, "name");
            m_current.artists.append(json_string_value(name));
            m_current.artists.append(", ");
        }

        /* Remove last ', ' */
        m_current.artists.pop_back();
        m_current.artists.pop_back();

        /* Cover link */
        curr = json_object_get(album, "images");
        if (curr) {
            curr = json_array_get(curr, 0);
            if (curr) curr = json_object_get(curr, "url");
            if (curr) {
                m_current.cover = json_string_value(curr);
                m_current.data |= CAP_COVER;
            }
        }
        /* Get title */
        name = json_object_get(track, "name");
        if (name) {
            m_current.title = json_string_value(name);
            m_current.data |= CAP_TITLE;
        }

        /* Get length */
        curr = json_object_get(track, "duration_ms");
        if (curr) {
            m_current.duration_ms = json_integer_value(curr);
            m_current.data |= CAP_LENGTH;
        }

        /* Album name */
        curr = json_object_get(album, "name");
        if (curr) {
            m_current.album = json_string_value(curr);
            m_current.data |= CAP_ALBUM;
        }

        /* Explicit ?*/
        curr = json_object_get(track, "explicit");
        if (curr) {
            m_current.is_explicit = json_integer_value(curr);
            m_current.data |= CAP_EXPLICIT;
        }

        /* Disc number */
        curr = json_object_get(track, "disc_number");
        if (curr) {
            m_current.disc_number = json_integer_value(curr);
            m_current.data |= CAP_DISC_NUMBER;
        }

        /* Track number */
        curr = json_object_get(track, "track_number");
        if (curr) {
            m_current.track_number = json_integer_value(curr);
            m_current.data |= CAP_TRACK_NUMBER;
        }

        /* Release date */
        curr = json_object_get(album, "release_date");
        if (curr) {
            m_current.data |= CAP_RELEASE;
            QString date = json_string_value(curr);
            m_current.release_precision = static_cast<date_precision>(qMin(date.count('-'), 2));
            QStringList list = date.split("-");
            switch (list.length()) {
            case 3:
                m_current.day = list[2].toStdString();
            case 2: /* Fallthrough */
                m_current.month = list[1].toStdString();
            case 1: /* Fallthrough */
                m_current.year = list[0].toStdString();
            default:;
            }
        }
    }
}
bool spotify_source::execute_capability(capability c)
{
    bool result = true;
    switch(c) {
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

size_t write_function(void *ptr, size_t size, size_t nmemb, std::string* str)
{
    size_t new_length = size * nmemb;
    try {
        str->append((char*)ptr, new_length);
    } catch (std::bad_alloc& e) {
        blog(LOG_ERROR, "[tuna] Error reading curl response: %s",
             e.what());
        return 0;
    }
    return new_length;
}

CURL* prepare_curl(struct curl_slist* header, std::string* response, const char* request)
{
    CURL* curl = curl_easy_init();
    curl_easy_setopt(curl, CURLOPT_POST, 1L);
    curl_easy_setopt(curl, CURLOPT_URL, TOKEN_URL);
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, header);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, strlen(request));
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, request);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_function);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, response);
#ifdef DEBUG
    curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
#endif
    return curl;
}

/* Requests an access token via request body
 * over a POST request to spotify */
json_t* request_token(const char* request, const char* credentials)
{
    if (!valid(request) || !valid(credentials)) {
        blog(LOG_ERROR, "[tuna] Cannot request token without valid credentials"
                        " and/or auth code!");
        return nullptr;
    }

    std::string response;

    /* Header text */
    QString header = "Authorization: Basic ";
    header.append(credentials);
    auto* list = curl_slist_append(nullptr, qPrintable(header));

    CURL* curl = prepare_curl(list, &response, request);
    CURLcode res = curl_easy_perform(curl);
    json_t* result = nullptr;
    json_error_t error;
    if (res == CURLE_OK) {
        blog(LOG_INFO, "[tuna] Curl response: %s", response.c_str());
        json_t* response_parsed = json_loads(response.c_str(), 0, &error);
        if (response_parsed) {
            result = response_parsed;
        } else {
            blog(LOG_ERROR, "[tuna] Couldn't parse response to json: %s",
                 error.text);
        }
    } else {
        blog(LOG_ERROR, "[tuna] Curl returned error code %i", res);
    }

    curl_slist_free_all(list);
    curl_easy_cleanup(curl);
    return result;
}

/* Gets a new token using the refresh token */
bool spotify_source::do_refresh_token(QString& log)
{
    static std::string request;
    bool result = true;
    request = "grant_type=refresh_token&refresh_token";
    request.append(m_refresh_token);
    auto* response = request_token(request.c_str(), m_creds.c_str());

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
            m_token_termination = os_gettime_ms() + json_integer_value(expires)
                    * 1000;
            m_logged_in = true;
            save();
        } else {
            blog(LOG_ERROR, "[tuna] Couldn't parse json response");
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
            m_token_termination = os_gettime_ms() + json_integer_value(expires)
                    * 1000;
            result = true;
        } else {
            blog(LOG_ERROR, "[tuna] Couldn't parse json response!");
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
json_t* execute_command(const char* auth_token, const char* url)
{
    std::string response;
    QString header = "Authorization: Bearer ";
    header.append(auth_token);
    auto* list = curl_slist_append(nullptr, qPrintable(header));

    CURL* curl = curl_easy_init();
    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, list);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_function);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
#ifdef DEBUG
    curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
#endif
    CURLcode res = curl_easy_perform(curl);
    json_t* result = nullptr;

    if (res == CURLE_OK) {
        json_error_t error;
        result = json_loads(response.c_str(), 0, &error);
        if (!result) {
            blog(LOG_ERROR, "[tuna] Failed to parse json response");
        }
    } else {
        blog(LOG_ERROR, "[tuna] CURL failed while sending spotify command");
    }
    curl_slist_free_all(list);
    curl_easy_cleanup(curl);
    return result;
}
