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
    connect(this, &spotify::login_state_changed, this, &spotify::apply_login_state);
}

spotify::~spotify()
{
    delete ui;
    delete m_token_refresh_promise;
    delete m_token_request_promise;
}

void spotify::load_settings()
{
    auto id = utf8_to_qt(CGET_STR(CFG_SPOTIFY_CLIENT_ID));
    auto secret = utf8_to_qt(CGET_STR(CFG_SPOTIFY_CLIENT_SECRET));
    auto state = CGET_BOOL(CFG_SPOTIFY_LOGGEDIN);

    ui->txt_client_id->setText(id);
    ui->txt_secret->setText(secret);
    apply_login_state(state, "");

    if (!state)
        binfo("Not logged into Spotify.");
    else
        binfo("Logged into Spotify.");

    if (!id.isEmpty() || !secret.isEmpty())
        binfo("Using custom Spotify credentials. ID set: %s, secret set: %s", id.isEmpty() ? "no" : "yes", secret.isEmpty() ? "no" : "yes");
}

void spotify::tick()
{
    auto check = [](auto& future) {
        if (future.valid()) {
            auto status = future.wait_for(std::chrono::microseconds(100));
            return status == std::future_status::ready;
        }
        return false;
    };

    if (check(m_token_refresh_future)) {
        auto result = m_token_refresh_future.get();
        apply_login_state(result.success, result.value);
        m_token_refresh_future = {};
        delete m_token_refresh_promise;
        m_token_refresh_promise = nullptr;
    }

    if (check(m_token_request_future)) {
        auto result = m_token_request_future.get();
        apply_login_state(result.success, result.value);
        m_token_request_future = {};
        delete m_token_request_promise;
        m_token_request_promise = nullptr;
    }
}

void spotify::save_settings()
{
    CSET_STR(CFG_SPOTIFY_CLIENT_ID, qt_to_utf8(ui->txt_client_id->text()));
    CSET_STR(CFG_SPOTIFY_CLIENT_SECRET, qt_to_utf8(ui->txt_secret->text()));
    auto spotify = music_sources::get<spotify_source>(S_SOURCE_SPOTIFY);
    if (spotify) {
        CSET_STR(CFG_SPOTIFY_AUTH_CODE, qt_to_utf8(spotify->auth_code()));
        CSET_STR(CFG_SPOTIFY_TOKEN, qt_to_utf8(spotify->token()));
        CSET_STR(CFG_SPOTIFY_REFRESH_TOKEN, qt_to_utf8(spotify->refresh_token()));
        CSET_BOOL(CFG_SPOTIFY_LOGGEDIN, spotify->is_logged_in());
        CSET_INT(CFG_SPOTIFY_TOKEN_TERMINATION, spotify->token_termination());
    }
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
    ui->txt_secret->setEchoMode(QLineEdit::Normal);
}

void spotify::on_btn_show_secret_released()
{
    ui->txt_secret->setEchoMode(QLineEdit::Password);
}

void spotify::on_btn_sp_show_auth_pressed()
{
    ui->txt_auth_code->setEchoMode(QLineEdit::Normal);
}

void spotify::on_btn_sp_show_auth_released()
{
    ui->txt_auth_code->setEchoMode(QLineEdit::Password);
}

void spotify::on_btn_sp_show_token_pressed()
{
    ui->txt_token->setEchoMode(QLineEdit::Normal);
}

void spotify::on_btn_sp_show_token_released()
{
    ui->txt_token->setEchoMode(QLineEdit::Password);
}

void spotify::on_btn_sp_show_refresh_token_pressed()
{
    ui->txt_refresh_token->setEchoMode(QLineEdit::Normal);
}

void spotify::on_btn_sp_show_refresh_token_released()
{
    ui->txt_refresh_token->setEchoMode(QLineEdit::Password);
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

    auto spotify = music_sources::get<spotify_source>(S_SOURCE_SPOTIFY);

    if (spotify) {
        spotify->set_auth_code(ui->txt_auth_code->text());
        delete m_token_request_promise;
        m_token_request_promise = new std::promise<result>;
        m_token_request_future = m_token_request_promise->get_future();
        // Do the request in a separate thread so the ui doesn't freeze
        std::thread([this] {
            auto spotify = music_sources::get<spotify_source>(S_SOURCE_SPOTIFY);
            if (spotify) {
                QString log;
                bool result = spotify->new_token(log);
                m_token_request_promise->set_value_at_thread_exit({ result, log });
            }
        }).detach();
    } else {
        berr("Couldn't get spotify source instance");
    }
}

void spotify::apply_login_state(bool state, const QString& log)
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
        save_settings();
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
    auto spotify = music_sources::get<spotify_source>(S_SOURCE_SPOTIFY);

    if (spotify) {
        spotify->set_auth_code(ui->txt_auth_code->text());
        m_token_refresh_promise = new std::promise<spotify::result>;
        m_token_refresh_future = m_token_refresh_promise->get_future();

        // Do the refresh in a  separate thread so the ui doesn't freeze
        std::thread([this] {
            auto spotify = music_sources::get<spotify_source>(S_SOURCE_SPOTIFY);
            if (spotify) {
                QString log;
                bool result = spotify->do_refresh_token(log);
                m_token_refresh_promise->set_value_at_thread_exit({ result, log });
            }
        }).detach();
    } else {
        berr("Couldn't get spotify source instance");
    }
}
