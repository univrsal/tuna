/*************************************************************************
 * This file is part of tuna
 * github.con/univrsal/tuna
 * Copyright 2019 univrsal <universailp@web.de>.
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

#include "tuna_gui.hpp"
#include "../query/spotify_source.hpp"
#include "../util/config.hpp"
#include "../util/constants.hpp"
#include "../util/tuna_thread.hpp"
#include "output_edit_dialog.hpp"
#include "ui_tuna_gui.h"
#include <QDate>
#include <QDesktopServices>
#include <QFileDialog>
#include <QList>
#include <QMessageBox>
#include <QPair>
#include <QPixmap>
#include <QString>
#include <QTimer>
#include <obs-frontend-api.h>
#include <obs-module.h>
#include <random>
#include <util/platform.h>

tuna_gui* tuna_dialog = nullptr;

tuna_gui::tuna_gui(QWidget* parent)
    : QDialog(parent)
    , ui(new Ui::tuna_gui)
{
    ui->setupUi(this);
    connect(ui->buttonBox->button(QDialogButtonBox::Apply),
        SIGNAL(clicked()),
        this,
        SLOT(on_apply_pressed()));
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);

    /* load logo */
    const char* path = obs_module_file("tuna.png");
    QPixmap img(path);
    ui->lbl_img->setPixmap(img);
    bfree((void*)path);
    ui->settings_tabs->setCurrentIndex(0);
    bool logged_in = CGET_BOOL(CFG_SPOTIFY_LOGGEDIN);
    if (logged_in) {
        ui->lbl_spotify_info->setText(T_SPOTIFY_LOGGEDIN);
        ui->lbl_spotify_info->setStyleSheet("QLabel { color: green; "
                                            "font-weight: bold;}");
    } else {
        ui->lbl_spotify_info->setText(T_SPOTIFY_LOGGEDOUT);
    }
    ui->btn_performrefresh->setEnabled(logged_in);

#ifdef LINUX
    ui->cb_source->addItem(T_SOURCE_MPD);
#else
    ui->settings_tabs->removeTab(2);
#endif
    /* TODO Lyrics */
    ui->frame_lyrics->setVisible(false);
}

