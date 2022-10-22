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

#include "wmc.hpp"
#include "../../query/wmc_source.hpp"
#include "../../util/config.hpp"
#include "../../util/constants.hpp"
#include "../../util/utility.hpp"
#include "ui_wmc.h"

wmc::wmc(QWidget* parent)
    : source_widget(parent)
    , ui(new Ui::wmc)
{
    ui->setupUi(this);
}

wmc::~wmc()
{
    delete ui;
}

void wmc::load_settings()
{
    auto idx = ui->cb_player->findText(utf8_to_qt(CGET_STR(CFG_WMC_PLAYER)));
    if (idx >= 0)
        ui->cb_player->setCurrentIndex(idx);
}

void wmc::save_settings()
{
    CSET_STR(CFG_WMC_PLAYER, qt_to_utf8(ui->cb_player->currentText()));
}

void wmc::tick()
{
    auto wmc_src = music_sources::get<wmc_source>(S_SOURCE_WMC);
    if (wmc_src) {
        std::lock_guard<std::mutex> lock(wmc_src->m_internal_mutex);

        QStringList new_players;
        for (auto const& player : wmc_src->m_registered_players)
            new_players.append(utf8_to_qt(player.c_str()));

        if (m_current_players != new_players) {
            m_current_players = new_players;
            auto current_player = ui->cb_player->currentText();
            ui->cb_player->clear();

            for (auto const& player : new_players)
                ui->cb_player->addItem(player);

            auto idx = ui->cb_player->findText(current_player);
            if (idx >= 0)
                ui->cb_player->setCurrentIndex(idx);
        }
    }
}

QComboBox* wmc::get_players()
{
    return ui->cb_player;
}
