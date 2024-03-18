/*

	Copyright (C) 2023 Solra Bizna
	and the "Aleph One" developers.

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 3 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	This license is contained in the file "COPYING",
	which is included with this source code; it is available online at
	http://www.gnu.org/licenses/gpl.html

    -

	Lua bindings to the Second Music System.

*/

#ifndef __LUA_SMS_H
#define __LUA_SMS_H

#include "cseries.h"

extern "C"
{
#include "lua.h"
#include "lauxlib.h"
#include "lualib.h"
}

#include "lua_templates.h"

extern char Lua_SMS_Name[]; // SMS
typedef L_Class<Lua_SMS_Name> Lua_SMS;

int Lua_SMS_register(lua_State* L);

#endif
