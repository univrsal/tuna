/* lastfm.hpp created on 2020.8.14
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 * github.com/univrsal/
 *
 */
#pragma once

#include "../tuna_gui.hpp"
#include <QWidget>

namespace Ui {
class lastfm;
}

class lastfm : public source_widget {
    Q_OBJECT

public:
    explicit lastfm(QWidget* parent = nullptr);
    ~lastfm();

    void load_settings() override;
    void save_settings() override;

private:
    Ui::lastfm* ui;
};
