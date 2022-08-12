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

#include "music_source.hpp"
#include "../gui/music_control.hpp"
#include "../gui/tuna_gui.hpp"
#include "../util/config.hpp"
#include "../util/format.hpp"
#include "../util/tuna_thread.hpp"
#include "../util/utility.hpp"
#include "gpmdp_source.hpp"
#include "icecast_source.hpp"
#include "lastfm_source.hpp"
#include "mpd_source.hpp"
#include "spotify_source.hpp"
#include "vlc_obs_source.hpp"
#include "web_source.hpp"
#include "window_source.hpp"
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QRegularExpression>
#include <obs-frontend-api.h>

namespace music_sources {
static int selected_index = -1;
QList<std::shared_ptr<music_source>> instances;

void init()
{
    obs_frontend_push_ui_translation(obs_module_get_string);
    instances.append(std::make_shared<spotify_source>());
    instances.append(std::make_shared<mpd_source>());
    instances.append(std::make_shared<vlc_obs_source>());
    instances.append(std::make_shared<window_source>());
    instances.append(std::make_shared<lastfm_source>());
    instances.append(std::make_shared<gpmdp_source>());
    instances.append(std::make_shared<web_source>());
    instances.append(std::make_shared<icecast_source>());
    obs_frontend_pop_ui_translation();

    for (auto& s : instances) {
        s->load();
        tuna_dialog->add_source(utf8_to_qt(s->name()), utf8_to_qt(s->id()), s->get_settings_tab());
        if (music_dock)
            music_dock->add_source(utf8_to_qt(s->name()), utf8_to_qt(s->id()));
    }

    const auto s = config_get_string(obs_frontend_get_global_config(), CFG_REGION, CFG_SELECTED_SOURCE);
    auto i = 0;
    for (const auto& src : qAsConst(music_sources::instances)) {
        if (strcmp(src->id(), s) == 0)
            break;
        i++;
    }
    tuna_dialog->select_source(i);
    if (music_dock)
        music_dock->select_source(i);
}

void load()
{
    for (auto& src : instances)
        src->load();
}

void save()
{
    for (auto& src : instances)
        src->save();
}

void select(const char* id)
{
    if (!id)
        return;

    auto selected = selected_source();
    if (selected && strcmp(selected->id(), id) == 0)
        return;

    tuna_thread::thread_mutex.lock();
    if (selected)
        selected->reset_info();
    int i = 0;
    for (const auto& src : qAsConst(instances)) {
        if (strcmp(src->id(), id) == 0) {
            selected_index = i;
            break;
        }
        i++;
    }

    /* Ensure that cover is set to place holder on switch */
    util::reset_cover();
    tuna_thread::thread_mutex.unlock();
}

void set_gui_values()
{
    for (const auto& src : qAsConst(instances))
        src->set_gui_values();
}

std::shared_ptr<music_source> selected_source_unsafe()
{
    if (selected_index >= 0) {
        auto ref = std::shared_ptr<music_source>(instances[selected_index]);
        return ref;
    }
    return nullptr;
}

std::shared_ptr<music_source> selected_source()
{
    if (selected_index >= 0) {
        std::lock_guard<std::mutex> lock(tuna_thread::thread_mutex);
        return std::shared_ptr<music_source>(instances[selected_index]);
    }
    return nullptr;
}

void deinit()
{
    /* check if all source references were decreased correctly */
    for (int i = 0; i < instances.count(); i++) {
        if (instances[i].use_count() > 1) {
            berr("Shared pointer of source %s is still in use!"
                 " (use count: %i)",
                instances[i]->id(), int(instances[i].use_count()));
        }
    }
    instances.clear();
}
}

bool music_source::download_missing_cover()
{
    static const QString request = "https://itunes.apple.com/search?term={}&media=music&entity=album"; // should we also look for singles?
    if (config::download_missing_cover && m_current.has_cover_lookup_information()) {
        auto search_term = QUrl::toPercentEncoding(m_current.get<QStringList>(meta::ARTIST)[0] + " " + m_current.get(meta::ALBUM));
        auto url = request;
        url = url.replace("{}", search_term);
        auto doc = util::curl_get_json(qt_to_utf8(url));
        if (doc["results"].isArray()) {
            auto first = doc["results"].toArray()[0].toObject();

            // We don't want to use the wrong cover so we check if the first (probably also best) search result
            // has a matching title. (We search if the title contains the currently playing title or the other
            // way around in case the titles aren't exactly the same (eg. it has something like a "(Single)"
            // prefix or postfix
            if (!first["collectionName"].toString().toLower().contains(m_current.get(meta::TITLE).toLower()) || m_current.get(meta::TITLE).toLower().contains(first["collectionName"].toString().toLower())) {
                return false;
            }
            if (first["artworkUrl60"].isString()) {
                auto url2 = first["artworkUrl60"].toString();
                url2 = url2.replace("60x60", QString::number(config::cover_size) + "x" + QString::number(config::cover_size));
                return util::download_cover(url2);
            }
        }
    }
    return false;
}

music_source::music_source(const char* id, const char* name, source_widget* w)
    : m_id(id)
    , m_name(name)
    , m_settings_tab(w)
{
}

void music_source::load()
{
    if (m_settings_tab)
        m_settings_tab->load_settings();
}

void music_source::save()
{
    if (m_settings_tab)
        m_settings_tab->save_settings();
}

void music_source::set_gui_values()
{
    if (m_settings_tab)
        m_settings_tab->load_settings();
}

void music_source::handle_cover()
{
    if (m_current == m_prev)
        return;

    if (m_current.get<int>(meta::STATUS) == state_playing) {
        if (!util::download_cover(m_current.get(meta::COVER))) {
            if (!download_missing_cover())
                util::reset_cover();
        }
    } else if (m_current.get<int>(meta::STATUS) != state_paused || config::placeholder_when_paused) {
        /* We either
            - are in a stopped/unknown state                -> reset cover
            - are paused & want a placeholder when paused   -> reset cover
            - do not have a cover                           -> try downloading cover
        */
        if (!m_current.has(meta::COVER))
            download_missing_cover();
        else
            util::reset_cover();
    }
}

bool music_source::valid_format(const QString& str)
{
    QString copy = str;
    return format::execute(copy);
}
