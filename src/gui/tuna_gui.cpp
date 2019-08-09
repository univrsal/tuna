/* tuna_gui.cpp created on 2019.8.8
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
#include "tuna_gui.hpp"
#include "ui_tuna_gui.h"
#include "../util/config.hpp"
#include "../util/constants.hpp"
#include "../util/tuna_thread.hpp"
#include "../query/spotify_source.hpp"
#include <obs-module.h>
#include <QPixmap>
#include <QDesktopServices>
#include <QMessageBox>
#include <QDate>
#include <random>

tuna_gui* tuna_dialog = nullptr;

tuna_gui::tuna_gui(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::tuna_gui)
{
    ui->setupUi(this);

    /* load logo */
    const char* path = obs_module_file("tuna.jpg");
    QPixmap img(path);
    ui->lbl_img->setPixmap(img);
    bfree((void*)path);
    ui->settings_tabs->setCurrentIndex(0);

    if (config_get_bool(config::instance, CFG_REGION, CFG_SPOTIFY_LOGGEDIN)) {
        ui->lbl_spotify_info->setText(T_SPOTIFY_LOGGEDIN);
        ui->lbl_spotify_info->setStyleSheet("QLabel { color: green; "
                                          "font-weight: bold;}");
    } else {
        ui->lbl_spotify_info->setText(T_SPOTIFY_LOGGEDOUT);
    }

    /* setup paths */
    ui->txt_song_info->setText(config_get_string(config::instance, CFG_REGION,
                                                 CFG_SONG_PATH));
    ui->txt_song_cover->setText(config_get_string(config::instance, CFG_REGION,
                                                 CFG_COVER_PATH));
    ui->txt_song_lyrics->setText(config_get_string(config::instance, CFG_REGION,
                                                 CFG_LYRICS_PATH));
    ui->cb_source->setCurrentIndex(config_get_uint(config::instance, CFG_REGION,
                                                  CFG_SELECTED_SOURCE));
    ui->sb_refresh_rate->setValue(config_get_uint(config::instance, CFG_REGION,
                                                  CFG_REFRESH_RATE));
    set_state();
}

void tuna_gui::set_state()
{
    if (thread::thread_state)
        ui->lbl_status->setText(T_STATUS_RUNNING);
    else
        ui->lbl_status->setText(T_STATUS_STOPPED);
}

tuna_gui::~tuna_gui()
{
    delete ui;
}

void tuna_gui::toggleShowHide()
{
    setVisible(!isVisible());
}

void tuna_gui::on_btn_sp_show_auth_pressed()
{
    ui->txt_auth_code->setEchoMode(QLineEdit::Normal);
}

void tuna_gui::on_btn_sp_show_auth_released()
{
    ui->txt_auth_code->setEchoMode(QLineEdit::Password);
}

void tuna_gui::on_btn_open_login_clicked()
{
    static QString base_url = "https://univrsal.github.io/auth/login?state=";
    QMessageBox::information(this, "Info", T_SPOTIFY_WARNING);
    /* Each login uses a random number before login which is matched
     * against the number returned after login, to make sure the login
     * process finished correctly */
    std::random_device dev;
    std::mt19937 rng(dev());
    std::uniform_int_distribution<std::mt19937::result_type> dist(1, 32000);
    QDesktopServices::openUrl(QUrl(base_url + QString::number(dist(rng))));
}

void tuna_gui::on_txt_auth_code_textChanged(const QString &arg1)
{
    ui->btn_request_token->setEnabled(arg1.length() > 0);
}

void tuna_gui::apply_login_state(bool state, const QString& log)
{
    if (state) {
        ui->txt_token->setText(QString::fromStdString(config::spotify->token()));
        ui->txt_refresh_token->setText(QString::fromStdString(config::spotify->refresh_token()));
        ui->lbl_spotify_info->setText(T_SPOTIFY_LOGGEDIN);
        ui->lbl_spotify_info->setStyleSheet("QLabel { color: green; "
                                          "font-weight: bold;}");
        config::spotify->save();
    } else {
        ui->lbl_spotify_info->setText(T_SPOTIFY_LOGGEDOUT);
        ui->lbl_spotify_info->setStyleSheet("QLabel {}");
    }
    ui->btn_performrefresh->setEnabled(state);

    /* Log */
    QDate now = QDate::currentDate();
    ui->txt_json_log->appendPlainText("= " + now.toString() + " =");
    ui->txt_json_log->appendPlainText(log);
}

void tuna_gui::on_btn_request_token_clicked()
{
    QString log;
    config::spotify->set_auth_code(ui->txt_auth_code->text().toStdString());
    bool result = config::spotify->new_token(log);
    apply_login_state(result, log);
}

void tuna_gui::on_btn_performrefresh_clicked()
{
    QString log;
    config::spotify->set_auth_code(ui->txt_auth_code->text().toStdString());
    bool result = config::spotify->new_token(log);
    apply_login_state(result, log);
}

void tuna_gui::on_tuna_gui_accepted()
{
    config_set_string(config::instance, CFG_REGION, CFG_SONG_PATH,
                      qPrintable(ui->txt_song_info->text()));
    config_set_string(config::instance, CFG_REGION, CFG_COVER_PATH,
                      qPrintable(ui->txt_song_cover->text()));
    config_set_string(config::instance, CFG_REGION, CFG_LYRICS_PATH,
                      qPrintable(ui->txt_song_lyrics->text()));
    config_set_int(config::instance, CFG_REGION, CFG_SELECTED_SOURCE,
                   ui->cb_source->currentIndex());
    config_set_uint(config::instance, CFG_REGION, CFG_REFRESH_RATE,
                    ui->sb_refresh_rate->value());
    thread::mutex.lock();
    thread::sleep_time = ui->sb_refresh_rate->value();
    thread::mutex.unlock();
}

void tuna_gui::on_btn_start_clicked()
{
    if (!thread::start()) {
        QMessageBox::warning(this, "Error", "Thread couldn't be started!");
    }
    set_state();
}

void tuna_gui::on_btn_stop_clicked()
{
    thread::stop();
    set_state();
}
