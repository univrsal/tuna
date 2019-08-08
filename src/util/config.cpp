/**
 * This file is part of tuna
 * which is licensed under the GPL v2.0
 * See LICENSE or http://www.gnu.org/licenses
 * github.com/univrsal/tuna
 */
#include "config.hpp"
#include <util/config-file.h>
#include <obs-module.h>

namespace config
{
    config_t* instance = nullptr;

    void init_default()
    {
        const char* path_song_file = obs_module_file("song.txt");
        const char* path_cover_art = obs_get_module_data_path(obs_current_module());

        config_set_default_string(instance, CFG_REGION_BASICS, CFG_SONG_PATH,
                                  path_song_file);
        config_set_default_string(instance, CFG_REGION_BASICS, CFG_COVER_PATH,
                                  path_cover_art);

        /* Source states */
        config_set_default_bool(instance, CFG_REGION_SOURCES,
                                CFG_SPOTIFY_ENABLED, true);
        config_set_default_bool(instance, CFG_REGION_SOURCES,
                                CFG_MPD_ENABLED, true);
        config_set_default_bool(instance, CFG_REGION_SOURCES,
                                CFG_WINDOW_TITLE_ENABLED, true);
    }

    bool load(const char* path)
    {
        int result = config_open(&instance, path, CONFIG_OPEN_ALWAYS);

        if (result == CONFIG_SUCCESS) {
            return true;
        } else if (result == CONFIG_FILENOTFOUND || result == CONFIG_ERROR) {
            blog(LOG_ERROR, "[tuna] couldn't load config file!");
            return false;
        }
        return result == CONFIG_SUCCESS;
    }

    bool save() {
        int result = config_save(instance);
        if (result == CONFIG_SUCCESS) {
            return true;
        } else if (result == CONFIG_FILENOTFOUND || result == CONFIG_ERROR) {
            blog(LOG_ERROR, "[tuna] couldn't load config file!");
            return false;
        }
        return result == CONFIG_SUCCESS;
    }

    void close() {
        save();
        config_close(instance);
    }
}
