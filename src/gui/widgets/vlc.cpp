/*************************************************************************
 * This file is part of tuna
 * github.com/univrsal/tuna
 * Copyright 2021 univrsal <uni@vrsal.de>.
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
#include <obs-frontend-api.h>

vlc::vlc(QWidget* parent)
    : source_widget(parent)
    , ui(new Ui::vlc)
{
    ui->setupUi(this);
    connect(ui->cb_scene, qOverload<int>(&QComboBox::currentIndexChanged), this, &vlc::on_scene_changed);
    connect(ui->btn_add_source, &QPushButton::clicked, this, &vlc::on_add_source);
    connect(ui->btn_remove_source, &QPushButton::clicked, this, &vlc::on_remove_source);
    connect(ui->source_list, &QListWidget::indexesMoved, this, &vlc::on_reorder_sources);
}

vlc::~vlc()
{
    delete ui;
}

void vlc::load_settings()
{
    load_vlc_sources();
    select_vlc_source(utf8_to_qt(CGET_STR(CFG_VLC_ID)));
    ui->btn_refresh_vlc->setEnabled(util::have_vlc_source);
    ui->cb_vlc_source_name->setEnabled(util::have_vlc_source);

    QJsonDocument doc;
    if (!util::open_config(VLC_SCENE_MAPPING, doc)) {
        berr("Failed to load vlc mappings");
        return;
    }

    if (doc.isObject()) {
        auto test = QString(doc.toJson());
        for (const auto& k : doc.object().keys()) {
            auto map = doc.object()[k].toArray();
            if (!map.isEmpty()) {
                m_source_map[k] = map;
            }
        }
        binfo("%s", qt_to_utf8(test));
    } else {
        berr("Failed to load vlc mappings: must be an object");
        return;
    }
}

void vlc::build_list()
{
    auto scene = ui->cb_scene->currentText();
    auto maps = m_source_map[scene].toArray();
    ui->source_list->clear();
    for (const auto& map : maps) {
        if (valid_source_name(map.toString())) {
            ui->source_list->addItem(map.toString());
        }
    }
}

bool vlc::has_mapping(const char* scene, const char* source)
{
    for (const auto& k : m_source_map.keys()) {
        if (k == utf8_to_qt(scene)) {
            for (const auto& mapping : m_source_map[k].toArray()) {
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
    QJsonObject new_map;
    for (auto& k : m_source_map.keys()) {
        auto* scene = obs_get_scene_by_name(qt_to_utf8(k));

        if (scene) {
            QJsonArray arr;
            for (const auto& m : m_source_map[k].toArray()) {
                if (valid_source_name(m.toString()))
                    arr.append(m.toString());
            }
            new_map[k] = arr;
            obs_scene_release(scene);
        }
    }
    m_source_map = new_map;
}

QJsonArray vlc::get_mappings_for_scene(const char* scene)
{
    if (m_source_map.contains(utf8_to_qt(scene)))
        return m_source_map[utf8_to_qt(scene)].toArray();
    return {};
}

void vlc::save_settings()
{
    CSET_STR(CFG_VLC_ID, qt_to_utf8(ui->cb_vlc_source_name->currentText()));
    if (!util::save_config(VLC_SCENE_MAPPING, QJsonDocument(m_source_map))) {
        berr("Failed to save vlc mappings");
    }
}

static bool add_source(void* data, obs_source_t* src)
{
    auto* id = obs_source_get_id(src);
    if (strcmp(id, "vlc_source") == 0) {
        auto* name = obs_source_get_name(src);
        QComboBox* cb = reinterpret_cast<QComboBox*>(data);
        cb->addItem(utf8_to_qt(name));
    }

    return true;
}

void vlc::load_vlc_sources()
{
    ui->cb_vlc_source_name->clear();
    ui->cb_vlc_source_name->addItem(T_VLC_NONE);

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

    obs_enum_sources(add_source, ui->cb_vlc_source_name);
}

void vlc::on_btn_refresh_vlc_clicked()
{
    load_vlc_sources();
}

void vlc::on_scene_changed(int)
{
    refresh_sources();
    ui->source_list->clear();
    auto id = ui->cb_scene->currentText();

    if (m_source_map.contains(id)) {
        auto val = m_source_map[id];
        if (val.isArray()) {
            for (const auto& entry : val.toArray()) {
                if (entry.isString())
                    ui->source_list->addItem(new QListWidgetItem(entry.toString()));
            }
        }
    }
}

void vlc::on_remove_source()
{
    auto source = ui->source_list->currentItem()->text();
    auto scene = ui->cb_scene->currentText();
    ui->source_list->takeItem(ui->source_list->currentRow());
    rebuild_from_list();
}

void vlc::on_reorder_sources(const QModelIndexList&)
{
    rebuild_from_list();
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
    for (int i = 0; i < ui->source_list->count(); i++) {
        auto item = ui->source_list->item(i);

        if (item && valid_source_name(item->text()))
            valid_sources.append(item->text());
    }

    ui->source_list->clear();
    ui->source_list->addItems(valid_sources);
    auto scene = ui->cb_scene->currentText();

    m_source_map[scene] = QJsonArray::fromStringList(valid_sources);
}

void vlc::on_add_source()
{
    auto src_id = ui->cb_source->currentText();
    auto scene_id = ui->cb_scene->currentText();

    auto* src = obs_get_source_by_name(qt_to_utf8(src_id));
    auto* scene = obs_get_scene_by_name(qt_to_utf8(scene_id));

    if (src && scene) {
        std::lock_guard<std::mutex> lock(tuna_thread::thread_mutex);
        if (m_source_map.contains(scene_id)) {
            auto arr = m_source_map[scene_id].toArray();
            arr.append(src_id);
            m_source_map[scene_id] = arr;
        } else {
            QJsonArray arr;
            arr.append(src_id);
            m_source_map[scene_id] = arr;
        }

        ui->source_list->addItem(src_id);
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
    }
}

void vlc::select_vlc_source(const QString& id)
{
    const auto idx = ui->cb_vlc_source_name->findText(id, Qt::MatchExactly);

    if (idx >= 0)
        ui->cb_vlc_source_name->setCurrentIndex(idx);
    else
        ui->cb_vlc_source_name->setCurrentIndex(0);
}
