/**
 * This file is part of obs-studio
 * which is licensed under the GPL 2.0
 * See COPYING or https://www.gnu.org/licenses/old-licenses/gpl-2.0.txt
 * https://raw.githubusercontent.com/obsproject/obs-studio/master/UI/frontend-plugins/frontend-tools/auto-scene-switcher-osx.mm
 */

#import <AppKit/AppKit.h>
#include <util/platform.h>
#include "window_helper.hpp"

using namespace std;

void GetWindowList(vector<string> &windows)
{
	windows.resize(0);

	@autoreleasepool {
		NSWorkspace *ws = [NSWorkspace sharedWorkspace];
		NSArray *array = [ws runningApplications];
		for (NSRunningApplication *app in array) {
			NSString *name = app.localizedName;
			if (!name)
				continue;

			const char *str = name.UTF8String;
			if (str && *str)
				windows.emplace_back(str);
		}
	}
}

void GetCurrentWindowTitle(string &title)
{
	title.resize(0);

	@autoreleasepool {
		NSWorkspace *ws = [NSWorkspace sharedWorkspace];
		NSRunningApplication *app = [ws frontmostApplication];
		if (app) {
			NSString *name = app.localizedName;
			if (!name)
				return;

			const char *str = name.UTF8String;
			if (str && *str)
				title = str;
		}
	}
}
