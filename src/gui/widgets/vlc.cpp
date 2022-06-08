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

#include "vlc.hpp"
#include "../../util/config.hpp"
#include "../../util/constants.hpp"
#include "../../util/tuna_thread.hpp"
#include "../../util/utility.hpp"
#include "ui_vlc.h"
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMessageBox>
#include <QSpacerItem>
#include <obs-frontend-api.h>

vlc::vlc(QWidget* parent)
    : source_widget(parent)
    , ui(new Ui::vlc)
{
    ui->setupUi(this);
    m_list = new drag_list(this);
    m_list->setDragDropMode(QAbstractItemView::InternalMove);
    auto* spacer = new QSpacerItem(0, 0, QSizePolicy::Maximum, QSizePolicy::Expanding);
    ui->verticalLayout->addWidget(m_list);
    ui->verticalLayout->addItem(spacer);
    connect(ui->cb_scene, qOverload<int>(&QComboBox::currentIndexChanged), this, &vlc::on_scene_changed);
    connect(ui->btn_add_source, &QPushButton::clicked, this, &vlc::on_add_source);
    connect(ui->btn_remove_source, &QPushButton::clicked, this, &vlc::on_remove_source);
}

vlc::~vlc()
{
    delete ui;
}

void vlc::load_settings()
{
    load_vlc_sources();
    build_list();
    ui->btn_refresh_vlc->setEnabled(util::have_vlc_source);
    ui->btn_add_source->setEnabled(util::have_vlc_source);
    ui->btn_remove_source->setEnabled(util::have_vlc_source);
    ui->cb_scene->setEnabled(util::have_vlc_source);
    ui->cb_source->setEnabled(util::have_vlc_source);

    QJsonDocument doc;
    if (!util::open_config(VLC_SCENE_MAPPING, doc)) {
        berr("Failed to load vlc mappings");
        return;
    }

    if (doc.isObject()) {
        m_source_map = doc.object();
    } else {
        berr("Failed to load vlc mappings: Json content must be an object");
        return;
    }
}

void vlc::build_list()
{
    auto sc = get_scene_collection();
    auto scene = ui->cb_scene->currentText();
    auto maps = m_source_map[sc].toObject()[scene].toArray();
    m_list->clear();
    for (const auto& map : maps) {
        if (valid_source_name(map.toString())) {
            m_list->addItem(map.toString());
        }
    }
}

bool vlc::has_mapping(const char* scene, const char* source)
{
    auto map = m_source_map[get_scene_collection()].toObject();
    for (const auto& k : map.keys()) {
        if (k == utf8_to_qt(scene)) {
            for (const auto& mapping : map[k].toArray()) {
                if (mapping == utf8_to_qt(source)) {
                    return true;
                }
            }
        }
    }
    return false;
}

void vlc::rebuild_mapping()
{
    auto sc = get_scene_collection();
    auto map = m_source_map[sc].toObject();
    for (auto& scene_id : map.keys()) {
        auto* scene = obs_get_scene_by_name(qt_to_utf8(scene_id));
        if (scene) {
            QJsonArray arr;
            for (const auto& m : map[scene_id].toArray()) {
                if (valid_source_name(m.toString()))
                    arr.append(m.toString());
            }
            obs_scene_release(scene);
            set_map(scene_id, arr);
        }
    }
}

QJsonArray vlc::get_mappings_for_scene(const char* scene)
{
    auto sc = get_scene_collection();
    if (m_source_map[sc].toObject().contains(utf8_to_qt(scene)))
        return m_source_map[sc].toObject()[utf8_to_qt(scene)].toArray();
    return {};
}

void vlc::save_settings()
{
    if (!util::save_config(VLC_SCENE_MAPPING, QJsonDocument(m_source_map)))
        berr("Failed to save vlc mappings");
}

void vlc::load_vlc_sources()
{
    ui->cb_scene->clear();
    ui->cb_source->clear();

    obs_enum_scenes([](void* data, obs_source_t* src) {
        auto* name = obs_source_get_name(src);
        auto* cb = reinterpret_cast<QComboBox*>(data);
        cb->addItem(utf8_to_qt(name));
        return true;
    },
        ui->cb_scene);

    refresh_sources();
}

