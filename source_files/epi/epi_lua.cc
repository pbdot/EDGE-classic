
// clang-format off
#include <lua.hpp>
#include <LuaBridge/LuaBridge.h>
// clang-format on

#include "epi_lua.h"

#include "epi_str_util.h"
#include "epi_windows.h"

namespace epi
{

static int  luaopen_debugger(lua_State *lua);
static int  dbg_pcall(lua_State *lua, int nargs, int nresults, int msgh);
static void dbg_setup_default(lua_State *lua);
static void LuaRegisterCoreLibraries(lua_State *L);

static void *LUA_DefaultAllocator(void *user, void *ptr, size_t osize, size_t nsize)
{
    EPI_UNUSED(user);
    EPI_UNUSED(osize);
    if (nsize == 0)
    {
        if (ptr)
        {
            Mem_Free(ptr);
        }
        return NULL;
    }
    else
        return Mem_Realloc(ptr, nsize);
}

class LuaState : public ILuaState
{
  public:
    LuaState(const LuaConfig &config) : config_(config)
    {
        state_ = lua_newstate(LUA_DefaultAllocator, nullptr);

        state_lookup_[state_]             = this;
        states_[config_.name_.GetIndex()] = this;

        const luaL_Reg loadedlibs[] = {
            {LUA_GNAME, luaopen_base},          {LUA_LOADLIBNAME, luaopen_package}, {LUA_OSLIBNAME, luaopen_os},
            {LUA_COLIBNAME, luaopen_coroutine}, {LUA_TABLIBNAME, luaopen_table},    {LUA_STRLIBNAME, luaopen_string},
            {LUA_MATHLIBNAME, luaopen_math},    {LUA_UTF8LIBNAME, luaopen_utf8},    {nullptr, nullptr}};

        const luaL_Reg *lib;
        /* "require" functions from 'loadedlibs' and set results to global table */
        for (lib = loadedlibs; lib->func; lib++)
        {
            luaL_requiref(state_, lib->name, lib->func, 1);
            lua_pop(state_, 1); /* remove lib */
        }

        LuaRegisterCoreLibraries(state_);

        for (size_t i = 0; i < config_.modules_.size(); i++)
        {
            config_.modules_[i].lua_open_func_(state_);
            lua_setglobal(state_, config_.modules_[i].name_.c_str());
        }

        // replace searchers with only preload and custom searcher
        lua_getglobal(state_, "package");
        lua_getfield(state_, -1, "searchers");
        lua_newtable(state_);
        lua_geti(state_, -2, 1);
        lua_seti(state_, -2, 1);
        lua_pushcfunction(state_, LuaPackSearcher);
        lua_seti(state_, -2, 2);
        lua_setfield(state_, -3, "searchers");
        // pop package and searchers off stack
        lua_pop(state_, 2);

        Sandbox();

        if (config_.debug_enabled_)
        {
            lua_newtable(state_);
            lua_setglobal(state_, "__ec_debugger_source");
            dbg_setup_default(state_);
        }
        else
        {
            lua_pushcfunction(state_, LuaDbgNOP);
            lua_setglobal(state_, "dbg");
        }

        EPI_ASSERT(!lua_gettop(state_));
    }

    lua_State* GetLuaState()
    {
        return state_;
    }

    // NOP dbg() for when debugger is disabled and someone has left some breakpoints
    // in code
    static int LuaDbgNOP(lua_State *L)
    {
        LuaState *state = Get(L);

        if (state->debug_warn_)
        {
            state->debug_warn_ = false;
            LogWarning("LUA: dbg() called without lua_debug being set.  Please check that "
                       "a stray dbg call didn't get left "
                       "in source.");
        }
        return 0;
    }

    static int LuaMsgHandler(lua_State *L)
    {
        const char *msg = lua_tostring(L, 1);
        if (msg == nullptr)
        {                                            /* is error object not a string? */
            if (luaL_callmeta(L, 1, "__tostring") && /* does it have a metamethod */
                lua_type(L, -1) == LUA_TSTRING)      /* that produces a string? */
                return 1;                            /* that is the message */
            else
                msg = lua_pushfstring(L, "(error object is a %s value)", luaL_typename(L, 1));
        }
        luaL_traceback(L, L, msg, 1); /* append a standard traceback */
        return 1;                     /* return the traceback */
    }

