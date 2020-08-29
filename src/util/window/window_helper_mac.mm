/**
 * This file is part of obs-studio
 * which is licensed under the GPL 2.0
 * See COPYING or https://www.gnu.org/licenses/old-licenses/gpl-2.0.txt
 * https://raw.githubusercontent.com/obsproject/obs-studio/master/UI/frontend-plugins/frontend-tools/auto-scene-switcher-osx.mm
 * https://github.com/obsproject/obs-studio/blob/master/plugins/mac-capture/window-utils.m
 */

#import <CoreGraphics/CGWindow.h>
#import <Cocoa/Cocoa.h>
#include <util/platform.h>
#include "window_helper.hpp"

#define WINDOW_NAME ((NSString *)kCGWindowName)
#define WINDOW_NUMBER ((NSString *)kCGWindowNumber)
#define OWNER_NAME ((NSString *)kCGWindowOwnerName)
#define OWNER_PID ((NSNumber *)kCGWindowOwnerPID)

using namespace std;

static NSComparator win_info_cmp = ^(NSDictionary *o1, NSDictionary *o2) {
	NSComparisonResult res = [o1[OWNER_NAME] compare:o2[OWNER_NAME]];
	if (res != NSOrderedSame)
		return res;

	res = [o1[OWNER_PID] compare:o2[OWNER_PID]];
	if (res != NSOrderedSame)
		return res;

	res = [o1[WINDOW_NAME] compare:o2[WINDOW_NAME]];
	if (res != NSOrderedSame)
		return res;

	return [o1[WINDOW_NUMBER] compare:o2[WINDOW_NUMBER]];
};

NSArray *enumerate_windows(void)
{
	NSArray *arr = CFBridgingRelease(CGWindowListCopyWindowInfo(kCGWindowListOptionAll, kCGNullWindowID));
	return [arr sortedArrayUsingComparator:win_info_cmp];
}

static inline const char *make_name(NSString *owner, NSString *name)
{
	if (!owner.length)
		return "";

	NSString *str = [NSString stringWithFormat:@"[%@] %@", owner, name];
	return str.UTF8String;
}

void GetWindowList(vector<string> &windows)
{
	@autoreleasepool {
		for (NSDictionary *d in enumerate_windows()) {
			windows.emplace_back(make_name(d[OWNER_NAME], d[WINDOW_NAME]));
		}
	}
}

void GetWindowAndExeList(vector<pair<string, string>>& list)
{
	@autoreleasepool {
		for (NSDictionary *d in enumerate_windows()) {
			list.emplace_back(pair<string, string>(((NSString *)d[OWNER_NAME]).UTF8String, ((NSString *)d[WINDOW_NAME]).UTF8String));
		}
	}
}
