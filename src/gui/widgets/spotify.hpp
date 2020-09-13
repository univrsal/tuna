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

namespace Ui {
class spotify;
}

class spotify : public source_widget {
    Q_OBJECT
    void apply_login_state(bool state, const QString& log);

public:
    explicit spotify(QWidget* parent = nullptr);
    ~spotify();

    void save_settings() override;
    void load_settings() override;
signals:
    void login_state_changed(bool sate, QString& log);
private slots:
    void on_btn_id_show_pressed();
    void on_btn_id_show_released();
    void on_btn_show_secret_pressed();
    void on_btn_show_secret_released();
    void on_btn_sp_show_auth_pressed();
    void on_btn_sp_show_auth_released();
    void on_btn_sp_show_token_pressed();
    void on_btn_sp_show_token_released();
    void on_btn_sp_show_refresh_token_pressed();
    void on_btn_sp_show_refresh_token_released();
    void on_btn_open_login_clicked();
    void on_txt_auth_code_textChanged(const QString& arg1);
    void on_btn_request_token_clicked();
    void on_btn_performrefresh_clicked();

private:
    Ui::spotify* ui;
};
