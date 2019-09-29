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
#include <curl/curl.h>
#include <jansson.h>
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

	m_capabilities = CAP_TITLE | CAP_ARTIST | CAP_ALBUM | CAP_RELEASE | CAP_COVER | CAP_LENGTH | CAP_NEXT_SONG |
					 CAP_PREV_SONG | CAP_PLAY_PAUSE | CAP_VOLUME_UP | CAP_VOLUME_DOWN | CAP_VOLUME_MUTE |
					 CAP_PREV_SONG | CAP_STATUS;
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
				blog(LOG_DEBUG, "[tuna] Successfully renewed Spotify token");
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
json_t *execute_command(const char *auth_token, const char *url,
                        std::string &response_header);

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
            blog(LOG_INFO, "[tuna] Waiting for Spotify-API timeout");
            return;
        }
    }

    std::string header = "";
    json_t *song_info = execute_command(m_token.c_str(), PLAYER_URL, header);
    json_t *err = nullptr;
    if ((err = json_object_get(song_info, "error"))) {
        json_t *error_code = json_object_get(err, "status");
        int code = -1;
        if (error_code)
            code = json_integer_value(error_code);

        if (code == STATUS_RETRY_AFTER && !header.empty()) {
            extract_timeout(header, m_timeout_length);
            if (m_timeout_length) {
                blog(LOG_WARNING, "[tuna] Spotify-API Rate limit hit, waiting %s seconds\n", m_timeout_length);
                m_timeout_length *= SECOND_TO_NS;
                m_timout_start = os_gettime_ns();
            }
        }
	} else if (song_info) {
		json_t *progress = json_object_get(song_info, "progress_ms");
		json_t *device = json_object_get(song_info, "device");
		json_t *playing = json_object_get(song_info, "is_playing");

		if (device && progress && playing) {
			json_t *is_private = json_object_get(device, "is_private_session");
			if (is_private && json_true() == is_private) {
				blog(LOG_ERROR, "[tuna] Spotify session is private! Can't read track");
			} else {
				json_t *track = json_object_get(song_info, "item");
				if (track) {
					parse_track_json(track);
					m_current.is_playing = json_true() == playing;
				} else {
					blog(LOG_ERROR, "[tuna] Couldn't get spotify track json");
				}
			}
			m_current.progress_ms = json_integer_value(progress);
		} else {
			char *json_str = json_dumps(song_info, 0);
			blog(LOG_ERROR, "[tuna] Couldn't fetch song data from spotify json: %s", json_str);
			free(json_str);
		}
		json_decref(song_info);
	}
}

