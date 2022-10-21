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

#include "mpris.hpp"
#include "../../query/mpris_source.hpp"
#include "../../util/config.hpp"
#include "../../util/constants.hpp"
#include "../../util/utility.hpp"
#include "ui_mpris.h"

mpris::mpris(QWidget* parent)
    : source_widget(parent)
    , ui(new Ui::mpris)
{
    ui->setupUi(this);
}

mpris::~mpris()
{
    delete ui;
}

void mpris::load_settings()
{
    auto idx = ui->cb_player->findData(utf8_to_qt(CGET_STR(CFG_MPRIS_PLAYER)));
    if (idx >= 0)
        ui->cb_player->setCurrentIndex(idx);
}

void mpris::save_settings()
{
    CSET_STR(CFG_MPRIS_PLAYER, qt_to_utf8(ui->cb_player->currentData().toString()));
}

void mpris::tick()
{
    auto mpris_src = music_sources::get<mpris_source>(S_SOURCE_MPRIS);
    if (mpris_src) {
        std::lock_guard<std::mutex> lock(mpris_src->m_internal_mutex);

        if (m_current_players != mpris_src->m_players) {
            m_current_players = mpris_src->m_players;
            auto current_player = ui->cb_player->currentData().toString();
            ui->cb_player->clear();

            for (auto const& player : mpris_src->m_players.keys())
                ui->cb_player->addItem(mpris_src->m_players[player], player);

            auto idx = ui->cb_player->findData(current_player);
            if (idx >= 0)
                ui->cb_player->setCurrentIndex(idx);
        }
    }
}

QComboBox* mpris::get_players()
{
    return ui->cb_player;
}