    static int LuaPackLoader(lua_State *L)
    {
        LuaState *lstate = Get(L);

        const char *name = luaL_checkstring(L, 1);

        std::string pack_name;
        GetRequirePackPath(name, pack_name);

        epi::File *file = lstate->config_.open_file_callback_(pack_name);

        if (!file)
        {
            FatalError("LUA: %s.lua: NOT FOUND\n", name);
            return 0;
        }

        std::string source = file->ReadText();

        delete file;

        int results = lstate->DoFile(pack_name.c_str(), source.c_str());
        return results;
    }

    static int LuaPackSearcher(lua_State *L)
    {
        LuaState *lstate = Get(L);

        const char *name = luaL_checkstring(L, 1);

        std::string pack_name;
        GetRequirePackPath(name, pack_name);

        if (lstate->config_.check_file_callback_(pack_name) == -1)
        {
            FatalError("LUA: Unable to load file %s", pack_name.c_str());
            return 0;
        }

        lua_pushcfunction(L, LuaPackLoader);
        lua_pushstring(L, name);
        return 2;
    }

    static int SandboxWarning(lua_State *L)
    {
        const char *function_name = luaL_checkstring(L, lua_upvalueindex(1));
        LogWarning("LUA: Called sandbox disabled function %s\n", function_name);
        return 0;
    }

    void LuaError(const char *msg, const char *luaerror)
    {
        std::string error(luaerror);
        std::replace(error.begin(), error.end(), '\t', '>');

        FatalError("%s", (msg + error).c_str());
    }

    static void GetRequirePackPath(const char *name, std::string &out)
    {
        std::string require_name(name);
        std::replace(require_name.begin(), require_name.end(), '.', '/');
        out = epi::StringFormat("scripts/lua/%s.lua", require_name.c_str());
    }

    static LuaState *Get(lua_State *L)
    {
        auto state = state_lookup_.find(L);
        if (state == state_lookup_.end())
        {
            FatalError("Unknown Lua State");
        }

        return state->second;
    }

    int DoFile(const char *filename, const char *source)
    {
        if (config_.debug_enabled_)
        {
            lua_getglobal(state_, "__ec_debugger_source");
            lua_getfield(state_, -1, filename);
            if (lua_isstring(state_, -1))
            {
                LogWarning("LUA: Redundant execution of %s", filename);
                lua_pop(state_, 2);
                return 0;
            }
            lua_pop(state_, 1);
            lua_pushstring(state_, source);
            lua_setfield(state_, -2, filename);
            lua_pop(state_, 1);
        }
        int top    = lua_gettop(state_);
        int status = luaL_loadbuffer(state_, source, strlen(source), (std::string("@") + filename).c_str());

        if (status != LUA_OK)
        {
            LuaError(epi::StringFormat("LUA: Error compiling %s\n", filename ? filename : "???").c_str(),
                     lua_tostring(state_, -1));
        }

        if (config_.debug_enabled_)
        {
            status = dbg_pcall(state_, 0, LUA_MULTRET, 0);
        }
        else
        {
            int base = lua_gettop(state_);            // function index
            lua_pushcfunction(state_, LuaMsgHandler); // push message handler */
            lua_insert(state_, base);                 // put it under function and args */
            status = lua_pcall(state_, 0, LUA_MULTRET, base);
            lua_remove(state_, base);
        }

        if (status != LUA_OK)
        {
            LuaError(epi::StringFormat("LUA: Error in %s\n", filename ? filename : "???").c_str(),
                     lua_tostring(state_, -1));
        }

        return lua_gettop(state_) - top;
    }

