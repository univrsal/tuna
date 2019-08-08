/**
 * This file is part of tuna
 * which is licensed under the GPL v2.0
 * See LICENSE or http://www.gnu.org/licenses
 * github.com/univrsal/tuna
 */
#include <string>
#include <QString>
#include <util/config-file.h>
#include <curl/curl.h>
#include <jansson.h>
#include <util/platform.h>
#include "spotify_source.hpp"
#include "../util/creds.hpp"
#include "../util/constants.hpp"
#include "../util/config.hpp"
#include "../gui/tuna_gui.hpp"

#define TOKEN_URL		"https://accounts.spotify.com/api/token"
#define REDIRECT_URI	"https%3A%2F%2Funivrsal.github.io%2Fauth%2Ftoken"

#define valid(s)		(s && strlen(s) > 0)

spotify_source::spotify_source()
{
    QString str = SPOTIFY_CLIENT_ID;
    str.append(":").append(SPOTIFY_CLIENT_SECRET);
    QString str2(str.toUtf8().toBase64());
    m_creds = qPrintable(str2);
    m_capabilities = CAP_TITLE | CAP_ARTIST | CAP_ALBUM | CAP_RELEASE
            | CAP_COVER | CAP_LENGTH | CAP_NEXT_SONG | CAP_PREV_SONG
            | CAP_PLAY_PAUSE | CAP_VOLUME_UP | CAP_VOLUME_DOWN
            | CAP_VOLUME_MUTE | CAP_PREV_SONG | CAP_STATUS;
}

void spotify_source::load()
{
    config_set_default_bool(config::instance, CFG_REGION,
                            CFG_SPOTIFY_ENABLED, true);
    config_set_default_bool(config::instance, CFG_REGION,
                            CFG_SPOTIFY_LOGGEDIN, false);
    config_set_default_string(config::instance, CFG_REGION,
                              CFG_SPOTIFY_TOKEN, "");
    config_set_default_string(config::instance, CFG_REGION,
                              CFG_SPOTIFY_AUTH_CODE, "");
    config_set_default_uint(config::instance, CFG_REGION,
                            CFG_SPOTIFY_TOKEN_TERMINATION, 0);

    m_enabled = config_get_bool(config::instance, CFG_REGION,
                                CFG_SPOTIFY_ENABLED);
    m_logged_in = config_get_bool(config::instance, CFG_REGION,
                                  CFG_SPOTIFY_LOGGEDIN);
    m_token = config_get_string(config::instance, CFG_REGION,
                                CFG_SPOTIFY_TOKEN);
    m_auth_code = config_get_string(config::instance, CFG_REGION,
                                    CFG_SPOTIFY_AUTH_CODE);
    m_token_termination = config_get_uint(config::instance, CFG_REGION,
                                   CFG_SPOTIFY_TOKEN_TERMINATION);

    if (!m_enabled)
        return;

    /* Token handling */
    if (m_logged_in) {
        if (os_gettime_ns() > m_token_termination)
            do_refresh_token();
    }
}

void spotify_source::refresh()
{
    if (m_logged_in) {
        if (os_gettime_ns() > m_token_termination)
            do_refresh_token();
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

/* Requests an access token via an auth code
 * over a POST request to spotify */
bool spotify_source::request_token(const char* grant_type, const char* code_id,
                                   const char* code)
{
    if (!valid(grant_type) || !valid(code) || !valid(code_id)
            || m_creds.length() < 1) {
        blog(LOG_ERROR, "[tuna] Cannot request token without valid credentials"
                        " and/or auth code!");
        return false;
    }

    CURL* curl = curl_easy_init();
    std::string response;

    /* Header text */
    QString request = "Authorization: Basic ";
    request.append(m_creds);
    auto* list = curl_slist_append(nullptr, qPrintable(request));

    /* Request body */
    json_error_t error;
    json_t* body = json_pack_ex(&error, 0, "{ssssss}",
                             "grand_type", grant_type,
                             code_id, code,
                             "redirect_uri", REDIRECT_URI);

    if (!body) {
        blog(LOG_ERROR, "Error while packing json request body: %s",
             error.text);
        curl_easy_cleanup(curl);
        json_decref(body);
        return false;
    }
    char* json_text = json_dumps(body, 0);

    curl_easy_setopt(curl, CURLOPT_POST, 1L);
    curl_easy_setopt(curl, CURLOPT_URL, TOKEN_URL);
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, list);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, strlen(json_text));
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, json_text);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_function);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
#ifdef DEBUG
    curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
#endif

    CURLcode res = curl_easy_perform(curl);
    bool result = true;
    if (res == CURLE_OK) {
        blog(LOG_INFO, "[tuna] Curl response: %s", response.c_str());
        json_t* response_parsed = json_loads(response.c_str(), 0, &error);
        if (response_parsed) {
            json_t* expires_obj = json_object_get(response_parsed, "expires_in");
            json_t* access_token = json_object_get(response_parsed, "access_token");
            json_t* refresh_token = json_object_get(response_parsed, "refresh_token");

            if (expires_obj && access_token && refresh_token) {
                m_token_termination = os_gettime_ns() / 10e6 +
                        (json_integer_value(expires_obj) * 10e3);
                m_token = json_string_value(access_token);
                m_refresh_token = json_string_value(refresh_token);
            } else {
                blog(LOG_ERROR, "Couldn't recieve data from json response");
                result = false;
            }
        } else {
            blog(LOG_ERROR, "[tuna] Couldn't parse response to json: %s",
                 error.text);
            result = false;
        }
    } else {
        blog(LOG_ERROR, "[tuna] Curl returned error code %i", res);
        result = false;
    }

    free(json_text);
    json_decref(body);
    curl_slist_free_all(list);
    return result;
}

/* Gets a new token using the refresh token */
bool spotify_source::do_refresh_token()
{
    return request_token("refresh_token", "refresh_token", qPrintable(m_refresh_token));
}

/* Gets the first token from the access code */
bool spotify_source::new_token()
{
    return request_token("authorization", "code", qPrintable(m_auth_code));
}
