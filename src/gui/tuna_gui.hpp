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

#pragma once

#include <QDialog>

namespace Ui {
class tuna_gui;
}

class tuna_gui : public QDialog {
    Q_OBJECT

    void load_vlc_sources();

public:
    explicit tuna_gui(QWidget* parent = nullptr);

    ~tuna_gui();

    void toggleShowHide();

    void add_output(const QString& format, const QString& path, bool log_mode);
    void edit_output(const QString& format, const QString& path, bool log_mode);
    void get_selected_output(QString& format, QString& path, bool& log_mode);

signals:
    void login_state_changed(bool sate, QString& log);
    void vlc_source_selected(const QString& id);
    void mpd_source_changed(const QString& ip, uint16_t port, bool local, const QString& base_folder);
    void window_source_changed(const QString& title, const QString& replace, const QString& with,
        const QString& pause_if, bool regex, uint16_t cut_begin, uint16_t cut_end);

    void source_registered(const QString& display, const QString& id);

private:
    void set_mpd_local(bool local) const;
private slots:

    /* Music source interactions */
    void select_vlc_source(const QString& id);
    void apply_login_state(bool state, const QString& log) const;
    void update_mpd(const QString& ip, uint16_t port, bool local, const QString& base_folder) const;
    void update_window(const QString& title, const QString& replace, const QString& with, const QString& pause_if,
        bool regex, uint16_t cut_begin, uint16_t cut_end) const;

    void add_music_source(const QString& display, const QString& id);

    /* Element interactions */
    void choose_file(QString& path, const char* title, const char* file_types);
    void on_apply_pressed();
    void on_btn_sp_show_auth_pressed();
    void on_btn_sp_show_auth_released();
    void on_btn_open_login_clicked();
    void on_txt_auth_code_textChanged(const QString& arg1) const;
    void on_btn_request_token_clicked();
    void on_btn_performrefresh_clicked();
    void on_tuna_gui_accepted();
    void on_btn_start_clicked();
    void set_state();
    void on_btn_stop_clicked();
    void on_btn_sp_show_token_pressed();
    void on_btn_sp_show_token_released();
    void on_btn_sp_show_refresh_token_pressed();
    void on_btn_sp_show_refresh_token_released();
    void on_checkBox_stateChanged(int arg1);
    void on_btn_browse_song_cover_clicked();
    void on_btn_browse_song_lyrics_clicked();
    void on_btn_add_output_clicked();
    void on_btn_remove_output_clicked();
    void on_btn_edit_output_clicked();
    void on_pb_refresh_vlc_clicked();
    void on_btn_browse_base_folder_clicked();
    void on_rb_remote_clicked(bool checked);
    void on_rb_local_clicked(bool checked);
    void on_btn_id_show_pressed();
    void on_btn_id_show_released();
    void on_btn_show_secret_pressed();
    void on_btn_show_secret_released();

private:
    Ui::tuna_gui* ui;
};

extern tuna_gui* tuna_dialog;