    void CallGlobalFunction(const char *function_name)
    {
        int top = lua_gettop(state_);
        lua_getglobal(state_, function_name);
        int status = 0;
        if (config_.debug_enabled_)
        {
            status = dbg_pcall(state_, 0, 0, 0);
        }
        else
        {
            int base = lua_gettop(state_);                 // function index
            lua_pushcfunction(state_, LuaMsgHandler); // push message handler */
            lua_insert(state_, base);                 // put it under function and args */

            status = lua_pcall(state_, 0, 0, base);
        }

        if (status != LUA_OK)
        {
            LuaError(epi::StringFormat("Error calling global function %s\n", function_name).c_str(),
                     lua_tostring(state_, -1));
        }

        lua_settop(state_, top);
    }

    bool DoFile(const char *filename)
    {
        File *file = config_.open_file_callback_(filename);
        EPI_ASSERT(file);

        std::string source = file->ReadText();

        DoFile(filename, source.c_str());

        return true;
    }

    void SandboxModule(const char *module_name, const char **functions)
    {
        int i = 0;
        lua_getglobal(state_, module_name);
        while (const char *function_name = functions[i++])
        {
            lua_pushfstring(state_, "%s.%s", module_name, function_name);
            lua_pushcclosure(state_, SandboxWarning, 1);
            lua_setfield(state_, -2, function_name);
        }
        lua_pop(state_, 1);
    }

    void Sandbox()
    {
        // clear out search path and loadlib
        lua_getglobal(state_, "package");
        lua_pushnil(state_);
        lua_setfield(state_, -2, "loadlib");
        lua_pushnil(state_);
        lua_setfield(state_, -2, "searchpath");
        // pop package off stack
        lua_pop(state_, 1);

        // os module
        const char *os_functions[] = {"execute", "exit", "getenv", "remove", "rename", "setlocale", "tmpname", nullptr};
        SandboxModule("os", os_functions);

        // base/global functions
        const char *base_functions[] = {"dofile", "loadfile", nullptr};
        SandboxModule("_G", base_functions);

        // if debugging is enabled, load debug/io libs and sandbox
        if (config_.debug_enabled_)
        {
            // open the debug library and io libraries
            luaL_requiref(state_, LUA_DBLIBNAME, luaopen_debug, 1);
            luaL_requiref(state_, LUA_IOLIBNAME, luaopen_io, 1);
            lua_pop(state_, 2);

            const char *io_functions[] = {"close", "input",   "lines", "open", "output",
                                          "popen", "tmpfile", "type",  nullptr};
            SandboxModule("io", io_functions);
        }
    }

    bool       debug_warn_ = true;
    lua_State *state_;

    LuaConfig config_;

