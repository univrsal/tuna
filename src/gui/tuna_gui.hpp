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
#include <map>
#include <tuple>

namespace Ui {
class tuna_gui;
}

class source_widget : public QWidget {
public:
    explicit source_widget(QWidget* parent = nullptr)
        : QWidget(parent)
    {
    }
    virtual void save_settings() = 0;
    virtual void load_settings() = 0;
};

class tuna_gui : public QDialog {
    Q_OBJECT

    QList<source_widget*> m_source_widgets;

public:
    explicit tuna_gui(QWidget* parent = nullptr);

    ~tuna_gui();

    void toggleShowHide();
    void add_output(const QString& format, const QString& path, bool log_mode);
    void edit_output(const QString& format, const QString& path, bool log_mode);
    void get_selected_output(QString& format, QString& path, bool& log_mode);

signals:
    void source_registered(const QString& display, const QString& id, source_widget* w);

private slots:
    void add_music_source(const QString& display, const QString& id, source_widget* w);

    /* Element interactions */
    void choose_file(QString& path, const char* title, const char* file_types);
    void on_apply_pressed();
    void on_tuna_gui_accepted();
    void on_btn_start_clicked();
    void set_state();
    void on_btn_stop_clicked();
    void on_btn_browse_song_cover_clicked();
    void on_btn_browse_song_lyrics_clicked();
    void on_btn_add_output_clicked();
    void on_btn_remove_output_clicked();
    void on_btn_edit_output_clicked();

private:
    Ui::tuna_gui* ui;
};

extern tuna_gui* tuna_dialog;
