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

#include "web_server.hpp"
#include "../plugin-macros.generated.h"
#include "config.hpp"
#include "tuna_thread.hpp"
#include "utility.hpp"
#include <QDateTime>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <ctime>
#include <httplib.h>
#include <sstream>
#include <util/platform.h>

namespace web_thread {

std::thread thread_handle;
std::mutex current_song_mutex;
song current_song;

httplib::Server* server {};

//* GET requests will result in song information */
static inline void handle_info_get(const httplib::Request&, httplib::Response& res)
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

    res.set_header("Access-Control-Allow-Origin", "*");
    res.set_header("Server", "tuna/" PLUGIN_VERSION);
    res.set_header("Content-Type", "application/json; charset=utf-8");
    res.set_header("Connection", "close");
    res.set_header("Cache-Control", "no-store");
    res.set_header("Content-Language", "en-US");
    res.set_content(str.c_str(), "application/json; charset=utf-8");
    res.status = 200;
}

//* POST means we're getting information */
static void handle_post(const httplib::Request& req, httplib::Response& res)
{
    /* Parse POST data JSON */
    auto str = utf8_to_qt(req.body.c_str());
    QJsonParseError err {};
    QJsonDocument doc = QJsonDocument::fromJson(str.toUtf8(), &err);

    if (err.error == QJsonParseError::NoError && doc.isObject()) {
        auto const data = doc.object()["data"];

        if (data.isObject()) {
            auto const obj = data.toObject();
            std::lock_guard<std::mutex> lock(current_song_mutex);
            current_song.from_json(obj);
        }
    } else {
        bwarn("Error while parsing JSON received via POST: %s", qt_to_utf8(err.errorString()));
        bwarn("JSON: %s", req.body.c_str());
        res.set_content(qt_to_utf8(err.errorString()), "text/plain");
        res.set_header("Access-Control-Allow-Origin", "*");
        res.set_header("Server", "tuna/" PLUGIN_VERSION);
        res.status = 500;
        return;
    }

    /* Simple OK reponse */
    res.set_content("200 OK", "text/plain; charset=utf-8");
    res.set_header("Connection", "close");
    res.set_header("Cache-Control", "no-store");
    res.set_header("Access-Control-Allow-Origin", "*");
    res.set_header("Server", "tuna/" PLUGIN_VERSION);
    res.set_header("Connection", "close");
    res.set_header("Cache-Control", "no-store");
    res.set_header("Content-Language", "en-US");
    res.status = 200;
}

bool start()
{
    if (server && server->is_running() && server->is_valid())
        return true;
    stop();
    server = new httplib::Server;

    server->set_logger([](const httplib::Request&, const httplib::Response&) {});
    server->Options("/", [](const httplib::Request&, httplib::Response& res) {
        time_t now = time(nullptr);
        char date[100];
        strftime(date, sizeof(date), "%d, %b %Y %H:%M:%S GMT", gmtime(&now));
        res.set_header("Cache-control", "max-age=604800");
        res.set_header("Access-Control-Allow-Methods", "POST");
        res.set_header("Access-Control-Allow-Headers", "*");
        res.set_header("Access-Control-Max-Age", "84600");
        res.set_header("Access-Control-Allow-Origin", "*");
        res.set_header("Date", date);
        res.set_header("Server", "tuna/" PLUGIN_VERSION);
        res.set_content(date, "text/plain");
    });
    server->Get("/cover.png", [](const httplib::Request&, httplib::Response& res) {
        QFile f(config::cover_path);
        if (f.open(QIODevice::ReadOnly)) {
            auto data = f.readAll();
            res.set_content(data, data.length(), "image/png");
            res.set_header("Access-Control-Allow-Origin", "*");
            res.set_header("Cache-Control", "no-cache");
            res.set_header("Server", "tuna/" PLUGIN_VERSION);
            res.status = 200;
        } else {
            res.set_content("500 Internal Server Error: Couldn't open cover file", "text/plain");
            res.set_header("Server", "tuna/" PLUGIN_VERSION);
            res.status = 500;
        }
    });
    server->Get("/", handle_info_get);
    server->Post("/", handle_post);

    thread_handle = std::thread(thread_method);
    return thread_handle.native_handle();
}

void stop()
{
    if (server) {
        bdebug("Stopping webserver...");
        if (server->is_running() && server->is_valid())
            server->stop();
        thread_handle.join();
        delete server;
        server = nullptr;
        binfo("Stopped webserver");
    }
}

void thread_method()
{
    util::set_thread_name("tuna-webserver");
    auto port = CGET_STR(CFG_SERVER_PORT);
    binfo("Webserver listening on %s", port);
    server->listen("0.0.0.0", atoi(port));
}
}
