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

#ifdef HAVE_SECOND_MUSIC_SYSTEM

#include "lua_sms.h"
#include "SecondMusicSystem.h" // the aleph one header

#include "second-music-system.h" // the library header

char Lua_SMS_Name[] = "SMS";

const char* FADE_TYPE_LIST[] = {
    // Indexes within this array must correspond to `SMS_FADE_TYPE_*` constants
    // in `second-music-system.h`
    "exponential",
    "logarithmic",
    "linear",
    nullptr
};

int Lua_SMS_Optional_Fade_Type(lua_State* L, int index) {
    if((index > 0 && lua_gettop(L) < index) || lua_isnil(L, index)) {
        return SMS_FADE_TYPE_DEFAULT;
    } else if(lua_isstring(L, index)) {
        return luaL_checkoption(L, index, nullptr, FADE_TYPE_LIST);
    } else {
		return luaL_argerror(L, index, "must be nil, \"exponential\", \"logarithmic\", or \"linear\"");
    }
}

int Lua_SMS_Replace_Soundtrack(lua_State* L) {
    size_t source_len = 0;
    const char* source_code = luaL_checklstring(L, 1, &source_len);
    char* error_out = nullptr;
    size_t error_len = 0;
    SMS_Soundtrack* soundtrack = SMS_Soundtrack_parse_new(source_code, source_len, &error_out, &error_len);
    if(soundtrack == nullptr) {
        assert(error_out != nullptr);
        // We can't use luaL_error immediately, because we need to free the
        // error message...
        luaL_where(L, 1);
        lua_pushlstring(L, error_out, error_len);
        free(error_out);
        lua_concat(L, 2);
        return lua_error(L);
    }
    assert(soundtrack != nullptr);
    assert(error_out == nullptr);
    SMS::SetMusicSearchPath(L_Get_Search_Path(L));
    auto commander = SMS::GetCommander();
    assert(commander);
    SMS_Commander_replace_soundtrack(commander, soundtrack);
    return 0;
}

int Lua_SMS_Precache(lua_State* L) {
    size_t flow_name_len;
    const char* flow_name = luaL_checklstring(L, 1, &flow_name_len);
    auto commander = SMS::GetCommander();
    assert(commander);
    SMS_Commander_precache(commander, flow_name, flow_name_len);
    return 0;
}

int Lua_SMS_Unprecache(lua_State* L) {
    size_t flow_name_len;
    const char* flow_name = luaL_checklstring(L, 1, &flow_name_len);
    auto commander = SMS::GetCommander();
    assert(commander);
    SMS_Commander_unprecache(commander, flow_name, flow_name_len);
    return 0;
}

int Lua_SMS_Unprecache_All(lua_State* L) {
    auto commander = SMS::GetCommander();
    assert(commander);
    SMS_Commander_unprecache_all(commander);
    return 0;
}

int Lua_SMS_Set_Flow_Control(lua_State* L) {
    size_t control_name_len;
    const char* control_name = luaL_checklstring(L, 1, &control_name_len);
    auto commander = SMS::GetCommander();
    assert(commander);
    if(lua_gettop(L) < 2 || lua_isnil(L, 2)) {
        SMS_Commander_clear_flow_control(commander, control_name, control_name_len);
    } else if(lua_isnumber(L, 2)) {
        SMS_Commander_set_flow_control_to_number(commander, control_name, control_name_len, lua_tonumber(L, 2));
    } else if(lua_isstring(L, 2)) {
        size_t value_len;
        const char* value = lua_tolstring(L, 2, &value_len);
        SMS_Commander_set_flow_control_to_string(commander, control_name, control_name_len, value, value_len);
    } else {
        return luaL_argerror(L, 2, "must be a number, a string, or nil");
    }
    return 0;
}

int Lua_SMS_Clear_Flow_Control(lua_State* L) {
    size_t control_name_len;
    const char* control_name = luaL_checklstring(L, 1, &control_name_len);
    auto commander = SMS::GetCommander();
    assert(commander);
    SMS_Commander_clear_flow_control(commander, control_name, control_name_len);
    return 0;
}

int Lua_SMS_Clear_Prefixed_Flow_Controls(lua_State* L) {
    size_t control_prefix_len;
    const char* control_prefix = luaL_checklstring(L, 1, &control_prefix_len);
    auto commander = SMS::GetCommander();
    assert(commander);
    SMS_Commander_clear_prefixed_flow_controls(commander, control_prefix, control_prefix_len);
    return 0;
}

