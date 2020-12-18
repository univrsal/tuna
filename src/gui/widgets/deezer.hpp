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

#pragma once

#include "../tuna_gui.hpp"
#include <QWidget>

namespace Ui {
class deezer;
}

class deezer : public source_widget {
    Q_OBJECT

public:
    explicit deezer(QWidget* parent = nullptr);
    ~deezer();

    void load_settings() override;
    void save_settings() override;

private slots:
    void on_btn_id_show_pressed();

    void on_btn_id_show_released();

    void on_btn_show_secret_pressed();

    void on_btn_show_secret_released();

    void on_btn_sp_show_auth_pressed();

    void on_btn_sp_show_auth_released();

    void on_btn_sp_show_token_pressed();

    void on_btn_sp_show_token_released();

private:
    Ui::deezer* ui;
};
