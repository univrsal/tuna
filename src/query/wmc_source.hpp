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
#include "music_source.hpp"
#include <string>
#include <vector>

#pragma comment(lib, "windowsapp")

#include <algorithm>
#include <map>
#include <mutex>
#include <winrt/Windows.Foundation.Collections.h>
#include <winrt/Windows.Graphics.Imaging.h>
#include <winrt/Windows.Media.Control.h>
#include <winrt/Windows.Storage.Streams.h>
#include <winrt/base.h>

using namespace winrt;
using namespace Windows::Graphics::Imaging;
using namespace Windows::Media::Control;
using namespace Windows::Storage::Streams;
using namespace Windows::Foundation::Collections;

class wmc_source : public music_source {
    friend class wmc;
    GlobalSystemMediaTransportControlsSessionManager m_session_manager { nullptr };
    std::string m_selected_player {};
    std::vector<std::string> m_registered_players {};
    std::mutex m_internal_mutex;
    std::map<std::string, song> m_info;

public:
    wmc_source();
    ~wmc_source() = default;

    void refresh() override;
    void handle_media_property_change(GlobalSystemMediaTransportControlsSession session, MediaPropertiesChangedEventArgs const& arg);
    void handle_media_playback_info_change(GlobalSystemMediaTransportControlsSession session, PlaybackInfoChangedEventArgs const& args);
    void update_players();
    bool execute_capability(capability c) override;
    bool enabled() const { return true; }
    void request_manager();
    void handle_cover() override
    { /* NO-OP */
    }

    void load() override;

    //bool enabled() const override;
    //void handle_cover() override;
    //void reset_info() override;
};