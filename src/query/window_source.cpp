/**
 * This file is part of tuna
 * which is licensed under the GPL v2.0
 * See LICENSE or http://www.gnu.org/licenses
 * github.com/univrsal/tuna
 */
#include "window_source.hpp"
#include "../util/config.hpp"
#include "../util/utility.hpp"
#include "../util/window/window_helper.hpp"
#include "../gui/tuna_gui.hpp"
#include <QRegularExpression>

window_source::window_source()
{
    m_capabilities = CAP_PLAIN;
}

void window_source::load()
{
    CDEF_STR(CFG_WINDOW_TITLE, "");
    CDEF_BOOL(CFG_WINDOW_REGEX, false);
    CDEF_STR(CFG_WINDOW_SEARCH, "");
    CDEF_STR(CFG_WINDOW_REPLACE, "");
    CDEF_UINT(CFG_WINDOW_CUT_BEGIN, 0);
    CDEF_UINT(CFG_WINDOW_CUT_END, 0);

    m_title = CGET_STR(CFG_WINDOW_TITLE);
    m_regex = CGET_BOOL(CFG_WINDOW_REGEX);
    m_search = CGET_STR(CFG_WINDOW_SEARCH);
    m_replace = CGET_STR(CFG_WINDOW_REPLACE);
    m_cut_begin = CGET_UINT(CFG_WINDOW_CUT_BEGIN);
    m_cut_end = CGET_UINT(CFG_WINDOW_CUT_END);
}

void window_source::save()
{
    CSET_STR(CFG_WINDOW_TITLE, m_title.c_str());
    CSET_STR(CFG_WINDOW_SEARCH, m_search.c_str());
    CSET_STR(CFG_WINDOW_REPLACE, m_replace.c_str());
    CSET_BOOL(CFG_WINDOW_REGEX, m_regex);
    CSET_UINT(CFG_WINDOW_CUT_BEGIN, m_cut_begin);
    CSET_UINT(CFG_WINDOW_CUT_END, m_cut_begin);
}

void window_source::refresh()
{
    std::vector<std::string> window_titles;
    GetWindowList(window_titles);
    QRegularExpression regex(m_title.c_str());
    std::string result = "";

    for (const auto& title : window_titles) {
        bool matches = false;
        if (m_regex) {
            matches = regex.match(title.c_str()).hasMatch();
        } else {
            /* Direct search */
            matches = title.find(m_title) != std::string::npos;
        }

        if (matches) {
            result = title;
            break;
        }
    }

    /* Replace & cut */
    util::replace_all(result, m_search, m_replace);
    if (m_cut_end + m_cut_begin < result.length())
        result = result.substr(m_cut_begin, m_cut_end);

    m_current = {};
    m_current.data = CAP_PLAIN;
    m_current.title = result;
}

bool window_source::execute_capability(capability c)
{
    /* NO-OP */
    return true;
}

void window_source::load_gui_values()
{
    tuna_dialog->set_window_regex(m_regex);
    tuna_dialog->set_window_title(m_title.c_str());
    tuna_dialog->set_window_search(m_search.c_str());
    tuna_dialog->set_window_replace(m_replace.c_str());
    tuna_dialog->set_window_cut_begin(m_cut_begin);
    tuna_dialog->set_window_cut_end(m_cut_end);
}
