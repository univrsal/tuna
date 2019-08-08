/**
 * This file is part of tuna
 * which is licensed under the GPL v2.0
 * See LICENSE or http://www.gnu.org/licenses
 * github.com/univrsal/tuna
 */
#include "config.hpp"
#include "../query/spotify_source.hpp"

#include <util/config-file.h>
#include <obs-module.h>
#include <obs-frontend-api.h>

namespace config
{
    config_t* instance = nullptr;

    spotify_source* spotify = nullptr;
    void init_default()
    {
        const char* path_song_file = obs_module_file("song.txt");
        const char* path_cover_art = obs_get_module_data_path(obs_current_module());

        config_set_default_string(instance, CFG_REGION, CFG_SONG_PATH,
                                  path_song_file);
        config_set_default_string(instance, CFG_REGION, CFG_COVER_PATH,
                                  path_cover_art);

        /* Source states */
        config_set_default_bool(instance, CFG_REGION,
                                CFG_SPOTIFY_ENABLED, true);
        config_set_default_bool(instance, CFG_REGION,
                                CFG_MPD_ENABLED, true);
        config_set_default_bool(instance, CFG_REGION,
                                CFG_WINDOW_TITLE_ENABLED, true);
    }

    void load()
    {
        instance = obs_frontend_get_global_config();
        init_default();

        spotify = new spotify_source;

        spotify->load();
    }
}
