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

	void apply_login_state(bool state, const QString& log);

	void select_vlc_source(const char* id);

	void set_spotify_auth_code(const char* str);

	void set_spotify_auth_token(const char* str);

	void set_spotify_refresh_token(const char* str);

	void set_mpd_ip(const char* ip);

	void set_mpd_port(uint16_t port);

	void set_mpd_local(bool state);

	void set_window_title(const char* str);

	void set_window_search(const char* str);

	void set_window_pause(const char* str);

	void set_window_replace(const char* str);

	void set_window_cut_begin(uint16_t n);

	void set_window_cut_end(uint16_t n);

	void set_window_regex(bool state);

	void add_output(const QString& format, const QString& path);

	void edit_output(const QString& format, const QString& path);

	void get_selected_output(QString& format, QString& path);
private slots:

    void choose_file(QString& path, const char* title, const char* file_types);

    void on_apply_pressed();

    void on_btn_sp_show_auth_pressed();

    void on_btn_sp_show_auth_released();

    void on_btn_open_login_clicked();

    void on_txt_auth_code_textChanged(const QString& arg1);

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

    void on_cb_local_clicked(bool checked);

    void on_pb_refresh_vlc_clicked();

private:
    Ui::tuna_gui* ui;
};

extern tuna_gui* tuna_dialog;