    static std::unordered_map<ENameIndex, LuaState *>  states_;
    static std::unordered_map<lua_State *, LuaState *> state_lookup_;
};

std::unordered_map<ENameIndex, LuaState *>  LuaState::states_;
std::unordered_map<lua_State *, LuaState *> LuaState::state_lookup_;

ILuaState *ILuaState::CreateVM(const LuaConfig &config)
{
    if (LuaState::states_.find(config.name_.GetIndex()) != LuaState::states_.end())
    {
        FatalError("Duplicate LuaStates created with same ename");
    }

    return new LuaState(config);
}

// Core

//------------------------------------------------------------------------
//  SYSTEM MODULE
//------------------------------------------------------------------------

// sys.error(str)
//
static int SYS_error(lua_State *L)
{
    const char *s = luaL_checkstring(L, 1);
    FatalError("%s\n", s);
    return 0;
}

// sys.print(str)
//
static int SYS_print(lua_State *L)
{
    const char *s = luaL_checkstring(L, 1);
    LogPrint("%s\n", s);
    return 0;
}

// sys.debug_print(str)
//
static int SYS_debug_print(lua_State *L)
{
    const char *s = luaL_checkstring(L, 1);
    LogDebug("%s\n", s);
    return 0;
}

// sys.edge_version()
//
static int SYS_edge_version(lua_State *L)
{
    lua_pushnumber(L, 1.5);
    return 1;
}

#ifdef WIN32
static bool console_allocated = false;
#endif
static int SYS_AllocConsole(lua_State *L)
{
    EPI_UNUSED(L);
#ifdef WIN32
    if (console_allocated)
    {
        return 0;
    }

    console_allocated = true;
    AllocConsole();
    freopen("CONIN$", "r", stdin);
    freopen("CONOUT$", "w", stdout);
    freopen("CONOUT$", "w", stderr);
#endif
    return 0;
}

// SYSTEM
static const luaL_Reg syslib[] = {{"error", SYS_error},
                                  {"print", SYS_print},
                                  {"debug_print", SYS_debug_print},
                                  {"edge_version", SYS_edge_version},
                                  {"allocate_console", SYS_AllocConsole},
                                  {nullptr, nullptr}};

static int luaopen_sys(lua_State *L)
{
    luaL_newlib(L, syslib);
    return 1;
}

const luaL_Reg loadlibs[] = {{"sys", luaopen_sys}, {nullptr, nullptr}};

void LuaRegisterCoreLibraries(lua_State *L)
{
    const luaL_Reg *lib;
    /* "require" functions from 'loadedlibs' and set results to global table */
    for (lib = loadlibs; lib->func; lib++)
    {
        luaL_requiref(L, lib->name, lib->func, 1);
        lua_pop(L, 1); /* remove lib */
    }
}

// Debugger

int luaopen_debugger(lua_State *lua)
{
    LuaState *state = LuaState::Get(lua);

    epi::File *file = state->config_.open_file_callback_("scripts/lua/core/debugger.lua");

    EPI_ASSERT(file);

    std::string debugger_source = file->ReadText();

    delete file;

    if (luaL_loadbufferx(lua, debugger_source.c_str(), debugger_source.length(), "<debugger.lua>", nullptr) ||
        lua_pcall(lua, 0, LUA_MULTRET, 0))
        lua_error(lua);

    // Or you could load it from disk:
    // if(luaL_dofile(lua, "debugger.lua")) lua_error(lua);

    return 1;
}

static const char *MODULE_NAME = "DEBUGGER_LUA_MODULE";
static const char *MSGH        = "DEBUGGER_LUA_MSGH";

void dbg_setup(lua_State *lua, const char *name, const char *globalName, lua_CFunction readFunc,
               lua_CFunction writeFunc)
{
    // Check that the module name was not already defined.
    lua_getfield(lua, LUA_REGISTRYINDEX, MODULE_NAME);
    assert(lua_isnil(lua, -1) || strcmp(name, luaL_checkstring(lua, -1)));
    lua_pop(lua, 1);

    // Push the module name into the registry.
    lua_pushstring(lua, name);
    lua_setfield(lua, LUA_REGISTRYINDEX, MODULE_NAME);

    // Preload the module
    luaL_requiref(lua, name, luaopen_debugger, false);

    // Insert the msgh function into the registry.
    lua_getfield(lua, -1, "msgh");
    lua_setfield(lua, LUA_REGISTRYINDEX, MSGH);

    if (readFunc)
    {
        lua_pushcfunction(lua, readFunc);
        lua_setfield(lua, -2, "read");
    }

    if (writeFunc)
    {
        lua_pushcfunction(lua, writeFunc);
        lua_setfield(lua, -2, "write");
    }

    if (globalName)
    {
        lua_setglobal(lua, globalName);
    }
    else
    {
        lua_pop(lua, 1);
    }
}

void dbg_setup_default(lua_State *lua)
{
    dbg_setup(lua, "debugger", "dbg", nullptr, nullptr);
}

int dbg_pcall(lua_State *lua, int nargs, int nresults, int msgh)
{
    // Call regular lua_pcall() if a message handler is provided.
    if (msgh)
        return lua_pcall(lua, nargs, nresults, msgh);

    // Grab the msgh function out of the registry.
    lua_getfield(lua, LUA_REGISTRYINDEX, MSGH);
    if (lua_isnil(lua, -1))
    {
        luaL_error(lua, "Tried to call dbg_call() before calling dbg_setup().");
    }

    // Move the error handler just below the function.
    msgh = lua_gettop(lua) - (1 + nargs);
    lua_insert(lua, msgh);

    // Call the function.
    int err = lua_pcall(lua, nargs, nresults, msgh);

    // Remove the debug handler.
    lua_remove(lua, msgh);

    return err;
}

} // namespace epi