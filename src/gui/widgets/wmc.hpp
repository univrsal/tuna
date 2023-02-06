/*************************************************************************
 * This file is part of tuna
 * git.vrsal.xyz/alex/tuna
 * Copyright 2023 univrsal <uni@vrsal.xyz>.
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
#include <QComboBox>
#include <QWidget>

namespace Ui {
class wmc;
}

class wmc : public source_widget {

    Q_OBJECT
    QStringList m_current_players;

public:
    explicit wmc(QWidget* parent = nullptr);
    ~wmc();

    void load_settings() override;
    void save_settings() override;
    void tick() override;

    QComboBox* get_players();

private:
    Ui::wmc* ui;
};
