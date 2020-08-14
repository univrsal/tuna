/* spotify.cpp created on 2020.8.14
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

#include "spotify.hpp"
#include "../../query/spotify_source.hpp"
#include "../../util/config.hpp"
#include "../../util/constants.hpp"
#include "../../util/utility.hpp"
#include "ui_spotify.h"
#include <QDesktopServices>
#include <QMessageBox>

spotify::spotify(QWidget* parent)
    : source_widget(parent)
    , ui(new Ui::spotify)
{
    ui->setupUi(this);
}

spotify::~spotify()
{
    delete ui;
}

void spotify::load_settings()
{
    ui->txt_client_id->setText(utf8_to_qt(CGET_STR(CFG_SPOTIFY_CLIENT_ID)));
    ui->txt_secret->setText(utf8_to_qt(CGET_STR(CFG_SPOTIFY_CLIENT_SECRET)));
    apply_login_state(CGET_BOOL(CFG_SPOTIFY_LOGGEDIN), "");
}

void spotify::save_settings()
{
    CSET_STR(CFG_SPOTIFY_CLIENT_ID, qt_to_utf8(ui->txt_client_id->text()));
    CSET_STR(CFG_SPOTIFY_CLIENT_SECRET, qt_to_utf8(ui->txt_secret->text()));
}

void spotify::on_btn_id_show_pressed()
{
    ui->txt_client_id->setEchoMode(QLineEdit::Normal);
}

void spotify::on_btn_id_show_released()
{
    ui->txt_client_id->setEchoMode(QLineEdit::Password);
}

void spotify::on_btn_show_secret_pressed()
{
    ui->txt_client_id->setEchoMode(QLineEdit::Normal);
}

void spotify::on_btn_show_secret_released()
{
    ui->txt_client_id->setEchoMode(QLineEdit::Password);
}

void spotify::on_btn_sp_show_auth_pressed()
{
    ui->txt_client_id->setEchoMode(QLineEdit::Normal);
}

void spotify::on_btn_sp_show_auth_released()
{
    ui->txt_client_id->setEchoMode(QLineEdit::Password);
}

void spotify::on_btn_sp_show_token_pressed()
{
    ui->txt_client_id->setEchoMode(QLineEdit::Normal);
}

void spotify::on_btn_sp_show_token_released()
{
    ui->txt_client_id->setEchoMode(QLineEdit::Password);
}

void spotify::on_btn_sp_show_refresh_token_pressed()
{
    ui->txt_client_id->setEchoMode(QLineEdit::Normal);
}

void spotify::on_btn_sp_show_refresh_token_released()
{
    ui->txt_client_id->setEchoMode(QLineEdit::Password);
}

void spotify::on_btn_open_login_clicked()
{
    static QString base_url = "https://univrsal.github.io/auth/login?client_id=";
    auto client_id = ui->txt_client_id->text();
    if (client_id.isEmpty())
        client_id = "847d7cf0c5dc4ff185161d1f000a9d0e";
    QString url = base_url + client_id;
    QMessageBox::information(this, "Info", T_SPOTIFY_WARNING);
    QDesktopServices::openUrl(QUrl(url));
}

void spotify::on_txt_auth_code_textChanged(const QString& arg1)
{
    ui->btn_request_token->setEnabled(arg1.length() > 0);
}

void spotify::on_btn_request_token_clicked()
{
    /* Make sure that the custom data is in the config, otherwise the user
     * has to click apply before */
    CSET_STR(CFG_SPOTIFY_CLIENT_ID, qt_to_utf8(ui->txt_client_id->text()));
    CSET_STR(CFG_SPOTIFY_CLIENT_SECRET, qt_to_utf8(ui->txt_secret->text()));

    QString log;
    auto result = false;
    auto spotify = music_sources::get<spotify_source>(S_SOURCE_SPOTIFY);

    if (spotify) {
        spotify->set_auth_code(ui->txt_auth_code->text());
        result = spotify->new_token(log);
    } else {
        berr("Couldn't get spotify source instance");
    }
    apply_login_state(result, log);
}

void spotify::apply_login_state(bool state, const QString& log) const
{
    if (state) {
        auto spotify = music_sources::get<spotify_source>(S_SOURCE_SPOTIFY);
        if (spotify) {
            ui->txt_token->setText(spotify->token());
            ui->txt_refresh_token->setText(spotify->refresh_token());
            ui->txt_auth_code->setText(spotify->auth_code());
        }

        ui->lbl_spotify_info->setText(T_SPOTIFY_LOGGEDIN);
        ui->lbl_spotify_info->setStyleSheet("QLabel { color: green; "
                                            "font-weight: bold;}");
        config::save();
    } else {
        ui->lbl_spotify_info->setText(T_SPOTIFY_LOGGEDOUT);
        ui->lbl_spotify_info->setStyleSheet("QLabel {}");
    }
    ui->btn_performrefresh->setEnabled(state);

    /* Log */
    if (ui->cb_use_log->isChecked() && !log.isEmpty()) {
        QDateTime now = QDateTime::currentDateTime();
        ui->txt_json_log->append("= " + now.toString("yyyy.MM.dd hh:mm") + " =");
        ui->txt_json_log->append(log);
    }
}

void spotify::on_btn_performrefresh_clicked()
{
    QString log;
    bool result = false;
    auto spotify = music_sources::get<spotify_source>(S_SOURCE_SPOTIFY);

    if (spotify) {
        spotify->set_auth_code(ui->txt_auth_code->text());
        result = spotify->do_refresh_token(log);
    } else {
        berr("Couldn't get spotify source instance");
    }
    apply_login_state(result, log);
}
