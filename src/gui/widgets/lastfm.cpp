/*************************************************************************
 * This file is part of tuna
 * github.com/univrsal/tuna
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

#include "lastfm.hpp"
#include "../../util/config.hpp"
#include "../../util/constants.hpp"
#include "../../util/utility.hpp"
#include "ui_lastfm.h"

lastfm::lastfm(QWidget* parent)
    : source_widget(parent)
    , ui(new Ui::lastfm)
{
    ui->setupUi(this);
}

lastfm::~lastfm()
{
    delete ui;
}

void lastfm::load_settings()
{
    ui->txt_username->setText(utf8_to_qt(CGET_STR(CFG_LASTFM_USERNAME)));
    ui->txt_apikey->setText(utf8_to_qt(CGET_STR(CFG_LASTFM_API_KEY)));
}

void lastfm::save_settings()
{
    CSET_STR(CFG_LASTFM_USERNAME, qt_to_utf8(ui->txt_username->text()));
    CSET_STR(CFG_LASTFM_API_KEY, qt_to_utf8(ui->txt_apikey->text()));
}
