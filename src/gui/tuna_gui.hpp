/* tuna_gui.hpp created on 2019.8.8
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 * github.com/univrsal/
 *
 */
#pragma once

#include <QDialog>

namespace Ui {
class tuna_gui;
}

class tuna_gui : public QDialog
{
    Q_OBJECT

public:
    explicit tuna_gui(QWidget *parent = nullptr);
    ~tuna_gui();
    void toggleShowHide();

    void apply_login_state(bool state, const QString& log);

private slots:
    void on_btn_sp_show_auth_pressed();

    void on_btn_sp_show_auth_released();

    void on_btn_open_login_clicked();

    void on_txt_auth_code_textChanged(const QString &arg1);

    void on_btn_request_token_clicked();

    void on_btn_performrefresh_clicked();

    void on_tuna_gui_accepted();

    void on_btn_start_clicked();

    void set_state();
    void on_btn_stop_clicked();

private:
    Ui::tuna_gui *ui;
};

extern tuna_gui* tuna_dialog;
