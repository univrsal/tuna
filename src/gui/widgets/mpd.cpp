/*************************************************************************
 * This file is part of tuna
 * github.com/univrsal/tuna
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

#include "mpd.hpp"
#include "../../util/config.hpp"
#include "../../util/constants.hpp"
#include "../../util/utility.hpp"
#include "ui_mpd.h"
#include <QFileDialog>

mpd::mpd(QWidget* parent)
    : source_widget(parent)
    , ui(new Ui::mpd)
{
    ui->setupUi(this);
}

mpd::~mpd()
{
    delete ui;
}

void mpd::load_settings()
{
    on_rb_remote_toggled(!CGET_BOOL(CFG_MPD_LOCAL));
    ui->txt_ip->setText(utf8_to_qt(CGET_STR(CFG_MPD_IP)));
    ui->sb_port->setValue(CGET_INT(CFG_MPD_PORT));
    ui->txt_base_folder->setText(utf8_to_qt(CGET_STR(CFG_MPD_BASE_FOLDER)));
}

void mpd::save_settings()
{
    CSET_STR(CFG_MPD_IP, qt_to_utf8(ui->txt_ip->text()));
    CSET_UINT(CFG_MPD_PORT, ui->sb_port->value());
    CSET_BOOL(CFG_MPD_LOCAL, ui->rb_local->isChecked());
    auto path = ui->txt_base_folder->text();
    if (!path.endsWith("/"))
        path.append("/");
    CSET_STR(CFG_MPD_BASE_FOLDER, qt_to_utf8(path));
}

void mpd::on_rb_remote_toggled(bool remote)
{
    ui->rb_local->setChecked(!remote);
    ui->txt_base_folder->setEnabled(!remote);
    ui->rb_local->setChecked(!remote);
    ui->txt_ip->setEnabled(remote);
    ui->sb_port->setEnabled(remote);
}

void mpd::on_btn_browse_base_folder_clicked()
{
    ui->txt_base_folder->setText(QFileDialog::getExistingDirectory(this, T_SELECT_MPD_FOLDER));
}