int Lua_SMS_Clear_All_Flow_Controls(lua_State* L) {
    auto commander = SMS::GetCommander();
    assert(commander);
    SMS_Commander_clear_all_flow_controls(commander);
    return 0;
}

int Lua_SMS_Fade_Mix_Control_To(lua_State* L) {
    size_t control_name_len;
    const char* control_name = luaL_checklstring(L, 1, &control_name_len);
    auto target_volume = luaL_optnumber(L, 2, 1.0);
    auto fade_length = luaL_optnumber(L, 3, 0.0);
    int fade_type = Lua_SMS_Optional_Fade_Type(L, 4);
    auto commander = SMS::GetCommander();
    assert(commander);
    SMS_Commander_fade_mix_control_to(commander, control_name, control_name_len, target_volume, fade_length, fade_type);
    return 0;
}

int Lua_SMS_Fade_Prefixed_Mix_Controls_To(lua_State* L) {
    size_t control_prefix_len;
    const char* control_prefix = luaL_checklstring(L, 1, &control_prefix_len);
    auto target_volume = luaL_optnumber(L, 2, 1.0);
    auto fade_length = luaL_optnumber(L, 3, 0.0);
    int fade_type = Lua_SMS_Optional_Fade_Type(L, 4);
    auto commander = SMS::GetCommander();
    assert(commander);
    SMS_Commander_fade_prefixed_mix_controls_to(commander, control_prefix, control_prefix_len, target_volume, fade_length, fade_type);
    return 0;
}

int Lua_SMS_Fade_All_Mix_Controls_To(lua_State* L) {
    auto target_volume = luaL_optnumber(L, 1, 1.0);
    auto fade_length = luaL_optnumber(L, 2, 0.0);
    int fade_type = Lua_SMS_Optional_Fade_Type(L, 3);
    auto commander = SMS::GetCommander();
    assert(commander);
    SMS_Commander_fade_all_mix_controls_to(commander, target_volume, fade_length, fade_type);
    return 0;
}

int Lua_SMS_Fade_All_Mix_Controls_Except_Main_To(lua_State* L) {
    auto target_volume = luaL_optnumber(L, 1, 1.0);
    auto fade_length = luaL_optnumber(L, 2, 0.0);
    int fade_type = Lua_SMS_Optional_Fade_Type(L, 3);
    auto commander = SMS::GetCommander();
    assert(commander);
    SMS_Commander_fade_all_mix_controls_except_main_to(commander, target_volume, fade_length, fade_type);
    return 0;
}

int Lua_SMS_Fade_Mix_Control_Out(lua_State* L) {
    size_t control_name_len;
    const char* control_name = luaL_checklstring(L, 1, &control_name_len);
    auto fade_length = luaL_optnumber(L, 2, 0.0);
    int fade_type = Lua_SMS_Optional_Fade_Type(L, 3);
    auto commander = SMS::GetCommander();
    assert(commander);
    SMS_Commander_fade_mix_control_out(commander, control_name, control_name_len, fade_length, fade_type);
    return 0;
}

int Lua_SMS_Fade_Prefixed_Mix_Controls_Out(lua_State* L) {
    size_t control_prefix_len;
    const char* control_prefix = luaL_checklstring(L, 1, &control_prefix_len);
    auto fade_length = luaL_optnumber(L, 2, 0.0);
    int fade_type = Lua_SMS_Optional_Fade_Type(L, 3);
    auto commander = SMS::GetCommander();
    assert(commander);
    SMS_Commander_fade_prefixed_mix_controls_out(commander, control_prefix, control_prefix_len, fade_length, fade_type);
    return 0;
}

int Lua_SMS_Fade_All_Mix_Controls_Out(lua_State* L) {
    auto fade_length = luaL_optnumber(L, 1, 0.0);
    int fade_type = Lua_SMS_Optional_Fade_Type(L, 2);
    auto commander = SMS::GetCommander();
    assert(commander);
    SMS_Commander_fade_all_mix_controls_out(commander, fade_length, fade_type);
    return 0;
}

int Lua_SMS_Fade_All_Mix_Controls_Except_Main_Out(lua_State* L) {
    auto fade_length = luaL_optnumber(L, 1, 0.0);
    int fade_type = Lua_SMS_Optional_Fade_Type(L, 2);
    auto commander = SMS::GetCommander();
    assert(commander);
    SMS_Commander_fade_all_mix_controls_except_main_out(commander, fade_length, fade_type);
    return 0;
}