void vlc::on_btn_refresh_vlc_clicked()
{
    load_vlc_sources();
}

void vlc::on_scene_changed(int)
{
    refresh_sources();
    m_list->clear();
    auto id = ui->cb_scene->currentText();
    auto map = m_source_map[get_scene_collection()].toObject();
    if (map.contains(id)) {
        auto val = map[id];
        if (val.isArray()) {
            for (const auto& entry : val.toArray()) {
                if (entry.isString())
                    m_list->addItem(new QListWidgetItem(entry.toString()));
            }
        }
    }
}

void vlc::on_remove_source()
{
    m_list->takeItem(m_list->currentRow());
    rebuild_from_list();
    refresh_sources();
}

void vlc::set_map(const QString& scene, const QJsonArray& map)
{
    auto sc = get_scene_collection();
    auto obj = m_source_map[sc].toObject();
    obj[scene] = map;
    m_source_map[sc] = obj;
}

bool vlc::valid_source_name(const QString& str)
{
    auto* src = obs_get_source_by_name(qt_to_utf8(str));
    if (src) {
        obs_source_release(src);
        return true;
    }
    return false;
}

void vlc::rebuild_from_list()
{
    QStringList valid_sources;
    for (int i = 0; i < m_list->count(); i++) {
        auto item = m_list->item(i);

        if (item && valid_source_name(item->text()))
            valid_sources.append(item->text());
    }

    m_list->clear();
    m_list->addItems(valid_sources);
    auto scene = ui->cb_scene->currentText();
    auto sc = get_scene_collection();
    set_map(scene, QJsonArray::fromStringList(valid_sources));
}

QString vlc::get_scene_collection()
{
    auto* name = obs_frontend_get_current_scene_collection();
    auto str = utf8_to_qt(name);
    bfree(name);
    return str;
}

void vlc::on_add_source()
{
    auto src_id = ui->cb_source->currentText();
    auto scene_id = ui->cb_scene->currentText();
    auto sc = get_scene_collection();
    auto* src = obs_get_source_by_name(qt_to_utf8(src_id));
    auto* scene = obs_get_scene_by_name(qt_to_utf8(scene_id));

    if (src && scene) {
        std::lock_guard<std::mutex> lock(tuna_thread::thread_mutex);
        if (m_source_map[sc].toObject().contains(scene_id)) {
            auto arr = m_source_map[sc].toObject()[scene_id].toArray();
            arr.append(src_id);
            set_map(scene_id, arr);
        } else {
            QJsonArray arr;
            arr.append(src_id);
            set_map(scene_id, arr);
        }

        m_list->addItem(src_id);
    } else {
        QMessageBox::information(this, T_ERROR_TITLE, T_VLC_INVALID);
    }
    refresh_sources();
    obs_source_release(src);
    obs_scene_release(scene);
}

obs_scene_t* vlc::get_selected_scene()
{
    return obs_get_scene_by_name(qt_to_utf8(ui->cb_scene->currentText()));
}
struct source_data {
    QComboBox* cb;
    vlc* v;
};

void vlc::refresh_sources()
{
    ui->cb_source->clear();
    auto* scene = get_selected_scene();
    source_data d = { ui->cb_source, this };
    if (scene) {
        obs_scene_enum_items(
            scene, [](obs_scene_t* s, obs_sceneitem_t* item, void* data) {
                auto* src = obs_sceneitem_get_source(item);
                auto* scene = obs_scene_get_source(s);
                if (src && scene) {
                    auto* id = obs_source_get_id(src);
                    if (strcmp(id, "vlc_source") == 0) {
                        auto* scene_name = obs_source_get_name(scene);
                        auto* src_name = obs_source_get_name(src);
                        auto* d = reinterpret_cast<source_data*>(data);
                        if (!d->v->has_mapping(scene_name, src_name))
                            d->cb->addItem(utf8_to_qt(src_name));
                    }
                }

                return true;
            },
            &d);
        obs_scene_release(scene);
    }
}
