/*************************************************************************
 * This file is part of tuna
 * git.vrsal.xyz/alex/tuna
 * Copyright 2022 univrsal <uni@vrsal.xyz>.
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

#include "web_server.hpp"
#include "config.hpp"
#include "tuna_thread.hpp"
#include "utility.hpp"
#include <QDateTime>
#include <QJsonDocument>
#include <QJsonObject>
#include <ctime>
extern "C" {
#include <mongoose.h>
}
#include <util/platform.h>

namespace web_thread {

std::thread thread_handle;
std::mutex current_song_mutex;
song current_song;
std::atomic<bool> thread_flag {};

struct mg_mgr mgr;
struct mg_connection* nc;

/* GET requests will result in song information */
static inline void handle_info_get(struct mg_connection* nc)
{
    /* Write current song to json
     * and properly convert it to utf8
     */
    QJsonObject obj;
    QJsonDocument doc;
    QString json;

    tuna_thread::copy_mutex.lock();
    tuna_thread::copy.to_json(obj);
    tuna_thread::copy_mutex.unlock();

    doc.setObject(obj);
    json = QString(doc.toJson(QJsonDocument::Indented));
    std::wstring wstr = json.toStdWString();
    std::string str;

    size_t len = os_wcs_to_utf8(wstr.c_str(), 0, nullptr, 0);
    str.resize(len);
    os_wcs_to_utf8(wstr.c_str(), 0, &str[0], len + 1);

    /* Send basic http response with json */
    mg_printf(nc,
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: application/json; charset=utf-8\r\n"
        "Content-Length: %i\r\n"
        "Connection: close\r\n"
        "Cache-Control: no-store\r\n"
        "Content-Language: en-US\r\n"
        "Server: tuna/%s\r\n"
        "Access-Control-Allow-Origin: *\r\n"
        "\r\n"
        "%s",
        int(len), TUNA_VERSION, str.c_str());
}

static inline void handle_cover_get(struct mg_connection* nc, struct mg_http_message* msg)
{
    struct mg_http_serve_opts opts {
    };
    opts.mime_types = "png=image/png";
    opts.extra_headers = "Cache-Control: no-store\r\n";
    mg_http_serve_file(nc, msg, qt_to_utf8(config::cover_path), &opts);
}

/* POST means we're getting information */
static void handle_post(struct mg_connection* nc, struct mg_http_message* msg)
{
    /* Parse POST data JSON */
    QByteArray arr = QByteArray(msg->body.ptr, msg->body.len);
    QJsonParseError err {};
    QJsonDocument doc = QJsonDocument::fromJson(arr, &err);

    if (err.error == QJsonParseError::NoError && doc.isObject()) {
        auto data = doc.object()["data"];

        if (data.isObject()) {
            std::lock_guard<std::mutex> lock(current_song_mutex);
            current_song.from_json(data.toObject());
        }
    } else {
        bwarn("Error while parsing JSON received via POST: %s", qt_to_utf8(err.errorString()));
        bwarn("JSON: %s", msg->body.ptr);
        mg_printf(nc,
            "HTTP/1.1 400 Bad request\r\n"
            "Connection: close\r\n"
            "Server: tuna/%s\r\n"
            "\r\n",
            TUNA_VERSION);
        return;
    }

    /* Simple OK reponse with mirror of received data */
    mg_printf(nc,
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: text/plain; charset=utf-8\r\n"
        "Content-Length: %i\r\n"
        "Connection: close\r\n"
        "Cache-Control: no-store\r\n"
        "Content-Language: en-US\r\n"
        "Server: tuna/%s\r\n"
        "Access-Control-Allow-Origin: *\r\n"
        "\r\n"
        "%s",
        int(msg->body.len), TUNA_VERSION, msg->body.ptr);
}

static inline void handle_options(struct mg_connection* nc)
{
    /* UTC time */
    time_t now = time(nullptr);
    char date[100];
    strftime(date, sizeof(date), "%d, %b %Y %H:%M:%S GMT", gmtime(&now));

    /* Confirm that we allow post */
    mg_printf(nc,
        "HTTP/1.1 204 No Content\r\n"
        "Cache-Control: max-age=604800\r\n"
        "Access-Control-Allow-Origin: *\r\n"
        "Access-Control-Allow-Methods: POST\r\n"
        "Access-Control-Allow-Headers: *\r\n"
        "Access-Control-Max-Age: 84600\r\n"
        "Date: %s\r\n"
        "Server: tuna/%s\r\n"
        "\r\n",
        date, TUNA_VERSION);
}

static void event_handler(struct mg_connection* nc, int ev, void* d, void* priv)
{
    if (ev == MG_EV_HTTP_MSG) {
        auto* incoming = reinterpret_cast<struct mg_http_message*>(d);
        QString method = utf8_to_qt(incoming->method.ptr);
        QString uri = utf8_to_qt(incoming->uri.ptr).split(" ")[0];
        if (method.startsWith("GET")) {
            if (uri.startsWith("/cover.png"))
                handle_cover_get(nc, incoming);
            else
                handle_info_get(nc);
        } else if (method.startsWith("POST"))
            handle_post(nc, incoming);
        else if (method.startsWith("OPTIONS"))
            handle_options(nc);
    }
}

bool start()
{
    if (thread_flag)
        return true;
    thread_flag = true;

    bool result = true;
    auto port = CGET_STR(CFG_SERVER_PORT);
    std::string url = "http://localhost:";
    url = url.append(port);
    binfo("Starting web server on %s", port);
    mg_log_set_callback([](const void* buf, size_t, void*) { binfo("mongoose: %s", (char*)buf); }, nullptr);
    mg_mgr_init(&mgr);
    nc = mg_http_listen(&mgr, url.c_str(), event_handler, NULL);

    if (!nc) {
        berr("Failed to start listener");
        return false;
    }

    thread_handle = std::thread(thread_method);
    result = thread_handle.native_handle();

    thread_flag = result;
    return result;
}

void stop()
{
    if (!thread_flag)
        return;
    auto port = CGET_STR(CFG_SERVER_PORT);
    binfo("Stopping web server running on %s", port);

    thread_flag = false;
    thread_handle.join();
    mg_mgr_free(&mgr);
}

void thread_method()
{
    util::set_thread_name("tuna-webserver");

    for (;;) {
        mg_mgr_poll(&mgr, 500);
        if (!thread_flag)
            break;
    }
    binfo("Web server thread stopped");
}

}
