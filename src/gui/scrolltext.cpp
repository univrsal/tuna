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
 *************************************************************************/

#include "scrolltext.hpp"
#include <QPainter>

scroll_text::scroll_text(QWidget *parent) : QWidget(parent), m_scroll_pos(0)
{
	m_static_text.setTextFormat(Qt::PlainText);

	setFixedHeight(fontMetrics().height());
	m_left_margin = height() / 3;

	set_separator(" // ");

	connect(&m_timer, SIGNAL(timeout()), this, SLOT(timer_timeout()));
	m_timer.setInterval(50);
}

QString scroll_text::text() const
{
	return m_text;
}

void scroll_text::set_text(QString text)
{
	m_text = text;
	update_text();
	update();
}

QString scroll_text::separator() const
{
	return m_separator;
}

void scroll_text::set_separator(QString separator)
{
	m_separator = separator;
	update_text();
	update();
}

void scroll_text::update_text()
{
	m_timer.stop();
#if QT_VERSION_MINOR <= 10
#define horizontalAdvance width
#endif
	m_single_text_width = fontMetrics().horizontalAdvance(m_text);

	m_scroll_enabled = (m_single_text_width > width() - m_left_margin);

	if (m_scroll_enabled) {
		m_scroll_pos = -64;
		m_static_text.setText(m_text + m_separator);
		m_timer.start();
	} else
		m_static_text.setText(m_text);

	m_static_text.prepare(QTransform(), font());
	m_whole_text_size = QSize(fontMetrics().horizontalAdvance(m_static_text.text()), fontMetrics().height());
#undef horizontalAdvance
}

void scroll_text::paintEvent(QPaintEvent *)
{
	QPainter p(this);

	if (m_scroll_enabled) {
		m_buffer.fill(qRgba(0, 0, 0, 0));
		QPainter pb(&m_buffer);
		pb.setPen(p.pen());
		pb.setFont(p.font());

		int x = qMin(-m_scroll_pos, 0) + m_left_margin;
		while (x < width()) {
			pb.drawStaticText(QPointF(x, (height() - m_whole_text_size.height()) / 2), m_static_text);
			x += m_whole_text_size.width();
		}

		//Apply Alpha Channel
		pb.setCompositionMode(QPainter::CompositionMode_DestinationIn);
		pb.setClipRect(width() - 15, 0, 15, height());
		pb.drawImage(0, 0, m_alpha_channel);
		pb.setClipRect(0, 0, 15, height());
		//initial situation: don't apply alpha channel in the left half of the image at all; apply it more and more until scrollPos gets positive
		if (m_scroll_pos < 0)
			pb.setOpacity((qreal)(qMax(-8, m_scroll_pos) + 8) / 8.0);
		pb.drawImage(0, 0, m_alpha_channel);

		//pb.end();
		p.drawImage(0, 0, m_buffer);
	} else {
		p.drawStaticText(QPointF((width() - m_whole_text_size.width()) / 2,
								 (height() - m_whole_text_size.height()) / 2),
						 m_static_text);
	}
}

void scroll_text::resizeEvent(QResizeEvent *)
{
	//When the widget is resized, we need to update the alpha channel.

	m_alpha_channel = QImage(size(), QImage::Format_ARGB32_Premultiplied);
	m_buffer = QImage(size(), QImage::Format_ARGB32_Premultiplied);

	//Create Alpha Channel:
	if (width() > 64) {
		//create first scanline
		QRgb *scanline1 = (QRgb *)m_alpha_channel.scanLine(0);
		for (int x = 1; x < 16; ++x)
			scanline1[x - 1] = scanline1[width() - x] = qRgba(0, 0, 0, x << 4);
		for (int x = 15; x < width() - 15; ++x)
			scanline1[x] = qRgb(0, 0, 0);
		//copy scanline to the other ones
		for (int y = 1; y < height(); ++y)
			memcpy(m_alpha_channel.scanLine(y), (uchar *)scanline1, width() * 4);
	} else
		m_alpha_channel.fill(qRgb(0, 0, 0));

	//Update scrolling state
	bool newScrollEnabled = (m_single_text_width > width() - m_left_margin);
	if (newScrollEnabled != m_scroll_enabled)
		update_text();
}

void scroll_text::timer_timeout()
{
	m_scroll_pos = (m_scroll_pos + 1) % m_whole_text_size.width();
	update();
}
