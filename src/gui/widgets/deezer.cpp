/*************************************************************************
 * This file is part of tuna
 * github.con/univrsal/tuna
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
#include "deezer.hpp"
#include "../../query/deezer_source.hpp"
#include "../../util/config.hpp"
#include "../../util/utility.hpp"
#include "ui_deezer.h"

deezer::deezer(QWidget* parent)
    : source_widget(parent)
    , ui(new Ui::deezer)
{
    ui->setupUi(this);
}

deezer::~deezer()
{
    delete ui;
}

void deezer::load_settings()
{
    ui->txt_client_id->setText(utf8_to_qt(CGET_STR(CFG_DEEZER_CLIENT_ID)));
    ui->txt_secret->setText(utf8_to_qt(CGET_STR(CFG_DEEZER_CLIENT_SECRET)));
    ui->txt_auth_code->setText(utf8_to_qt(CGET_STR(CFG_DEEZER_AUTH_CODE)));
    ui->txt_token->setText(utf8_to_qt(CGET_STR(CFG_DEEZER_TOKEN)));
}

void deezer::save_settings()
{
    CSET_STR(CFG_SPOTIFY_CLIENT_ID, qt_to_utf8(ui->txt_client_id->text()));
    CSET_STR(CFG_SPOTIFY_CLIENT_SECRET, qt_to_utf8(ui->txt_secret->text()));
    auto deezer = music_sources::get<deezer_source>(S_SOURCE_DEEZER);
    if (deezer) {
        CSET_STR(CFG_SPOTIFY_AUTH_CODE, qt_to_utf8(deezer->auth_code()));
        CSET_STR(CFG_SPOTIFY_TOKEN, qt_to_utf8(deezer->token()));
        CSET_BOOL(CFG_SPOTIFY_LOGGEDIN, deezer->is_logged_in());
    }
}

void deezer::on_btn_id_show_pressed()
{
    ui->txt_client_id->setEchoMode(QLineEdit::Normal);
}

void deezer::on_btn_id_show_released()
{
    ui->txt_client_id->setEchoMode(QLineEdit::Password);
}

void deezer::on_btn_show_secret_pressed()
{
    ui->txt_secret->setEchoMode(QLineEdit::Normal);
}

void deezer::on_btn_show_secret_released()
{
    ui->txt_secret->setEchoMode(QLineEdit::Password);
}

void deezer::on_btn_sp_show_auth_pressed()
{
    ui->txt_auth_code->setEchoMode(QLineEdit::Normal);
}

void deezer::on_btn_sp_show_auth_released()
{

    ui->txt_auth_code->setEchoMode(QLineEdit::Password);
}

void deezer::on_btn_sp_show_token_pressed()
{
    ui->txt_token->setEchoMode(QLineEdit::Normal);
}

void deezer::on_btn_sp_show_token_released()
{
    ui->txt_token->setEchoMode(QLineEdit::Password);
}
