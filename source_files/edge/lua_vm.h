#pragma once

#include <sol.hpp>

typedef enum { LUA_VM_UI = 0, LUA_VM_PLAY = 1, LUA_VM_DATA = 2 } lua_vm_type_e;

class lua_vm_c;

class lua_module_c {

	friend class lua_vm_c;

  protected:
	lua_module_c(std::string name, lua_vm_c *vm);

	void Init(const std::string &filename);

	std::string name_;
	lua_vm_c *vm_ = nullptr;

  private:
	const std::string &GetName() { return name_; }
	virtual void Open() = 0;

	std::unordered_map<std::string, sol::object> callbacks_;
};

class lua_vm_c {
  public:
	lua_vm_c(lua_vm_type_e type);

	void DoFile(const std::string &filename);

	void Call(const std::string& modulename, const std::string& functionname);

	template <class ModuleClass> void addModule()
	{
		auto module = new ModuleClass(this);
		module->Open();
		modules_[module->GetName()] = module;
	}

	sol::state &GetState() { return state_; }

	static lua_vm_c *GetVM(lua_vm_type_e type) { return vms_[type]; }

  private:
	void Open();

	std::unordered_map<std::string, lua_module_c *> modules_;

	static std::unordered_map<lua_vm_type_e, lua_vm_c *> vms_;

	lua_vm_type_e type_;

	sol::state state_;
};

void LUA_CheckError(const sol::protected_function_result &result,
					bool fatal = false);

// sys module
void LUA_Sys_Init(lua_vm_c *vm);

// hud module
void LUA_Hud_Init(lua_vm_c *vm);