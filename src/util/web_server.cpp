/*************************************************************************
 * This file is part of tuna
 * github.com/univrsal/tuna
 * Copyright 2020 univrsal <uni@vrsal.cf>.
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
#include <QJsonValue>
#include <mongoose.h>
#include <util/platform.h>

namespace web_thread {

volatile bool thread_flag = false;
std::mutex thread_mutex;
std::thread thread_handle;

struct mg_mgr mgr;
struct mg_connection* nc;
static struct mg_serve_http_opts opts;

static void event_handler(struct mg_connection* nc, int ev, void* p)
{
    if (ev == MG_EV_HTTP_REQUEST) {
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
            "\r\n"
            "%s",
            len, TUNA_VERSION, str.c_str());
    }
}

bool start()
{
    if (thread_flag)
        return true;
    thread_flag = true;

    bool result = true;
    auto port = CGET_STR(CFG_SERVER_PORT);

    binfo("Starting web server on %s", port);
    mg_mgr_init(&mgr, NULL);
    nc = mg_bind(&mgr, port, event_handler);

    if (!nc) {
        berr("Failed to start listener");
        result = false;
    }

    mg_set_protocol_http_websocket(nc);
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

    thread_mutex.lock();
    thread_flag = false;
    thread_mutex.unlock();
    thread_handle.join();
    mg_mgr_free(&mgr);
}

void thread_method()
{
    util::set_thread_name("tuna-webserver");

    for (;;) {
        mg_mgr_poll(&mgr, 500);
        std::lock_guard<std::mutex> lock(thread_mutex);
        if (!thread_flag)
            break;
    }
}

}
