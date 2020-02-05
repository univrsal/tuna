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

#include "window_source.hpp"
#include "../gui/tuna_gui.hpp"
#include "../util/config.hpp"
#include "../util/utility.hpp"
#include "../util/window/window_helper.hpp"
#include <QRegularExpression>

window_source::window_source()
{
    m_capabilities = CAP_TITLE;
}

void window_source::load()
{
    CDEF_STR(CFG_WINDOW_TITLE, "");
    CDEF_BOOL(CFG_WINDOW_REGEX, false);
    CDEF_STR(CFG_WINDOW_SEARCH, "");
    CDEF_STR(CFG_WINDOW_REPLACE, "");
    CDEF_STR(CFG_WINDOW_PAUSE, "");
    CDEF_UINT(CFG_WINDOW_CUT_BEGIN, 0);
    CDEF_UINT(CFG_WINDOW_CUT_END, 0);

    m_title = CGET_STR(CFG_WINDOW_TITLE);
    m_regex = CGET_BOOL(CFG_WINDOW_REGEX);
    m_search = CGET_STR(CFG_WINDOW_SEARCH);
    m_replace = CGET_STR(CFG_WINDOW_REPLACE);
    m_pause = CGET_STR(CFG_WINDOW_PAUSE);
    m_cut_begin = CGET_UINT(CFG_WINDOW_CUT_BEGIN);
    m_cut_end = CGET_UINT(CFG_WINDOW_CUT_END);
}

void window_source::save()
{
    CSET_STR(CFG_WINDOW_TITLE, util::qcstr(m_title));
    CSET_STR(CFG_WINDOW_SEARCH, util::qcstr(m_search));
    CSET_STR(CFG_WINDOW_REPLACE, util::qcstr(m_replace));
    CSET_STR(CFG_WINDOW_PAUSE, util::qcstr(m_pause));
    CSET_BOOL(CFG_WINDOW_REGEX, m_regex);
    CSET_UINT(CFG_WINDOW_CUT_BEGIN, m_cut_begin);
    CSET_UINT(CFG_WINDOW_CUT_END, m_cut_begin);
}

void window_source::refresh()
{
    if (m_title.isEmpty())
        return;

    std::vector<std::string> window_titles;
    GetWindowList(window_titles);
    QRegularExpression regex(m_title);
    QString result = "";

    for (const auto& title : window_titles) {
        bool matches = false;
        if (m_regex) {
            matches = regex.match(title.c_str()).hasMatch();
        } else {
            /* Direct search */
            matches = title.find(m_title.toStdString()) != std::string::npos;
            if (matches && !m_pause.isEmpty() && title.find(m_pause.toStdString()) != std::string::npos)
                matches = false;
        }

        if (matches) {
            result = title.c_str();
            break;
        }
    }

    m_current.clear();
    if (result.isEmpty()) {
        m_current.set_playing(false);
    } else {
        /* Replace & cut */
        result.replace(m_search, m_replace);
        if (0 < m_cut_end + m_cut_begin && m_cut_end + m_cut_begin < result.length())
            result = result.mid(m_cut_begin, result.length() - m_cut_begin - m_cut_end);

        m_current.set_playing(true);
        m_current.set_title(result);
    }
}

bool window_source::execute_capability(capability c)
{
    /* NO-OP */
    return true;
}

bool window_source::valid_format(const QString& str)
{
    QRegularExpression reg("/%[m|M]|%[a|A]|%[r|R]|%[y|Y]|%[d|D]|%[n|N]"
                           "%[p|P]%[l|L]/gm");
    return !reg.match(str).hasMatch();
}

void window_source::load_gui_values()
{
    tuna_dialog->set_window_regex(m_regex);
    tuna_dialog->set_window_title(m_title);
    tuna_dialog->set_window_search(m_search);
    tuna_dialog->set_window_replace(m_replace);
    tuna_dialog->set_window_pause(m_pause);
    tuna_dialog->set_window_cut_begin(m_cut_begin);
    tuna_dialog->set_window_cut_end(m_cut_end);
}
