/*************************************************************************
 * This file is part of tuna
 * github.con/univrsal/tuna
 * Copyright 2020 univrsal <universailp@web.de>.
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
#include <vector>

class song;
class specifier;

namespace format {

void init();
void execute(QString& out);

class specifier {
protected:
    char m_id;
    int m_tag_id;

public:
    specifier() = default;
    specifier(char id, int tag_id)
        : m_id(id)
        , m_tag_id(tag_id)
    {
    }

    virtual bool do_format(QString& slice, const song* s) const;
    bool replace(QString& slice, const song* s, const QString& data = "") const;

    char get_id() const { return m_id; }
};

class specifier_time : public specifier {
public:
    specifier_time(char id, int tag_id)
        : specifier(id, tag_id)
    {
    }

    bool do_format(QString& slice, const song* s) const override;
};

class specifier_int : public specifier {
public:
    specifier_int(char id, int tag_id)
        : specifier(id, tag_id)
    {
    }

    bool do_format(QString& slice, const song* s) const override;
};

class specifier_string : public specifier {
public:
    specifier_string(char id, int tag_id)
        : specifier(id, tag_id)
    {
    }

    bool do_format(QString& slice, const song* s) const override;
};

class specifier_static : public specifier {
    QString m_static_value;

public:
    specifier_static(char id, const QString& value)
        : specifier(id, 0)
        , m_static_value(value)
    {
    }

    bool do_format(QString& slice, const song* s) const override;
};

class specifier_string_list : public specifier {
    const QList<QString>* m_data;

public:
    specifier_string_list(char id, int tag_id)
        : specifier(id, tag_id)
    {
    }

    bool do_format(QString& slice, const song* s) const override;
};

class specifier_date : public specifier {
public:
    specifier_date(char id, int tag_id)
        : specifier(id, tag_id)
    {
    }

    bool do_format(QString& slice, const song* s) const override;
};

}