int Lua_SMS_Kill_Mix_Control(lua_State* L) {
    size_t control_name_len;
    const char* control_name = luaL_checklstring(L, 1, &control_name_len);
    auto commander = SMS::GetCommander();
    assert(commander);
    SMS_Commander_kill_mix_control(commander, control_name, control_name_len);
    return 0;
}

int Lua_SMS_Kill_Prefixed_Mix_Controls(lua_State* L) {
    size_t control_prefix_len;
    const char* control_prefix = luaL_checklstring(L, 1, &control_prefix_len);
    auto commander = SMS::GetCommander();
    assert(commander);
    SMS_Commander_kill_prefixed_mix_controls(commander, control_prefix, control_prefix_len);
    return 0;
}

int Lua_SMS_Kill_All_Mix_Controls(lua_State* L) {
    auto commander = SMS::GetCommander();
    assert(commander);
    SMS_Commander_kill_all_mix_controls(commander);
    return 0;
}

int Lua_SMS_Kill_All_Mix_Controls_Except_Main(lua_State* L) {
    auto commander = SMS::GetCommander();
    assert(commander);
    SMS_Commander_kill_all_mix_controls_except_main(commander);
    return 0;
}

int Lua_SMS_Start_Flow(lua_State* L) {
    size_t flow_name_len;
    const char* flow_name = luaL_checklstring(L, 1, &flow_name_len);
    auto target_volume = luaL_optnumber(L, 2, 1.0);
    auto fade_length = luaL_optnumber(L, 3, 0.0);
    int fade_type = Lua_SMS_Optional_Fade_Type(L, 4);
    auto commander = SMS::GetCommander();
    assert(commander);
    SMS_Commander_start_flow(commander, flow_name, flow_name_len, target_volume, fade_length, fade_type);
    return 0;
}

int Lua_SMS_Fade_Flow_To(lua_State* L) {
    size_t flow_name_len;
    const char* flow_name = luaL_checklstring(L, 1, &flow_name_len);
    auto target_volume = luaL_optnumber(L, 2, 1.0);
    auto fade_length = luaL_optnumber(L, 3, 0.0);
    int fade_type = Lua_SMS_Optional_Fade_Type(L, 4);
    auto commander = SMS::GetCommander();
    assert(commander);
    SMS_Commander_fade_flow_to(commander, flow_name, flow_name_len, target_volume, fade_length, fade_type);
    return 0;
}

int Lua_SMS_Fade_Prefixed_Flows_To(lua_State* L) {
    size_t flow_prefix_len;
    const char* flow_prefix = luaL_checklstring(L, 1, &flow_prefix_len);
    auto target_volume = luaL_optnumber(L, 2, 1.0);
    auto fade_length = luaL_optnumber(L, 3, 0.0);
    int fade_type = Lua_SMS_Optional_Fade_Type(L, 4);
    auto commander = SMS::GetCommander();
    assert(commander);
    SMS_Commander_fade_prefixed_flows_to(commander, flow_prefix, flow_prefix_len, target_volume, fade_length, fade_type);
    return 0;
}

int Lua_SMS_Fade_All_Flows_To(lua_State* L) {
    auto target_volume = luaL_optnumber(L, 1, 1.0);
    auto fade_length = luaL_optnumber(L, 2, 0.0);
    int fade_type = Lua_SMS_Optional_Fade_Type(L, 3);
    auto commander = SMS::GetCommander();
    assert(commander);
    SMS_Commander_fade_all_flows_to(commander, target_volume, fade_length, fade_type);
    return 0;
}

int Lua_SMS_Fade_Flow_Out(lua_State* L) {
    size_t flow_name_len;
    const char* flow_name = luaL_checklstring(L, 1, &flow_name_len);
    auto fade_length = luaL_optnumber(L, 2, 0.0);
    int fade_type = Lua_SMS_Optional_Fade_Type(L, 3);
    auto commander = SMS::GetCommander();
    assert(commander);
    SMS_Commander_fade_flow_out(commander, flow_name, flow_name_len, fade_length, fade_type);
    return 0;
}

int Lua_SMS_Fade_Prefixed_Flows_Out(lua_State* L) {
    size_t flow_prefix_len;
    const char* flow_prefix = luaL_checklstring(L, 1, &flow_prefix_len);
    auto fade_length = luaL_optnumber(L, 2, 0.0);
    int fade_type = Lua_SMS_Optional_Fade_Type(L, 3);
    auto commander = SMS::GetCommander();
    assert(commander);
    SMS_Commander_fade_prefixed_flows_out(commander, flow_prefix, flow_prefix_len, fade_length, fade_type);
    return 0;
}

