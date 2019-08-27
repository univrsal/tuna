/**
 * This file is part of tuna
 * which is licensed under the GPL v2.0
 * See LICENSE or http://www.gnu.org/licenses
 * github.com/univrsal/tuna
 */
#pragma once

#include "music_source.hpp"

class window_source : public music_source
{
	std::string m_title = "";
	std::string m_search = "", m_replace = "";
	uint16_t m_cut_begin = 0, m_cut_end;
	bool m_regex = false;
public:
	window_source();

	void load() override;

	void save() override;

	void refresh() override;

	bool execute_capability(capability c) override;

	void load_gui_values() override;
};
