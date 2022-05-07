/*************************************************************************
 * This file is part of tuna
 * github.com/univrsal/tuna
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

#pragma once

#include "../tuna_gui.hpp"
#include <QJsonArray>
#include <QJsonObject>
#include <QListWidgetItem>
#include <obs-module.h>

namespace Ui {
class vlc;
}

class drag_list;

class vlc : public source_widget {
    Q_OBJECT
    void load_vlc_sources();
    QJsonObject m_source_map;
    drag_list* m_list { nullptr };

public:
    explicit vlc(QWidget* parent = nullptr);
    ~vlc();

    void save_settings() override;
    void load_settings() override;

    void build_list();
    bool has_mapping(const char* scene, const char* source);
    void rebuild_mapping();
    QJsonArray get_mappings_for_scene(const char* scene);
    void rebuild_from_list();

    QString get_scene_collection();
private slots:
    void on_btn_refresh_vlc_clicked();
    void on_scene_changed(int index);
    void on_add_source();
    void on_remove_source();

private:
    void set_map(const QString& scene, const QJsonArray&);
    bool valid_source_name(const QString& str);

    obs_scene_t* get_selected_scene();
    void refresh_sources();
    Ui::vlc* ui;
};

class drag_list : public QListWidget {
    Q_OBJECT

    vlc* m_vlc;

public:
    drag_list(vlc* parent = nullptr)
        : QListWidget(parent)
        , m_vlc(parent)
    {
    }

protected:
    void dropEvent(QDropEvent* event) override
    {
        QListWidget::dropEvent(event);
        m_vlc->rebuild_from_list();
    }
};
