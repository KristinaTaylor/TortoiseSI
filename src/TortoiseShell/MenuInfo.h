// TortoiseSI - a Windows shell extension for easy version control

// Copyright (C) 2015 - TortoiseSI
// Copyright (C) ??? - TortoiseGit

// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software Foundation,
// 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
//

#pragma once
#include "FileStatus.h"

enum MenuItem {
	ViewSandbox,
	CreateSandbox,

	IgnoreSubMenu,
	UnIgnoreSubMenu,

	Test,

	Seperator,
	ResyncFile,
	ResyncEntireSandbox,
	DropSandbox,
	RetargetSandbox
};

struct MenuInfo
{
	MenuItem	menuID;			
	UINT				iconID;			///< the icon to show for the menu entry
	UINT				menuTextID;		///< the text of the menu entry
	UINT				menuDescID;		///< the description text for the menu entry
	std::function<void(const std::vector<std::wstring>&, HWND)>	siCommand;
	std::function<bool(const std::vector<std::wstring>&, FileStatusFlags)>			enable;
};
