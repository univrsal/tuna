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
    uint32_t m_required_caps {};

public:
    virtual ~specifier() = default;
    specifier() = default;
    specifier(const char* id, uint32_t caps, std::function<QString(const song&)> data_getter)
        : m_id(id)
        , m_data_getter(data_getter)
        , m_required_caps(caps)
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

    uint32_t get_required_caps() const { return m_required_caps; }
};

class static_specifier : public specifier {
public:
    static_specifier(const char* id, std::function<QString(const song&)> data_getter)
        : specifier(id, 0, data_getter)
    {
    }
    bool for_encoding() const override { return false; }
};

/*
class specifier_time : public specifier {
public:
    specifier_time(char id, int tag_id)
        : specifier(id, tag_id)
    {
    }

    bool do_format(QString& slice, const song& s) const override;
};

class specifier_int : public specifier {
public:
    specifier_int(char id, int tag_id)
        : specifier(id, tag_id)
    {
    }

    bool do_format(QString& slice, const song& s) const override;
};

class specifier_string : public specifier {
public:
    specifier_string(char id, int tag_id)
        : specifier(id, tag_id)
    {
    }

    bool do_format(QString& slice, const song& s) const override;
};

class specifier_static : public specifier {
    QString m_static_value;

public:
    specifier_static(char id, const QString& value)
        : specifier(id, 0)
        , m_static_value(value)
    {
    }

    bool do_format(QString& slice, const song& s) const override;
};

class specifier_string_list : public specifier {
    const QList<QString>* m_data;

public:
    specifier_string_list(char id, int tag_id)
        : specifier(id, tag_id)
    {
    }

    bool do_format(QString& slice, const song& s) const override;
};

class specifier_date : public specifier {
public:
    specifier_date(char id, int tag_id)
        : specifier(id, tag_id)
    {
    }

    bool do_format(QString& slice, const song& s) const override;
};
*/
extern const std::vector<std::unique_ptr<specifier>>& get_specifiers();

}