void spotify_source::parse_track_json(json_t *track)
{
	json_t *album = json_object_get(track, "album");
	json_t *artists = json_object_get(track, "artists");
	size_t index;
	json_t *curr, *name;
	if (album && artists) {
		m_current = {};
		m_current.release_precision = prec_unkown;

		/* Get All artists */
		json_array_foreach (artists, index, curr) {
			name = json_object_get(curr, "name");
			m_current.artists.append(json_string_value(name));
			m_current.artists.append(", ");
		}

		/* Remove last ', ' */
		m_current.artists.pop_back();
		m_current.artists.pop_back();
		if (!m_current.artists.empty())
			m_current.data |= CAP_ARTIST;

		/* Cover link */
		curr = json_object_get(album, "images");
		if (curr) {
			curr = json_array_get(curr, 0);
			if (curr)
				curr = json_object_get(curr, "url");
			if (curr) {
				m_current.cover = json_string_value(curr);
				if (!m_current.cover.empty())
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
				/* Sometimes the release date gets reported wrong, so adjust it
           * if a value wasn't found */
				if (m_current.day.empty())
					m_current.release_precision = prec_month;
			case 2: /* Fallthrough */
				m_current.month = list[1].toStdString();
				if (m_current.month.empty())
					m_current.release_precision = prec_year;
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

size_t write_callback(char *ptr, size_t size, size_t nmemb, std::string *str)
{
	size_t new_length = size * nmemb;
	try {
		str->append(ptr, new_length);
	} catch (std::bad_alloc &e) {
		blog(LOG_ERROR, "[tuna] Error reading curl response: %s", e.what());
		return 0;
	}
	return new_length;
}

size_t header_callback(char *ptr, size_t size, size_t nmemb, std::string *str)
{
    size_t new_length = size * nmemb;
    try {
        str->append(ptr, new_length);
    } catch (std::bad_alloc &e) {
        blog(LOG_ERROR, "[tuna] Error reading curl header: %s", e.what());
        return 0;
    }
    return new_length;
}

CURL *prepare_curl(struct curl_slist *header, std::string *response, std::string *response_header,
    const char *request)
{
	CURL *curl = curl_easy_init();
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
json_t *request_token(const char *request, const char *credentials)
{
	if (!valid(request) || !valid(credentials)) {
		blog(LOG_ERROR, "[tuna] Cannot request token without valid credentials"
						" and/or auth code!");
		return nullptr;
	}

	std::string response;
    std::string response_header;

	/* Header text */
	QString header = "Authorization: Basic ";
	header.append(credentials);
	auto *list = curl_slist_append(nullptr, qPrintable(header));

    CURL *curl = prepare_curl(list, &response, &response_header, request);
	CURLcode res = curl_easy_perform(curl);
	json_t *result = nullptr;
	json_error_t error;
	if (res == CURLE_OK) {
		blog(LOG_INFO, "[tuna] Curl response: %s", response.c_str());
		json_t *response_parsed = json_loads(response.c_str(), 0, &error);
		if (response_parsed) {
			result = response_parsed;
			/* Log response without tokens */
			json_t *dup = json_deep_copy(response_parsed);
			json_object_set(dup, "access_token", json_string("REDACTED"));
			json_object_set(dup, "refresh_token", json_string("REDACTED"));
			char *str = json_dumps(dup, 0);
			blog(LOG_INFO, "[tuna] Spotify response: %s", str);
			free(str);
			json_decref(dup);
		} else {
			blog(LOG_ERROR, "[tuna] Couldn't parse response to json: %s",
				 strlen(error.text) > 0 ? error.text : "Response was empty");
		}
	} else {
		blog(LOG_ERROR, "[tuna] Curl returned error code %i", res);
	}

	curl_slist_free_all(list);
	curl_easy_cleanup(curl);
	return result;
}

/* Gets a new token using the refresh token */
bool spotify_source::do_refresh_token(QString &log)
{
	static std::string request;
	bool result = true;
	request = "grant_type=refresh_token&refresh_token=";
	request.append(m_refresh_token);
	auto *response = request_token(request.c_str(), m_creds.c_str());

	if (response) {
		json_t *token = json_object_get(response, "access_token");
		json_t *expires = json_object_get(response, "expires_in");
		json_t *refresh_token = json_object_get(response, "refresh_token");

		/* Dump the json into the log textbox */
		const char *json_pretty = json_dumps(response, JSON_INDENT(4));
		log = json_pretty;
		free((void *)json_pretty);

		if (token && expires) {
			m_token = json_string_value(token);
			m_token_termination = util::epoch() + json_integer_value(expires);
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
bool spotify_source::new_token(QString &log)
{
	static std::string request;
	bool result = true;
	request = "grant_type=authorization_code&code=";
	request.append(m_auth_code).append("&redirect_uri=").append(REDIRECT_URI);
	auto *response = request_token(request.c_str(), m_creds.c_str());

	if (response) {
		json_t *token = json_object_get(response, "access_token");
		json_t *refresh = json_object_get(response, "refresh_token");
		json_t *expires = json_object_get(response, "expires_in");
		/* Dump the json into the log textbox */
		const char *json_pretty = json_dumps(response, JSON_INDENT(4));
		log = json_pretty;
		free((void *)json_pretty);

		if (token && refresh && expires) {
			m_token = json_string_value(token);
			m_refresh_token = json_string_value(refresh);
			m_token_termination = util::epoch() + json_integer_value(expires);
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
json_t *execute_command(const char *auth_token, const char *url, std::string &response_header)
{
    std::string response;
	QString header = "Authorization: Bearer ";
	header.append(auth_token);
	auto *list = curl_slist_append(nullptr, qPrintable(header));

	CURL *curl = curl_easy_init();
	curl_easy_setopt(curl, CURLOPT_URL, url);
	curl_easy_setopt(curl, CURLOPT_HTTPHEADER, list);
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
    curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, header_callback);
    curl_easy_setopt(curl, CURLOPT_HEADERDATA, &response_header);
    if (!response_header.empty())
        blog(LOG_DEBUG, "[tuna] Response header: %s", response_header.c_str());

#ifdef DEBUG
	curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
#endif
    CURLcode res = curl_easy_perform(curl);
	json_t *result = nullptr;

	if (res == CURLE_OK) {
		json_error_t error;
        result = json_loads(response.c_str(), 0, &error);
		if (!result) {
			blog(LOG_ERROR, "[tuna] Failed to parse json response: %s", response.c_str());
		}
	} else {
		blog(LOG_ERROR, "[tuna] CURL failed while sending spotify command");
    }

	curl_slist_free_all(list);
	curl_easy_cleanup(curl);
	return result;
}
