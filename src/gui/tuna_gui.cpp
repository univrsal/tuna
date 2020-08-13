/*************************************************************************
 * This file is part of tuna
 * github.con/univrsal/tuna
 * Copyright 2020 univrsal <universailp@web.de>.
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
#include "../query/vlc_obs_source.hpp"
#include "../util/config.hpp"
#include "../util/constants.hpp"
#include "../util/tuna_thread.hpp"
#include "../util/utility.hpp"
#include "music_control.hpp"
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
#include <exception>
#include <obs-frontend-api.h>
#include <obs-module.h>
#include <random>
#include <string>
#include <util/platform.h>

tuna_gui* tuna_dialog = nullptr;

tuna_gui::tuna_gui(QWidget* parent)
    : QDialog(parent)
    , ui(new Ui::tuna_gui)
{
    ui->setupUi(this);
    connect(ui->buttonBox->button(QDialogButtonBox::Apply), SIGNAL(clicked()), this, SLOT(on_apply_pressed()));

    /* Settings loading signals */
    connect(this, &tuna_gui::login_state_changed, this, &tuna_gui::apply_login_state);
    connect(this, &tuna_gui::vlc_source_selected, this, &tuna_gui::select_vlc_source);
    connect(this, &tuna_gui::window_source_changed, this, &tuna_gui::update_window);
    connect(this, &tuna_gui::source_registered, this, &tuna_gui::add_music_source);
    connect(this, &tuna_gui::mpd_source_changed, this, &tuna_gui::update_mpd);

    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);

    ui->settings_tabs->setCurrentIndex(0);

    /* Notify user, if vlc source is disabled */
    ui->lbl_vlc_disabled->setStyleSheet("QLabel { color: red;"
                                        "font-weight: bold; }");
    ui->lbl_vlc_disabled->setVisible(!util::vlc_loaded);
    ui->btn_refresh_vlc->setEnabled(util::vlc_loaded);
    ui->cb_vlc_source_name->setEnabled(util::vlc_loaded);

    /* TODO Lyrics */
    ui->frame_lyrics->setVisible(false);
}

void tuna_gui::choose_file(QString& path, const char* title, const char* file_types)
{
    path = QFileDialog::getSaveFileName(this, tr(title), QDir::home().path(), tr(file_types));
}

