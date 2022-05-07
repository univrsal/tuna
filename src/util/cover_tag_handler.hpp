/*************************************************************************
 * This file is part of tuna
 * github.com/univrsal/tuna
 * Copyright 2022 univrsal <uni@vrsal.xyz>.
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
#include <QString>

namespace cover {
/* Tries to get the song embbeded in the file */
extern bool find_embedded_cover(const QString& path);

/* Tries to find the cover in the folder that the file is located in */
extern bool find_local_cover(const QString& path, QString& cover_out);

/* Turns /home/usr/file.flac into /home/usr/ */
extern void get_file_folder(QString& path);
}
