/*************************************************************************
 * This file is part of tuna
 * github.com/univrsal/tuna
 * Copyright 2021 univrsal <uni@vrsal.de>.
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
 *
 *
 * Thanks StackOverflow, very cool
 * https://stackoverflow.com/a/10655396
 *************************************************************************/

#pragma once
#include <QStaticText>
#include <QTimer>
#include <QWidget>

class scroll_text : public QWidget {
	Q_OBJECT
	Q_PROPERTY(QString text READ text WRITE set_text)
	Q_PROPERTY(QString separator READ separator WRITE set_separator)

public:
	explicit scroll_text(QWidget *parent = nullptr);

	QString text() const;
	void set_text(QString text);

	QString separator() const;
	void set_separator(QString separator);

protected:
	virtual void paintEvent(QPaintEvent *);
	virtual void resizeEvent(QResizeEvent *);

private:
	void update_text();
	QString m_text;
	QString m_separator;
	QStaticText m_static_text;
	int m_single_text_width;
	QSize m_whole_text_size;
	int m_left_margin;
	bool m_scroll_enabled;
	int m_scroll_pos;
	QImage m_alpha_channel;
	QImage m_buffer;
	QTimer m_timer;

private slots:
	virtual void timer_timeout();
};
