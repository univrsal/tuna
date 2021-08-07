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

#pragma once

#include "../tuna_gui.hpp"

namespace Ui {
class vlc;
}

class vlc : public source_widget {
    Q_OBJECT
    void load_vlc_sources();

public:
    explicit vlc(QWidget* parent = nullptr);
    ~vlc();

    void select_vlc_source(const QString& id);
    void save_settings() override;
    void load_settings() override;
private slots:
    void on_btn_refresh_vlc_clicked();

private:
    Ui::vlc* ui;
};
