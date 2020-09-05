/*************************************************************************
 * This file is part of tuna
 * github.con/univrsal/tuna
 * Copyright 2020 univrsal <universailp@web.de>.
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

#include <QDialog>

class format_validator;

namespace Ui {
class output_edit_dialog;
}

class tuna_gui;
enum class edit_mode { create,
    modify };

class output_edit_dialog : public QDialog {
    Q_OBJECT
public:
    output_edit_dialog(edit_mode m, QWidget* parent = nullptr);
    ~output_edit_dialog();

private slots:
    void buttonBox_accepted();
    void browse_file_clicked();

    void on_txt_format_textChanged(const QString& arg1);

private:
    Ui::output_edit_dialog* ui;
    edit_mode m_mode;
    tuna_gui* m_tuna;
};
