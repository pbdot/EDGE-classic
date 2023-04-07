#include "i_defs.h"
#include "lua_vm.h"

lua_module_c::lua_module_c(std::string name, lua_vm_c *vm)
{
	name_ = name;
	vm_ = vm;
}

void lua_module_c::Init(const std::string &filename)
{
	sol::state &state = vm_->GetState();

	sol::protected_function_result result = state.safe_script_file(filename);

	if (result.valid()) {
		sol::type t = result.get_type();
		SYS_ASSERT(t == sol::type::table);

		sol::table m = result.get<sol::table>();

		for (const auto &key_value_pair : m) {
			sol::object name = key_value_pair.first;
			sol::object function = key_value_pair.second;
			SYS_ASSERT(name.get_type() == sol::type::string);
			SYS_ASSERT(function.get_type() == sol::type::function);
			callbacks_[name.as<std::string>()] = function;
		}
	}
	else {
		sol::error error = result;
		I_Error("Lua: Error %s\n", error.what());
	}
}

std::unordered_map<lua_vm_type_e, lua_vm_c *> lua_vm_c::vms_;

lua_vm_c::lua_vm_c(lua_vm_type_e type)
{
	SYS_ASSERT(vms_.find(type) == vms_.end());

	vms_[type] = this;

	Open();
}

void lua_vm_c::Call(const std::string& modulename, const std::string& functionname)
{
	modules_[modulename]->callbacks_[functionname].as<sol::protected_function>().call();
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
	state_.create_named_table("edge");
	// override the default lua print
	state_["print"] = LUA_Print;
}

void lua_vm_c::DoFile(const std::string &filename)
{
	LUA_CheckError(state_.do_file(filename), true);
}

static void LUA_VM_Create(lua_vm_type_e type)
{
	lua_vm_c *vm = new lua_vm_c(type);

	//@todo: better module registration

	switch (type) {
	case LUA_VM_UI:
		LUA_Sys_Init(vm);
		LUA_Hud_Init(vm);
		break;
	}
}

void LUA_Init() { LUA_VM_Create(LUA_VM_UI); }

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