void tuna_gui::set_state()
{
    if (thread::thread_flag)
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
        load_vlc_sources();
        music_sources::set_gui_values();

        /* setup config values */
        ui->txt_client_id->setText(utf8_to_qt(CGET_STR(CFG_SPOTIFY_CLIENT_ID)));
        ui->txt_secret->setText(utf8_to_qt(CGET_STR(CFG_SPOTIFY_CLIENT_SECRET)));
        ui->txt_song_cover->setText(utf8_to_qt(config::cover_path));
        ui->txt_song_lyrics->setText(utf8_to_qt(config::lyrics_path));
        ui->sb_refresh_rate->setValue(config::refresh_rate);
        ui->txt_song_placeholder->setText(utf8_to_qt(config::placeholder));
        ui->cb_dl_cover->setChecked(config::download_cover);
        ui->cb_source->setCurrentIndex(ui->cb_source->findData(utf8_to_qt(config::selected_source)));
        set_state();

        const auto s = CGET_STR(CFG_SELECTED_SOURCE);
        auto i = 0;
        for (const auto& src : music_sources::instances) {
            if (strcmp(src->id(), s) == 0)
                break;
            i++;
        }
        ui->cb_source->setCurrentIndex(i);

        /* Load table contents */
        int row = 1; /* Clear all rows except headers */
        for (; row < ui->tbl_outputs->rowCount(); row++)
            ui->tbl_outputs->removeRow(row);
        row = 0; /* Load rows */
        ui->tbl_outputs->setRowCount(config::outputs.size());
        for (const auto& entry : config::outputs) {
            ui->tbl_outputs->setItem(row, 0, new QTableWidgetItem(entry.format));
            ui->tbl_outputs->setItem(row, 1, new QTableWidgetItem(entry.path));
            ui->tbl_outputs->setItem(row, 2, new QTableWidgetItem(entry.log_mode ? "Yes" : "No"));
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

    static QString base_url = "https://univrsal.github.io/auth/login?client_id=";
    auto client_id = ui->txt_client_id->text();
    if (client_id.isEmpty())
        client_id = "847d7cf0c5dc4ff185161d1f000a9d0e";
    QString url = base_url + client_id;
    QMessageBox::information(this, "Info", T_SPOTIFY_WARNING);
    QDesktopServices::openUrl(QUrl(url));
}

void tuna_gui::on_txt_auth_code_textChanged(const QString& arg1) const
{
    ui->btn_request_token->setEnabled(arg1.length() > 0);
}

void tuna_gui::apply_login_state(bool state, const QString& log) const
{
    if (state) {
        try {
            auto spotify = music_sources::get<spotify_source>(S_SOURCE_SPOTIFY);
            ui->txt_token->setText(spotify->token());
            ui->txt_refresh_token->setText(spotify->refresh_token());
            ui->txt_auth_code->setText(spotify->auth_code());
            spotify.reset();
        } catch (std::invalid_argument& e) {
            berr("apply_login_state failed with: %s", e.what());
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

void tuna_gui::update_mpd(const QString& ip, uint16_t port, bool local, const QString& base_folder) const
{
    ui->txt_ip->setText(ip);
    ui->sb_port->setValue(port);
    ui->btn_browse_base_folder->setEnabled(local);
    ui->txt_base_folder->setText(base_folder);
    set_mpd_local(local);
}

void tuna_gui::update_window(const QString& title, const QString& replace, const QString& with, const QString& pause_if,
    bool regex, uint16_t cut_begin, uint16_t cut_end) const
{
    ui->cb_regex->setChecked(regex);
    ui->txt_title->setText(title);
    ui->txt_search->setText(replace);
    ui->txt_replace->setText(with);
    ui->txt_paused->setText(pause_if);
    ui->sb_begin->setValue(cut_begin);
    ui->sb_end->setValue(cut_end);
}

void tuna_gui::add_music_source(const QString& display, const QString& id)
{
    ui->cb_source->addItem(display, id);
}

void tuna_gui::on_btn_request_token_clicked()
{
    /* Make sure that the custom data is in the config, otherwise the user
     * has to click apply before */
    CSET_STR(CFG_SPOTIFY_CLIENT_ID, qt_to_utf8(ui->txt_client_id->text()));
    CSET_STR(CFG_SPOTIFY_CLIENT_SECRET, qt_to_utf8(ui->txt_secret->text()));

    QString log;
    auto result = false;
    try {
        auto spotify = music_sources::get<spotify_source>(S_SOURCE_SPOTIFY);
        spotify->set_auth_code(ui->txt_auth_code->text());
        result = spotify->new_token(log);
        spotify.reset();
    } catch (std::invalid_argument& e) {
        log = "on_btn_request_clicked failed with: ";
        log += e.what();
        berr("on_btn_request_clicked failed with: %s", e.what());
    }
    apply_login_state(result, log);
}

void tuna_gui::on_btn_performrefresh_clicked()
{
    QString log;
    bool result = false;
    try {
        auto spotify = music_sources::get<spotify_source>(S_SOURCE_SPOTIFY);
        spotify->set_auth_code(ui->txt_auth_code->text());
        result = spotify->do_refresh_token(log);
        spotify.reset();
    } catch (std::invalid_argument& e) {
        log = "on_btn_performrefresh_clicked failed with: ";
        log += e.what();
        berr("on_btn_performrefresh_clicked failed with: %s", e.what());
    }
    apply_login_state(result, log);
}

void tuna_gui::on_tuna_gui_accepted()
{
    CSET_STR(CFG_COVER_PATH, qt_to_utf8(ui->txt_song_cover->text()));
    CSET_STR(CFG_LYRICS_PATH, qt_to_utf8(ui->txt_song_lyrics->text()));
    QString tmp = ui->cb_source->currentData().toByteArray();
    CSET_STR(CFG_SELECTED_SOURCE, tmp.toStdString().c_str());
    CSET_UINT(CFG_REFRESH_RATE, ui->sb_refresh_rate->value());

    CSET_STR(CFG_SONG_PLACEHOLDER, qt_to_utf8(ui->txt_song_placeholder->text()));
    CSET_BOOL(CFG_DOWNLOAD_COVER, ui->cb_dl_cover->isChecked());

    CSET_STR(CFG_SPOTIFY_CLIENT_ID, qt_to_utf8(ui->txt_client_id->text()));
    CSET_STR(CFG_SPOTIFY_CLIENT_SECRET, qt_to_utf8(ui->txt_secret->text()));

    /* Source settings */
    CSET_STR(CFG_MPD_IP, qt_to_utf8(ui->txt_ip->text()));
    CSET_UINT(CFG_MPD_PORT, ui->sb_port->value());
    CSET_BOOL(CFG_MPD_LOCAL, ui->rb_local->isChecked());
    auto path = ui->txt_base_folder->text();
    if (!path.endsWith("/"))
        path.append("/");
    CSET_STR(CFG_MPD_BASE_FOLDER, qt_to_utf8(path));

    CSET_STR(CFG_WINDOW_TITLE, qt_to_utf8(ui->txt_title->text()));
    CSET_STR(CFG_WINDOW_PAUSE, qt_to_utf8(ui->txt_paused->text()));
    CSET_STR(CFG_WINDOW_SEARCH, qt_to_utf8(ui->txt_search->text()));
    CSET_STR(CFG_WINDOW_REPLACE, qt_to_utf8(ui->txt_replace->text()));
    CSET_BOOL(CFG_WINDOW_REGEX, ui->cb_regex->isChecked());
    CSET_UINT(CFG_WINDOW_CUT_BEGIN, ui->sb_begin->value());
    CSET_UINT(CFG_WINDOW_CUT_END, ui->sb_end->value());

    CSET_STR(CFG_VLC_ID, qt_to_utf8(ui->cb_vlc_source_name->currentText()));

    config::outputs.clear();
    for (int row = 0; row < ui->tbl_outputs->rowCount(); row++) {
        config::output tmp;
        tmp.format = ui->tbl_outputs->item(row, 0)->text();
        tmp.path = ui->tbl_outputs->item(row, 1)->text();
        tmp.log_mode = ui->tbl_outputs->item(row, 2)->text() == "Yes";
        config::outputs.push_back(tmp);
    }

    config::save_outputs(config::outputs);
    thread::thread_mutex.lock();
    config::refresh_rate = ui->sb_refresh_rate->value();
    thread::thread_mutex.unlock();

    config::load();

    if (music_control) {
        emit music_control->source_changed();
        emit music_control->thread_changed();
    }
}

void tuna_gui::on_apply_pressed()
{
    on_tuna_gui_accepted();
}

void tuna_gui::on_btn_start_clicked()
{
    if (!thread::start())
        QMessageBox::warning(this, "Error", "Thread couldn't be started!");
    CSET_BOOL(CFG_RUNNING, thread::thread_flag);
    set_state();
}

void tuna_gui::on_btn_stop_clicked()
{
    thread::stop();
    CSET_BOOL(CFG_RUNNING, thread::thread_flag);
    set_state();
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

void tuna_gui::add_output(const QString& format, const QString& path, bool log_mode)
{
    int row = ui->tbl_outputs->rowCount();
    ui->tbl_outputs->insertRow(row);
    ui->tbl_outputs->setItem(row, 0, new QTableWidgetItem(format));
    ui->tbl_outputs->setItem(row, 1, new QTableWidgetItem(path));
    ui->tbl_outputs->setItem(row, 2, new QTableWidgetItem(log_mode ? "Yes" : "No"));
}

void tuna_gui::edit_output(const QString& format, const QString& path, bool log_mode)
{
    auto selection = ui->tbl_outputs->selectedItems();
    if (!selection.empty() && selection.size() > 1) {
        selection.at(0)->setText(format);
        selection.at(1)->setText(path);
        selection.at(2)->setText(log_mode ? "Yes" : "No");
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

void tuna_gui::get_selected_output(QString& format, QString& path, bool& log_mode)
{
    auto selection = ui->tbl_outputs->selectedItems();
    if (!selection.empty() && selection.size() > 1) {
        format = selection.at(0)->text();
        path = selection.at(1)->text();
        log_mode = selection.at(2)->text() == "Yes";
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

static bool add_source(void* data, obs_source_t* src)
{
    auto* id = obs_source_get_id(src);
    if (strcmp(id, "vlc_source") == 0) {
        auto* name = obs_source_get_name(src);
        QComboBox* cb = reinterpret_cast<QComboBox*>(data);
        cb->addItem(utf8_to_qt(name));
    }
    return true;
}

void tuna_gui::load_vlc_sources()
{
    ui->cb_vlc_source_name->clear();
    ui->cb_vlc_source_name->addItem(T_VLC_NONE);
    obs_enum_sources(add_source, ui->cb_vlc_source_name);
}

void tuna_gui::on_pb_refresh_vlc_clicked()
{
    load_vlc_sources();
}

void tuna_gui::set_mpd_local(bool local) const
{
    ui->rb_local->setChecked(local);
    ui->rb_remote->setChecked(!local);
    ui->txt_ip->setEnabled(!local);
    ui->sb_port->setEnabled(!local);
    ui->txt_base_folder->setEnabled(local);
}

void tuna_gui::select_vlc_source(const QString& id)
{
    const auto idx = ui->cb_vlc_source_name->findText(id, Qt::MatchExactly);

    if (idx >= 0)
        ui->cb_vlc_source_name->setCurrentIndex(idx);
    else
        ui->cb_vlc_source_name->setCurrentIndex(0);
}

void tuna_gui::on_btn_browse_base_folder_clicked()
{
    ui->txt_base_folder->setText(QFileDialog::getExistingDirectory(this, T_SELECT_MPD_FOLDER));
}

void tuna_gui::on_rb_remote_clicked(bool checked)
{
    set_mpd_local(!checked);
}

void tuna_gui::on_rb_local_clicked(bool checked)
{
    set_mpd_local(checked);
}

void tuna_gui::on_btn_id_show_pressed()
{
    ui->txt_client_id->setEchoMode(QLineEdit::Normal);
}

void tuna_gui::on_btn_id_show_released()
{
    ui->txt_client_id->setEchoMode(QLineEdit::Password);
}

void tuna_gui::on_btn_show_secret_pressed()
{
    ui->txt_secret->setEchoMode(QLineEdit::Normal);
}

void tuna_gui::on_btn_show_secret_released()
{
    ui->txt_secret->setEchoMode(QLineEdit::Password);
}
