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
class mpd;
}

class mpd : public source_widget {
    Q_OBJECT

public:
    explicit mpd(QWidget* parent = nullptr);
    ~mpd();

    void load_settings() override;
    void save_settings() override;
private slots:
    void on_rb_remote_toggled(bool checked);

    void on_btn_browse_base_folder_clicked();

private:
    Ui::mpd* ui;
};
