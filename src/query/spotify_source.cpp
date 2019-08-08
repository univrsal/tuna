/**
 * This file is part of tuna
 * which is licensed under the GPL v2.0
 * See LICENSE or http://www.gnu.org/licenses
 * github.com/univrsal/tuna
 */
#include <util/config-file.h>
#include "spotify_source.hpp"
#include "../util/constants.hpp"
#include "../util/config.hpp"

spotify_source::spotify_source()
    : music_source(T_SPOTIFY_SOURCE)
{
    /* NO-OP */
}

void spotify_source::load()
{
    config_set_default_bool(config::instance, CFG_REGION_SPOTIFY,
                            CFG_SPOTIFY_LOGGED_IN, false);
    config_set_default_string(config::instance, CFG_REGION_SPOTIFY,
                              CFG_SPOTIFY_TOKEN, "");

    m_logged_in = config_get_bool(config::instance, CFG_REGION_SPOTIFY,
                                  CFG_SPOTIFY_LOGGED_IN);
    m_token = config_get_string(config::instance, CFG_REGION_SPOTIFY,
                                CFG_SPOTIFY_TOKEN);

}
