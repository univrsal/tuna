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

#include "vlc.hpp"
#include "../../util/config.hpp"
#include "../../util/constants.hpp"
#include "../../util/utility.hpp"
#include "ui_vlc.h"

vlc::vlc(QWidget *parent) : source_widget(parent), ui(new Ui::vlc)
{
	ui->setupUi(this);
}

vlc::~vlc()
{
	delete ui;
}

void vlc::load_settings()
{
	load_vlc_sources();
	select_vlc_source(utf8_to_qt(CGET_STR(CFG_VLC_ID)));
	ui->btn_refresh_vlc->setEnabled(util::have_vlc_source);
	ui->cb_vlc_source_name->setEnabled(util::have_vlc_source);
}

void vlc::save_settings()
{
	CSET_STR(CFG_VLC_ID, qt_to_utf8(ui->cb_vlc_source_name->currentText()));
}

static bool add_source(void *data, obs_source_t *src)
{
	auto *id = obs_source_get_id(src);
	if (strcmp(id, "vlc_source") == 0) {
		auto *name = obs_source_get_name(src);
		QComboBox *cb = reinterpret_cast<QComboBox *>(data);
		cb->addItem(utf8_to_qt(name));
	}
	return true;
}

void vlc::load_vlc_sources()
{
	ui->cb_vlc_source_name->clear();
	ui->cb_vlc_source_name->addItem(T_VLC_NONE);
	obs_enum_sources(add_source, ui->cb_vlc_source_name);
}

void vlc::on_btn_refresh_vlc_clicked()
{
	load_vlc_sources();
}

void vlc::select_vlc_source(const QString &id)
{
	const auto idx = ui->cb_vlc_source_name->findText(id, Qt::MatchExactly);

	if (idx >= 0)
		ui->cb_vlc_source_name->setCurrentIndex(idx);
	else
		ui->cb_vlc_source_name->setCurrentIndex(0);
}