int Lua_SMS_Fade_All_Flows_Out(lua_State* L) {
    auto fade_length = luaL_optnumber(L, 1, 0.0);
    int fade_type = Lua_SMS_Optional_Fade_Type(L, 2);
    auto commander = SMS::GetCommander();
    assert(commander);
    return 0;
}

int Lua_SMS_Kill_Flow(lua_State* L) {
    size_t flow_name_len;
    const char* flow_name = luaL_checklstring(L, 1, &flow_name_len);
    auto commander = SMS::GetCommander();
    assert(commander);
    SMS_Commander_kill_flow(commander, flow_name, flow_name_len);
    return 0;
}

int Lua_SMS_Kill_Prefixed_Flows(lua_State* L) {
    size_t flow_prefix_len;
    const char* flow_prefix = luaL_checklstring(L, 1, &flow_prefix_len);
    auto commander = SMS::GetCommander();
    assert(commander);
    SMS_Commander_kill_prefixed_flows(commander, flow_prefix, flow_prefix_len);
    return 0;
}

int Lua_SMS_Kill_All_Flows(lua_State* L) {
    auto commander = SMS::GetCommander();
    assert(commander);
    SMS_Commander_kill_all_flows(commander);
    return 0;
}

int Lua_SMS_Get_Fade_On_Leave_Map(lua_State* L) {
    lua_pushnumber(L, SMS::GetFadeOnLeaveMap());
    return 1;
}

int Lua_SMS_Get_Start_Flow_On_Leave_Map(lua_State* L) {
    auto value = SMS::GetStartFlowOnLeaveMap();
    if(value.empty()) lua_pushnil(L);
    else lua_pushlstring(L, value.data(), value.size());
    return 1;
}

int Lua_SMS_Get_Flow_Control_On_Leave_Map(lua_State* L) {
    auto value = SMS::GetFlowControlOnLeaveMap();
    if(value.empty()) lua_pushnil(L);
    else lua_pushlstring(L, value.data(), value.size());
    return 1;
}

int Lua_SMS_Set_Fade_On_Leave_Map(lua_State* L) {
    if(lua_gettop(L) < 2 || lua_isnil(L, 2))
        SMS::SetFadeOnLeaveMap(-1.0f);
    else if(lua_isboolean(L, 2))
        SMS::SetFadeOnLeaveMap(lua_toboolean(L, 2) ? 1.0f : -1.0f);
    else
        SMS::SetFadeOnLeaveMap(luaL_checknumber(L, 2));
    return 0;
}

int Lua_SMS_Set_Start_Flow_On_Leave_Map(lua_State* L) {
    size_t len;
    const char* p;
    if(lua_gettop(L) < 2 || lua_isnil(L, 2)) {
        len = 0;
        p = "";
    } else {
        p = luaL_checklstring(L, 2, &len);
    }
    SMS::SetStartFlowOnLeaveMap(std::string(p, len));
    return 0;
}

int Lua_SMS_Set_Flow_Control_On_Leave_Map(lua_State* L) {
    size_t len;
    const char* p;
    if(lua_gettop(L) < 2 || lua_isnil(L, 2)) {
        len = 0;
        p = "";
    } else {
        p = luaL_checklstring(L, 2, &len);
    }
    SMS::SetFlowControlOnLeaveMap(std::string(p, len));
    return 0;
}

int Lua_SMS_Get_Version_String(lua_State* L) {
    lua_pushstring(L, SMS_get_version_string());
    return 1;
}

int Lua_SMS_Get_Version_Number(lua_State* L) {
    auto big_number = SMS_get_version_number();
    lua_pushinteger(L, big_number);
    return 1;
}

int Lua_SMS_Get_Version_Major(lua_State* L) {
    auto big_number = SMS_get_version_number();
    lua_pushinteger(L, big_number >> 16);
    return 1;
}

int Lua_SMS_Get_Version_Minor(lua_State* L) {
    auto big_number = SMS_get_version_number();
    lua_pushinteger(L, (big_number >> 8) & 255);
    return 1;
}

int Lua_SMS_Get_Version_Patch(lua_State* L) {
    auto big_number = SMS_get_version_number();
    lua_pushinteger(L, big_number & 255);
    return 1;
}

