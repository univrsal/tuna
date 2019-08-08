/**
 * This file is part of tuna
 * which is licensed under the GPL v2.0
 * See LICENSE or http://www.gnu.org/licenses
 * github.com/univrsal/tuna
 */

#include <obs-module.h>
#include "util/constants.hpp"

OBS_DECLARE_MODULE()
OBS_MODULE_USE_DEFAULT_LOCALE(S_PLUGIN_ID, "en-US")

bool obs_module_load()
{
    return true;
}

void obs_module_unload()
{

}
