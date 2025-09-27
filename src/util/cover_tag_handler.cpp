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

/* Parts of this file are taken from Rainmeter
 *  https://github.com/rainmeter/rainmeter/blob/master/Library/NowPlaying/Cover.cpp
 *  https://github.com/rainmeter/rainmeter/blob/39c2ed8bf0eefe411dbb8d94a8d82659dac341ab/Library/NowPlaying/Player.cpp
 */

#include "cover_tag_handler.hpp"
#include "../query/song.hpp"
#include "config.hpp"
#include "utility.hpp"
#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QFileInfo>
#include <taglib/apefile.h>
#include <taglib/apeitem.h>
#include <taglib/apetag.h>
#include <taglib/asffile.h>
#include <taglib/attachedpictureframe.h>
#include <taglib/fileref.h>
#include <taglib/flacfile.h>
#include <taglib/id3v2frame.h>
#include <taglib/id3v2tag.h>
#include <taglib/mp4file.h>
#include <taglib/mpcfile.h>
#include <taglib/mpegfile.h>
#include <taglib/opusfile.h>
#include <taglib/tlist.h>
#include <taglib/tmap.h>

namespace cover {

bool write_bytes_to_file(const TagLib::ByteVector& data)
{
    if (data.isEmpty())
        return false;
    QFile f(config::cover_path);
    bool success = true;
    if (f.open(QIODevice::WriteOnly)) {
        success = f.write(data.data(), data.size()) == data.size();
        f.close();
    } else {
        success = false;
    }

    return success;
}

bool extract_ape(TagLib::APE::Tag* tag)
{
    const TagLib::APE::ItemListMap& listMap = tag->itemListMap();
    if (listMap.contains("COVER ART (FRONT)")) {
        const TagLib::ByteVector nullStringTerminator(1, 0);
        // TODO: Does this even work?
        auto item = listMap["COVER ART (FRONT)"].toString();
        const int pos = item.find(nullStringTerminator); // Skip the filename.
        if (pos != -1) {
            const TagLib::ByteVector& pic = item.substr(pos + 1).data(TagLib::String::UTF8);
            return write_bytes_to_file(pic);
        }
    }

    return false;
}

bool extract_id3(TagLib::ID3v2::Tag* tag)
{
    const TagLib::ID3v2::FrameList& frameList = tag->frameList("APIC");
    if (!frameList.isEmpty()) {
        const auto* frame = (TagLib::ID3v2::AttachedPictureFrame*)frameList.front();
        return write_bytes_to_file(frame->picture());
    }
    return false;
}

bool extract_asf(TagLib::ASF::File* file)
{
    const TagLib::ASF::AttributeListMap& attrListMap = file->tag()->attributeListMap();
    if (attrListMap.contains("WM/Picture")) {
        const TagLib::ASF::AttributeList& attrList = attrListMap["WM/Picture"];
        if (!attrList.isEmpty()) {
            // Let's grab the first cover. TODO: Check/loop for correct type.
            const TagLib::ASF::Picture& wmpic = attrList[0].toPicture();
            if (wmpic.isValid()) {
                return write_bytes_to_file(wmpic.picture());
            }
        }
    }

    return false;
}

bool extract_flac(TagLib::FLAC::File* file)
{
    const TagLib::List<TagLib::FLAC::Picture*>& picList = file->pictureList();
    if (!picList.isEmpty()) {
        // Just grab the first image.
        const TagLib::FLAC::Picture* pic = picList[0];
        return write_bytes_to_file(pic->data());
    }

    return false;
}

bool extract_mp4(TagLib::MP4::File* file)
{
    TagLib::MP4::Tag* tag = file->tag();
    const TagLib::MP4::ItemMap& itemListMap = tag->itemMap();
    if (itemListMap.contains("covr")) {
        const TagLib::MP4::CoverArtList& coverArtList = itemListMap["covr"].toCoverArtList();
        if (!coverArtList.isEmpty()) {
            const TagLib::MP4::CoverArt* pic = &(coverArtList.front());
            return write_bytes_to_file(pic->data());
        }
    }

    return false;
}

bool extract_opus(TagLib::Ogg::Opus::File* file)
{
    auto* tag = file->tag();
    auto pictures = tag->pictureList();
    if (!pictures.isEmpty()) {
        /* I'll just assume that the last image is the one with the biggest size */
        return write_bytes_to_file(pictures[pictures.size() - 1]->data());
    }
    return false;
}

bool get_embedded(TagLib::FileRef fr)
{
    bool found = false;

    if (TagLib::MPEG::File* mpeg = dynamic_cast<TagLib::MPEG::File*>(fr.file())) {
        if (mpeg->hasID3v2Tag()) {
            found = extract_id3(mpeg->ID3v2Tag());
        } else if (mpeg->hasAPETag()) {
            found = extract_ape(mpeg->APETag());
        }
    } else if (TagLib::FLAC::File* flac = dynamic_cast<TagLib::FLAC::File*>(fr.file())) {
        found = extract_flac(flac);
        if (!found && flac->ID3v2Tag())
            found = extract_id3(flac->ID3v2Tag());
    } else if (TagLib::MP4::File* mp4 = dynamic_cast<TagLib::MP4::File*>(fr.file())) {
        found = extract_mp4(mp4);
    } else if (TagLib::ASF::File* asf = dynamic_cast<TagLib::ASF::File*>(fr.file())) {
        found = extract_asf(asf);
    } else if (TagLib::APE::File* ape = dynamic_cast<TagLib::APE::File*>(fr.file())) {
        if (ape->APETag())
            found = extract_ape(ape->APETag());
    } else if (TagLib::MPC::File* mpc = dynamic_cast<TagLib::MPC::File*>(fr.file())) {
        if (mpc->APETag())
            found = extract_ape(mpc->APETag());
    } else if (TagLib::Ogg::Opus::File* ogg = dynamic_cast<TagLib::Ogg::Opus::File*>(fr.file())) {
        if (ogg->tag())
            found = extract_opus(ogg);
    }

    return found;
}

bool find_embedded_cover(const QString& path)
{
    bool result = false;
#ifdef _WIN32
    // Windoze can't into utf8
    const auto wstr = path.toStdWString();
    const TagLib::FileRef fr(wstr.c_str(), false);
#else
    const TagLib::FileRef fr(qt_to_utf8(path), false);
#endif

    if (!fr.isNull())
        result = get_embedded(fr);
    return result;
}

bool find_local_cover(const QString& folder, QString& out)
{
    static QStringList exts = { "*.jpg", "*.jpeg", "*.png", "*.bmp" };
    QList<QString> OtherFiles;
    QDirIterator it(folder, exts, QDir::Files, QDirIterator::NoIteratorFlags);
    while (it.hasNext()) {
        if (it.next().contains("cover", Qt::CaseInsensitive)) {
            out = it.filePath();
            return true;
        }
        OtherFiles.append(it.filePath());
    }

    /* If we couldn't find a file named cover, we resort to picking the largest
     * image file in the folder
     */
    qint64 biggest_image_size = 0;
    QString biggest_image = "";

    for (const auto& file : OtherFiles) {
        QFileInfo info(file);
        if (info.size() > biggest_image_size) {
            biggest_image = file;
            biggest_image_size = info.size();
        }
    }

    if (!biggest_image.isEmpty()) {
        out = biggest_image;
        return true;
    }

    return false;
}

void get_file_folder(QString& path)
{
    QFileInfo fi(path);
    if (fi.isFile())
        path = fi.dir().path();
}
}
