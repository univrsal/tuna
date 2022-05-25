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

#pragma once

#include "../tuna_gui.hpp"
#include <QTimer>
#include <string>
#include <utility>
#include <vector>

namespace Ui {
class window_title;
}

class window_title : public source_widget {
    Q_OBJECT
    std::vector<std::pair<std::string, std::string>> m_items;

public:
    explicit window_title(QWidget* parent = nullptr);
    ~window_title();

    void save_settings() override;
    void load_settings() override;

    void refresh_process_list();
private slots:

    void on_rb_process_name_clicked(bool checked);
    void on_rb_window_title_clicked(bool checked);
    void on_btn_refresh_clicked();

private:
    Ui::window_title* ui;
    QTimer* m_timer = nullptr;
};