void tuna_gui::choose_file(QString& path, const char* title, const char* file_types)
{
    path = QFileDialog::getSaveFileName(
        this, tr(title), QDir::home().path(), tr(file_types));
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
    if (isVisible()) {
        /* Load config values for sources on dialog show */
        config::load_gui_values();

        /* setup config values */
        ui->txt_song_cover->setText(CGET_STR(CFG_COVER_PATH));
        ui->txt_song_lyrics->setText(CGET_STR(CFG_LYRICS_PATH));
        ui->cb_source->setCurrentIndex(CGET_UINT(CFG_SELECTED_SOURCE));
        ui->sb_refresh_rate->setValue(CGET_UINT(CFG_REFRESH_RATE));
        ui->txt_song_placeholder->setText(
            QString::fromUtf8(CGET_STR(CFG_SONG_PLACEHOLDER)));
        ui->cb_dl_cover->setChecked(CGET_BOOL(CFG_DOWNLOAD_COVER));
        set_state();

        /* Load table contents */
        int row = 1; /* Clear all rows except headers */
        for (; row < ui->tbl_outputs->rowCount(); row++)
            ui->tbl_outputs->removeRow(row);
        row = 0; /* Load rows */
        ui->tbl_outputs->setRowCount(config::outputs.size());
        for (const auto& entry : config::outputs) {
            ui->tbl_outputs->setItem(row, 0, new QTableWidgetItem(entry.first));
            ui->tbl_outputs->setItem(row, 1, new QTableWidgetItem(entry.second));
            row++;
        }
    }
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

void tuna_gui::on_txt_auth_code_textChanged(const QString& arg1)
{
    ui->btn_request_token->setEnabled(arg1.length() > 0);
}

void tuna_gui::apply_login_state(bool state, const QString& log)
{
    if (state) {
        ui->txt_token->setText(QString::fromStdString(config::spotify->token()));
        ui->txt_refresh_token->setText(
            QString::fromStdString(config::spotify->refresh_token()));
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
    if (ui->cb_use_log->isChecked()) {
        QDateTime now = QDateTime::currentDateTime();
        ui->txt_json_log->append("= " + now.toString("yyyy.MM.dd hh:mm") + " =");
        ui->txt_json_log->append(log);
    }
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
    bool result = config::spotify->do_refresh_token(log);
    apply_login_state(result, log);
}

void tuna_gui::on_tuna_gui_accepted()
{
    CSET_STR(CFG_COVER_PATH, ui->txt_song_cover->text().toStdString().c_str());
    CSET_STR(CFG_LYRICS_PATH, ui->txt_song_lyrics->text().toStdString().c_str());
    CSET_INT(CFG_SELECTED_SOURCE, ui->cb_source->currentIndex());
    CSET_UINT(CFG_REFRESH_RATE, ui->sb_refresh_rate->value());

    CSET_STR(CFG_SONG_PLACEHOLDER,
        ui->txt_song_placeholder->text().toStdString().c_str());
    CSET_BOOL(CFG_DOWNLOAD_COVER, ui->cb_dl_cover->isChecked());

    /* Source settings */
    CSET_STR(CFG_MPD_IP, qPrintable(ui->txt_ip->text()));
    CSET_UINT(CFG_MPD_PORT, ui->sb_port->value());
    CSET_BOOL(CFG_MPD_LOCAL, ui->cb_local->isChecked());

    CSET_STR(CFG_WINDOW_TITLE, ui->txt_title->text().toStdString().c_str());
    CSET_STR(CFG_WINDOW_PAUSE, ui->txt_paused->text().toStdString().c_str());
    CSET_STR(CFG_WINDOW_SEARCH, ui->txt_search->text().toStdString().c_str());
    CSET_STR(CFG_WINDOW_REPLACE, ui->txt_replace->text().toStdString().c_str());
    CSET_BOOL(CFG_WINDOW_REGEX, ui->cb_regex->isChecked());
    CSET_UINT(CFG_WINDOW_CUT_BEGIN, ui->sb_begin->value());
    CSET_UINT(CFG_WINDOW_CUT_END, ui->sb_end->value());

    config::outputs.clear();
    for (int row = 0; row < ui->tbl_outputs->rowCount(); row++) {
        config::outputs.push_back(QPair<QString, QString>(
            ui->tbl_outputs->item(row, 0)->text(),
            ui->tbl_outputs->item(row, 1)->text()));
    }
    config::save_outputs(config::outputs);
    thread::mutex.lock();
    config::refresh_rate = ui->sb_refresh_rate->value();
    thread::mutex.unlock();

    config::load();
}

void tuna_gui::on_apply_pressed()
{
    on_tuna_gui_accepted();
}

void tuna_gui::on_btn_start_clicked()
{
    if (!thread::start()) {
        QMessageBox::warning(this, "Error", "Thread couldn't be started!");
    }
    CSET_BOOL(CFG_RUNNING, thread::thread_state);
    set_state();
}

void tuna_gui::on_btn_stop_clicked()
{
    thread::stop();
    CSET_BOOL(CFG_RUNNING, thread::thread_state);
    set_state();
}

void tuna_gui::set_mpd_ip(const char* ip)
{
    ui->txt_ip->setText(ip);
}

void tuna_gui::set_mpd_port(uint16_t port)
{
    ui->sb_port->setValue(port);
}

void tuna_gui::set_mpd_local(bool state)
{
    ui->cb_local->setChecked(state);
}

void tuna_gui::set_spotify_auth_code(const char* str)
{
    ui->txt_auth_code->setText(str);
}

void tuna_gui::set_spotify_auth_token(const char* str)
{
    ui->txt_token->setText(str);
}

void tuna_gui::set_window_regex(bool state)
{
    ui->cb_regex->setChecked(state);
}

void tuna_gui::set_window_title(const char* str)
{
    ui->txt_title->setText(str);
}

void tuna_gui::set_window_search(const char* str)
{
    ui->txt_search->setText(str);
}

void tuna_gui::set_window_pause(const char* str)
{
    ui->txt_paused->setText(str);
}

void tuna_gui::set_window_replace(const char* str)
{
    ui->txt_replace->setText(str);
}

void tuna_gui::set_window_cut_begin(uint16_t n)
{
    ui->sb_begin->setValue(n);
}

void tuna_gui::set_window_cut_end(uint16_t n)
{
    ui->sb_end->setValue(n);
}

void tuna_gui::set_spotify_refresh_token(const char* str)
{
    ui->txt_refresh_token->setText(str);
}

void tuna_gui::on_btn_sp_show_token_pressed()
{
    ui->txt_token->setEchoMode(QLineEdit::Normal);
}

void tuna_gui::on_btn_sp_show_token_released()
{
    ui->txt_token->setEchoMode(QLineEdit::Password);
}

void tuna_gui::on_btn_sp_show_refresh_token_pressed()
{
    ui->txt_refresh_token->setEchoMode(QLineEdit::Normal);
}

void tuna_gui::on_btn_sp_show_refresh_token_released()
{
    ui->txt_refresh_token->setEchoMode(QLineEdit::Password);
}

void tuna_gui::on_checkBox_stateChanged(int arg1)
{
    ui->txt_ip->setEnabled(arg1 > 0);
    ui->sb_port->setEnabled(arg1 > 0);
}

void tuna_gui::on_btn_browse_song_cover_clicked()
{
    QString path;
    choose_file(path, T_SELECT_COVER_FILE, FILTER("Image file", "*.png"));
    if (!path.isEmpty())
        ui->txt_song_cover->setText(path);
}

void tuna_gui::on_btn_browse_song_lyrics_clicked()
{
    QString path;
    choose_file(path, T_SELECT_LYRICS_FILE, FILTER("Text file", "*.txt"));
    if (!path.isEmpty())
        ui->txt_song_lyrics->setText(path);
}

void tuna_gui::add_output(const QString& format, const QString& path)
{
    int row = ui->tbl_outputs->rowCount();
    ui->tbl_outputs->insertRow(row);
    ui->tbl_outputs->setItem(row, 0, new QTableWidgetItem(format));
    ui->tbl_outputs->setItem(row, 1, new QTableWidgetItem(path));
}

void tuna_gui::edit_output(const QString& format, const QString& path)
{
    auto selection = ui->tbl_outputs->selectedItems();
    if (!selection.empty() && selection.size() > 1) {
        selection.at(0)->setText(format);
        selection.at(1)->setText(path);
    }
}

void tuna_gui::on_btn_add_output_clicked()
{
    obs_frontend_push_ui_translation(obs_module_get_string);
    auto* dialog = new output_edit_dialog(edit_mode::create, this);
    obs_frontend_pop_ui_translation();
    dialog->exec();
}

void tuna_gui::on_btn_remove_output_clicked()
{
    auto* select = ui->tbl_outputs->selectionModel();
    if (select->hasSelection()) {
        auto rows = select->selectedRows();
        if (!rows.empty()) {
            ui->tbl_outputs->removeRow(rows.first().row());
        }
    }
}

void tuna_gui::get_selected_output(QString& format, QString& path)
{
    auto selection = ui->tbl_outputs->selectedItems();
    if (!selection.empty() && selection.size() > 1) {
        format = selection.at(0)->text();
        path = selection.at(1)->text();
    }
}

void tuna_gui::on_btn_edit_output_clicked()
{
    auto selection = ui->tbl_outputs->selectedItems();
    if (!selection.empty() && selection.size() > 1) {
        obs_frontend_push_ui_translation(obs_module_get_string);
        auto* dialog = new output_edit_dialog(edit_mode::modify, this);
        obs_frontend_pop_ui_translation();
        dialog->exec();
    }
}

void tuna_gui::on_cb_local_clicked(bool checked)
{
    ui->txt_ip->setEnabled(!checked);
    ui->sb_port->setEnabled(!checked);
}
