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

#include "gpmdp_source.hpp"
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>

gpmdp_source::gpmdp_source()
    : music_source(S_SOURCE_GPMDP, T_SOURCE_GPMDP)
{
    m_capabilities = CAP_ALBUM | CAP_TITLE | CAP_ARTIST | CAP_STATUS | CAP_DURATION | CAP_PROGRESS | CAP_COVER;
#if _WIN32
    m_path = qgetenv("APPDATA") + "/Google Play Music Desktop Player/json_store/playback.json";
#elif __unix__
    QDir home = QDir::homePath();
    m_path = home.absolutePath() + "/.config/Google Play Music Desktop Player/json_store/playback.json";
#elif __APPLE__
    QDir home = QDir::homePath();
    m_path = home.absolutePath() + "/Library/Application Support/Google Play Music Desktop Player/json_store/playback.json";
#endif
}

void gpmdp_source::refresh()
{
    QFile file(m_path);

    if (file.open(QIODevice::ReadOnly)) {
		begin_refresh();
        m_current.clear();
        auto doc = QJsonDocument::fromJson(file.readAll());
        if (!doc.isObject())
            return;
        auto obj = doc.object();
        auto song = obj["song"].toObject();
        auto time = obj["time"].toObject();

        m_current.set_state(obj["playing"].toBool() ? state_playing : state_stopped);
        m_current.set_title(song["title"].toString());
        m_current.append_artist(song["artist"].toString());
        m_current.set_album(song["album"].toString());
        m_current.set_cover_link(song["albumArt"].toString());

        m_current.set_duration(time["total"].toInt());
        m_current.set_progress(time["current"].toInt());

        util::download_cover(m_current);
        file.close();
    }

    begin_refresh();
}
