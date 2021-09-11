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

#include "icecast.hpp"
#include "../../util/config.hpp"
#include "../../util/constants.hpp"
#include "../../util/utility.hpp"
#include "ui_icecast.h"

icecast::icecast(QWidget* parent)
    : source_widget(parent)
    , ui(new Ui::icecast)
{
    ui->setupUi(this);
}

icecast::~icecast()
{
    delete ui;
}

void icecast::load_settings()
{
    ui->txt_icecast_url->setText(utf8_to_qt(CGET_STR(CFG_ICECAST_URL)));
}

void icecast::save_settings()
{
    CSET_STR(CFG_ICECAST_URL, qt_to_utf8(ui->txt_icecast_url->text()));
}