const luaL_Reg Lua_SMS_Get[] = {
    // functions
	{"replace_soundtrack", L_TableFunction<Lua_SMS_Replace_Soundtrack>},
    {"precache", L_TableFunction<Lua_SMS_Precache>},
    {"unprecache", L_TableFunction<Lua_SMS_Unprecache>},
    {"unprecache_all", L_TableFunction<Lua_SMS_Unprecache_All>},
    {"set_flow_control", L_TableFunction<Lua_SMS_Set_Flow_Control>},
    {"clear_flow_control", L_TableFunction<Lua_SMS_Clear_Flow_Control>},
    {"clear_prefixed_flow_controls", L_TableFunction<Lua_SMS_Clear_Prefixed_Flow_Controls>},
    {"clear_all_flow_controls", L_TableFunction<Lua_SMS_Clear_All_Flow_Controls>},
    {"fade_mix_control_to", L_TableFunction<Lua_SMS_Fade_Mix_Control_To>},
    {"fade_prefixed_mix_controls_to", L_TableFunction<Lua_SMS_Fade_Prefixed_Mix_Controls_To>},
    {"fade_all_mix_controls_to", L_TableFunction<Lua_SMS_Fade_All_Mix_Controls_To>},
    {"fade_all_mix_controls_except_main_to", L_TableFunction<Lua_SMS_Fade_All_Mix_Controls_Except_Main_To>},
    {"fade_mix_control_out", L_TableFunction<Lua_SMS_Fade_Mix_Control_Out>},
    {"fade_prefixed_mix_controls_out", L_TableFunction<Lua_SMS_Fade_Prefixed_Mix_Controls_Out>},
    {"fade_all_mix_controls_out", L_TableFunction<Lua_SMS_Fade_All_Mix_Controls_Out>},
    {"fade_all_mix_controls_except_main_out", L_TableFunction<Lua_SMS_Fade_All_Mix_Controls_Except_Main_Out>},
    {"kill_mix_control", L_TableFunction<Lua_SMS_Kill_Mix_Control>},
    {"kill_prefixed_mix_controls", L_TableFunction<Lua_SMS_Kill_Prefixed_Mix_Controls>},
    {"kill_all_mix_controls", L_TableFunction<Lua_SMS_Kill_All_Mix_Controls>},
    {"kill_all_mix_controls_except_main", L_TableFunction<Lua_SMS_Kill_All_Mix_Controls_Except_Main>},
    {"start_flow", L_TableFunction<Lua_SMS_Start_Flow>},
    {"fade_flow_to", L_TableFunction<Lua_SMS_Fade_Flow_To>},
    {"fade_prefixed_flows_to", L_TableFunction<Lua_SMS_Fade_Prefixed_Flows_To>},
    {"fade_all_flows_to", L_TableFunction<Lua_SMS_Fade_All_Flows_To>},
    {"fade_flow_out", L_TableFunction<Lua_SMS_Fade_Flow_Out>},
    {"fade_prefixed_flows_out", L_TableFunction<Lua_SMS_Fade_Prefixed_Flows_Out>},
    {"fade_all_flows_out", L_TableFunction<Lua_SMS_Fade_All_Flows_Out>},
    {"kill_flow", L_TableFunction<Lua_SMS_Kill_Flow>},
    {"kill_prefixed_flows", L_TableFunction<Lua_SMS_Kill_Prefixed_Flows>},
    {"kill_all_flows", L_TableFunction<Lua_SMS_Kill_All_Flows>},
    // constants
    {"version_string", Lua_SMS_Get_Version_String},
    {"version_number", Lua_SMS_Get_Version_Number},
    {"version_major", Lua_SMS_Get_Version_Major},
    {"version_minor", Lua_SMS_Get_Version_Minor},
    {"version_patch", Lua_SMS_Get_Version_Patch},
    // fields
    {"fade_on_leave_map", Lua_SMS_Get_Fade_On_Leave_Map},
    {"start_flow_on_leave_map", Lua_SMS_Get_Start_Flow_On_Leave_Map},
    {"set_flow_control_on_leave_map", Lua_SMS_Get_Flow_Control_On_Leave_Map},
    {0, 0}
};

const luaL_Reg Lua_SMS_Set[] = {
    {"fade_on_leave_map", Lua_SMS_Set_Fade_On_Leave_Map},
    {"start_flow_on_leave_map", Lua_SMS_Set_Start_Flow_On_Leave_Map},
    {"set_flow_control_on_leave_map", Lua_SMS_Set_Flow_Control_On_Leave_Map},
    {0, 0}
};

#include "Logging.h"

int Lua_SMS_register(lua_State* L) {
    Lua_SMS::Register(L, Lua_SMS_Get, Lua_SMS_Set);
	Lua_SMS::Push(L, 0);
	lua_setglobal(L, Lua_SMS_Name);
	return 0;
}

#endif
