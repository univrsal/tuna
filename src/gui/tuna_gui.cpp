/* tuna_gui.cpp created on 2019.8.8
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
#include "tuna_gui.hpp"
#include "ui_tuna_gui.h"

tuna_gui::tuna_gui(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::tuna_gui)
{
    ui->setupUi(this);
}

tuna_gui::~tuna_gui()
{
    delete ui;
}
