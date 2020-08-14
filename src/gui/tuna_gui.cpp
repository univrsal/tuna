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
#include "../query/vlc_obs_source.hpp"
#include "../util/config.hpp"
#include "../util/constants.hpp"
#include "../util/tuna_thread.hpp"
#include "../util/utility.hpp"
#include "music_control.hpp"
#include "output_edit_dialog.hpp"
#include "ui_tuna_gui.h"
#include <QFileDialog>
#include <QMessageBox>
#include <QString>
#include <obs-frontend-api.h>
#include <string>
#include <util/platform.h>

tuna_gui* tuna_dialog = nullptr;

tuna_gui::tuna_gui(QWidget* parent)
    : QDialog(parent)
    , ui(new Ui::tuna_gui)
{
    ui->setupUi(this);
    connect(ui->buttonBox->button(QDialogButtonBox::Apply), SIGNAL(clicked()), this, SLOT(on_apply_pressed()));
    connect(this, &tuna_gui::source_registered, this, &tuna_gui::add_music_source);

    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);

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
        music_sources::set_gui_values();

        /* load basic values */
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

void tuna_gui::add_music_source(const QString& display, const QString& id, source_widget* w)
{
    ui->cb_source->addItem(display, id);
    ui->settings_tabs->insertTab(1, w, display);
    m_source_widgets.append(w);
}

void tuna_gui::on_tuna_gui_accepted()
{
    QString tmp = ui->cb_source->currentData().toByteArray();
    CSET_STR(CFG_SELECTED_SOURCE, tmp.toStdString().c_str());
    CSET_STR(CFG_COVER_PATH, qt_to_utf8(ui->txt_song_cover->text()));
    CSET_STR(CFG_LYRICS_PATH, qt_to_utf8(ui->txt_song_lyrics->text()));
    CSET_UINT(CFG_REFRESH_RATE, ui->sb_refresh_rate->value());
    CSET_STR(CFG_SONG_PLACEHOLDER, qt_to_utf8(ui->txt_song_placeholder->text()));
    CSET_BOOL(CFG_DOWNLOAD_COVER, ui->cb_dl_cover->isChecked());

    /* save outputs */
    config::outputs.clear();
    for (int row = 0; row < ui->tbl_outputs->rowCount(); row++) {
        config::output tmp;
        tmp.format = ui->tbl_outputs->item(row, 0)->text();
        tmp.path = ui->tbl_outputs->item(row, 1)->text();
        tmp.log_mode = ui->tbl_outputs->item(row, 2)->text() == "Yes";
        config::outputs.push_back(tmp);
    }
    config::save_outputs();

    for (auto& w : m_source_widgets) {
        if (w)
            w->save_settings();
    }

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
