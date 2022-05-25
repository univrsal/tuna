/*************************************************************************
 * This file is part of tuna
 * git.vrsal.xyz/alex/tuna
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
#include "../query/song.hpp"
#include "utility.hpp"
#include <QString>
#include <functional>
#include <memory>
#include <vector>

class song;

namespace format {

void init();
bool execute(QString& out);

class specifier {
protected:
    QString m_id {};
    std::function<QString(const song&)> m_data_getter {};
    std::vector<meta::type> m_required_caps {};

public:
    virtual ~specifier() = default;
    specifier() = default;

    specifier(const char* id, std::vector<meta::type> caps, std::function<QString(const song&)> data_getter)
        : m_id(id)
        , m_data_getter(data_getter)
        , m_required_caps(caps)
    {
    }

    specifier(const char* id, meta::type cap, std::function<QString(const song&)> data_getter)
        : m_id(id)
        , m_data_getter(data_getter)
        , m_required_caps({ cap })
    {
    }

    const QString& get_id() const { return m_id; }
    QString get_name() const
    {
        auto Tmp = "tuna.format." + m_id;
        return utf8_to_qt(obs_module_text(qt_to_utf8(Tmp)));
    }

    QString get_data(song const& s) const { return m_data_getter(s); }

    virtual bool for_encoding() const { return true; }

    std::vector<meta::type> const& get_required_caps() const { return m_required_caps; }
};

class static_specifier : public specifier {
public:
    static_specifier(const char* id, std::function<QString(const song&)> data_getter)
        : specifier(id, meta::NONE, data_getter)
    {
    }
    bool for_encoding() const override { return false; }
};

extern const std::vector<std::unique_ptr<specifier>>& get_specifiers();

}
