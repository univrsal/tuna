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

#include "window_source.hpp"
#include "../gui/tuna_gui.hpp"
#include "../gui/widgets/window_title.hpp"
#include "../util/config.hpp"
#include "../util/constants.hpp"
#include "../util/utility.hpp"
#include "../util/window/window_helper.hpp"
#include <QRegularExpression>

window_source::window_source()
    : music_source(S_SOURCE_WINDOW_TITLE, T_SOURCE_WINDOW_TITLE, new window_title)
{
    m_capabilities = CAP_TITLE;
}

bool window_source::enabled() const
{
    return true;
}

void window_source::load()
{
    music_source::load();
    CDEF_STR(CFG_WINDOW_TITLE, "");
    CDEF_BOOL(CFG_WINDOW_REGEX, false);
    CDEF_STR(CFG_WINDOW_SEARCH, "");
    CDEF_STR(CFG_WINDOW_REPLACE, "");
    CDEF_STR(CFG_WINDOW_PAUSE, "");
    CDEF_UINT(CFG_WINDOW_CUT_BEGIN, 0);
    CDEF_UINT(CFG_WINDOW_CUT_END, 0);
    CDEF_STR(CFG_WINDOW_PROCESS_NAME, "");
    CDEF_BOOL(CFG_WINDOW_USE_PROCRESS, false);

    m_title = utf8_to_qt(CGET_STR(CFG_WINDOW_TITLE));
    m_regex = CGET_BOOL(CFG_WINDOW_REGEX);
    m_search = utf8_to_qt(CGET_STR(CFG_WINDOW_SEARCH));
    m_replace = utf8_to_qt(CGET_STR(CFG_WINDOW_REPLACE));
    m_pause = utf8_to_qt(CGET_STR(CFG_WINDOW_PAUSE));
    m_cut_begin = CGET_UINT(CFG_WINDOW_CUT_BEGIN);
    m_cut_end = CGET_UINT(CFG_WINDOW_CUT_END);
    m_use_process_name = CGET_BOOL(CFG_WINDOW_USE_PROCRESS);
    m_process_name = utf8_to_qt(CGET_STR(CFG_WINDOW_PROCESS_NAME));
}

QString window_source::get_title(const vector<string>& windows)
{
    QRegularExpression regex(m_title);
    QString result = "";

    for (const auto& title : windows) {
        bool matches = false;
        if (m_regex) {
            matches = regex.match(title.c_str()).hasMatch();
        } else {
            /* Direct search */
            QString tmp(title.c_str());
            matches = tmp.contains(m_title);
            if (matches && !m_pause.isEmpty() && !tmp.contains(m_pause))
                matches = false;
        }

        if (matches) {
            result = title.c_str();
            break;
        }
    }
    return result;
}

QString window_source::get_title(const vector<pair<string, string>>& processes)
{
    QString result = "";
    for (const auto& p : processes) {
        if (utf8_to_qt(p.first.c_str()) == m_process_name) {
            result = utf8_to_qt(p.second.c_str());
            break;
        }
    }
    return result;
}

void window_source::refresh()
{
    if (m_title.isEmpty())
        return;
    QString result;

    if (m_use_process_name) {
        vector<pair<string, string>> processes;
        GetWindowAndExeList(processes);
        result = get_title(processes);
    } else {
        vector<string> windows;
        GetWindowList(windows);
        result = get_title(windows);
    }

    begin_refresh();
    m_current.clear();
    if (result.isEmpty()) {
        m_current.set_state(state_stopped);
    } else {
        /* Replace & cut */
        result.replace(m_search, m_replace);
        if (0 < m_cut_end + m_cut_begin && m_cut_end + m_cut_begin < result.length())
            result = result.mid(m_cut_begin, result.length() - m_cut_begin - m_cut_end);

        m_current.set_state(state_playing);
        m_current.set_title(result);
    }
}

bool window_source::execute_capability(capability)
{
    /* NO-OP */
    return true;
}
