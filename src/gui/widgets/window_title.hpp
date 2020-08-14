/* window_title.hpp created on 2020.8.14
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

namespace Ui {
class window_title;
}

class window_title : public source_widget {
    Q_OBJECT

public:
    explicit window_title(QWidget* parent = nullptr);
    ~window_title();

    void save_settings() override;
    void load_settings() override;

private:
    Ui::window_title* ui;
};
