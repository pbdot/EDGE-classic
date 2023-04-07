#include "i_defs.h"
#include "lua_vm.h"

lua_module_c::lua_module_c(std::string name, lua_vm_c *vm)
{
	name_ = name;
	vm_ = vm;
}

std::unordered_map<lua_vm_type_e, lua_vm_c *> lua_vm_c::vms_;

lua_vm_c::lua_vm_c(lua_vm_type_e type)
{
	SYS_ASSERT(vms_.find(type) == vms_.end());

	vms_[type] = this;

	Open();

	//@todo: better registration
	LUA_Sys_Init(this);

	switch (type) {
	case LUA_VM_UI:
		LUA_Hud_Init(this);
		break;
	}
}

// Lua print replacement
static int LUA_Print(lua_State *L);

void lua_vm_c::Open()
{
	state_.open_libraries(sol::lib::base, sol::lib::table, sol::lib::package,
						  sol::lib::os, sol::lib::io, sol::lib::math,
						  sol::lib::string, sol::lib::coroutine,
						  sol::lib::debug);

	// main edge module
	state_["edge"] = state_.create_table();
	// override the default lua print
	state_["print"] = LUA_Print;
}

void LUA_Init() 
{ 
	new lua_vm_c(LUA_VM_UI); 

	LUA_CheckError(lua_vm_c::GetVM(LUA_VM_UI)->GetState().do_file("test.lua"));
	
}

// Lua print replacement
int LUA_Print(lua_State *L)
{
	int n = lua_gettop(L);
	int i;
	for (i = 1; i <= n; i++) {
		size_t l;
		const char *s = luaL_tolstring(L, i, &l);
		if (i > 1)
			I_Printf("   ");
		I_Printf(s);
		lua_pop(L, 1);
	}
	I_Printf("\n");
	return 0;
}
