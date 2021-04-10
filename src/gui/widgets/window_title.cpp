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

#include "window_title.hpp"
#include "../../util/config.hpp"
#include "../../util/constants.hpp"
#include "../../util/utility.hpp"
#include "../../util/window/window_helper.hpp"
#include "ui_window_title.h"

window_title::window_title(QWidget *parent) : source_widget(parent), ui(new Ui::window_title)
{
	ui->setupUi(this);
	refresh_process_list();
}

window_title::~window_title()
{
	delete ui;
}

void window_title::refresh_process_list()
{
	ui->cb_procress_list->clear();
	ui->cb_procress_list->addItem(T_VLC_NONE);
	m_items.clear();
	GetWindowAndExeList(m_items);
	int count = 0;
	for (const auto &p : m_items) {
		QString t = utf8_to_qt(("[" + p.first + "] " + p.second).c_str());
		ui->cb_procress_list->addItem(t, count++);
	}
}

void window_title::load_settings()
{
	ui->cb_regex->setChecked(CGET_BOOL(CFG_WINDOW_REGEX));
	ui->txt_title->setText(utf8_to_qt(CGET_STR(CFG_WINDOW_TITLE)));
	ui->txt_search->setText(utf8_to_qt(CGET_STR(CFG_WINDOW_SEARCH)));
	ui->txt_replace->setText(utf8_to_qt(CGET_STR(CFG_WINDOW_REPLACE)));
	ui->txt_paused->setText(utf8_to_qt(CGET_STR(CFG_WINDOW_PAUSE)));
	ui->sb_begin->setValue(CGET_INT(CFG_WINDOW_CUT_BEGIN));
	ui->sb_end->setValue(CGET_INT(CFG_WINDOW_CUT_END));

	if (CGET_BOOL(CFG_WINDOW_USE_PROCRESS)) {
		ui->rb_process_name->setChecked(true);
		on_rb_process_name_clicked(true);
	} else {
		ui->rb_window_title->setChecked(true);
		on_rb_window_title_clicked(true);
	}
}

void window_title::save_settings()
{
	CSET_STR(CFG_WINDOW_TITLE, qt_to_utf8(ui->txt_title->text()));
	CSET_STR(CFG_WINDOW_PAUSE, qt_to_utf8(ui->txt_paused->text()));
	CSET_STR(CFG_WINDOW_SEARCH, qt_to_utf8(ui->txt_search->text()));
	CSET_STR(CFG_WINDOW_REPLACE, qt_to_utf8(ui->txt_replace->text()));
	CSET_BOOL(CFG_WINDOW_REGEX, ui->cb_regex->isChecked());
	CSET_UINT(CFG_WINDOW_CUT_BEGIN, ui->sb_begin->value());
	CSET_UINT(CFG_WINDOW_CUT_END, ui->sb_end->value());
	CSET_BOOL(CFG_WINDOW_USE_PROCRESS, ui->rb_process_name->isChecked());
	auto p = ui->cb_procress_list->currentData().toUInt();
	if (p < m_items.size())
		CSET_STR(CFG_WINDOW_PROCESS_NAME, m_items[p].first.c_str());
}

void window_title::on_rb_process_name_clicked(bool checked)
{
	ui->cb_procress_list->setEnabled(checked);
	ui->btn_refresh->setEnabled(checked);
	ui->txt_title->setEnabled(!checked);
	ui->txt_paused->setEnabled(!checked);
}

void window_title::on_rb_window_title_clicked(bool checked)
{
	ui->cb_procress_list->setEnabled(!checked);
	ui->btn_refresh->setEnabled(!checked);
	ui->txt_title->setEnabled(checked);
	ui->txt_paused->setEnabled(checked);
}

void window_title::on_btn_refresh_clicked()
{
	refresh_process_list();
}
