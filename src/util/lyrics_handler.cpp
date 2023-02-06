/*************************************************************************
 * This file is part of tuna
 * git.vrsal.xyz/alex/tuna
 * Copyright 2023 univrsal <uni@vrsal.xyz>.
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

#include "lyrics_handler.hpp"
#include "utility.hpp"
#include <taglib/fileref.h>
#include <taglib/tpropertymap.h>
namespace lyrics {

bool download_missing_lyrics(const song&)
{
    /* TODO */
    return false;
}

bool get_embedded(TagLib::FileRef fr)
{
    bool found = false;
    TagLib::PropertyMap tags = fr.file()->properties();

    for (TagLib::PropertyMap::ConstIterator i = tags.begin(); i != tags.end(); ++i) {
        for (TagLib::StringList::ConstIterator j = i->second.begin(); j != i->second.end(); ++j) {
            if (utf8_to_qt(i->first.toCString(true)).toLower().contains("lyrics")) {
                return util::write_lyrics(utf8_to_qt((*j).toCString(true)));
            }
        }
    }
    return found;
}

bool find_embedded_lyrics(const QString& path)
{
#ifdef _WIN32
    // Windoze can't into utf8
    const auto wstr = path.toStdWString();
    const TagLib::FileRef fr(wstr.c_str(), false);
#else
    const TagLib::FileRef fr(qt_to_utf8(path), false);
#endif

    if (fr.isNull())
        return false;

    return get_embedded(fr);
}

}
