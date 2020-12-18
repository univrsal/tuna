/*************************************************************************
 * This file is part of tuna
 * github.con/univrsal/tuna
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

#include "deezer_source.hpp"
#include "../gui/widgets/deezer.hpp"

#define DEEZER_TOKEN_URL "https://connect.deezer.com/oauth/access_token.php?app_id=%1&secret=%2&code=%3"

deezer_source::deezer_source()
    : music_source(S_SOURCE_DEEZER, T_SOURCE_DEEZER, new deezer)
{
}

void deezer_source::load()
{
}

void deezer_source::refresh()
{
}
