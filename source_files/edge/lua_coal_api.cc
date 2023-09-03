
#include "i_defs.h"
#include "version.h"
#include "dm_state.h"
#include "m_random.h"
#include "w_wad.h"
#include "lua_edge.h"

using namespace elua;

extern cvar_c r_doubleframes;

// COAL API COMPATIBILITY LAYER

//------------------------------------------------------------------------
//  SYSTEM MODULE
//------------------------------------------------------------------------

// sys.error(str)
//
static void SYS_error(const char* message)
{
    I_Error("%s\n", message);
}

// sys.print(str)
//
static void SYS_print(const char* message)
{
    I_Printf("%s\n", message);
}

// sys.debug_print(str)
//
static void SYS_debug_print(const char* message)
{
    I_Debugf("%s\n", message);
}

// sys.edge_version()
//
static double SYS_edge_version()
{
    return (double) edgeversion.f;
}

//------------------------------------------------------------------------
//  MATH MODULE
//------------------------------------------------------------------------

// math.rint(val)
static int MATH_rint(double val)
{
    return I_ROUND(val);
}

// math.floor(val)
static double MATH_floor(double val)
{
    return floor(val);
}

// math.ceil(val)
static double MATH_ceil(double val)
{
    return ceil(val);
}

// math.random()
static double MATH_random()
{
    return C_Random() / double(0x10000);
}

static double MATH_random2()
{
    return double(C_Random() % 11);
}

// math.cos(val)
static double MATH_cos(double val)
{
    return cos(val * M_PI / 180.0);
}

// math.sin(val)
static double MATH_sin(double val)
{
    return sin(val * M_PI / 180.0);
}

// math.tan(val)
static double MATH_tan(double val)
{
    return tan(val * M_PI / 180.0);
}

// math.acos(val)
static double MATH_acos(double val)
{
    return acos(val) * 180.0 / M_PI;
}

// math.asin(val)
static double MATH_asin(double val)
{
    return asin(val) * 180.0 / M_PI;
}

// math.atan(val)
static double MATH_atan(double val)
{
    return atan(val) * 180.0 / M_PI;
}

// math.atan2(x, y)
static double MATH_atan2(double x, double y)
{
    return atan2(y, x) * 180.0 / M_PI;
}

// math.log(val)
static double MATH_log(double val)
{
    if (val <= 0)
        I_Error("math.log: illegal input: %g\n", val);

    return log(val);
}

//------------------------------------------------------------------------
//  STRINGS MODULE
//------------------------------------------------------------------------

// strings.len(s)
//
static size_t STRINGS_len(const char* s)
{
    return strlen(s);
}

// Lobo: December 2021
//  strings.find(s,TextToFind)
//  returns substring position or -1 if not found
//
static int STRINGS_find(const char* s1, const char* s2)
{
    std::string str(s1);
    std::string str2(s2);

    return str.find(str2);
}

// strings.sub(s, start, end)
//
static std::string STRINGS_sub(const char* s, int start, int end)
{
    int len = strlen(s);

    // negative values are relative to END of the string (-1 = last character)
    if (start < 0)
        start += len + 1;
    if (end < 0)
        end += len + 1;

    if (start < 1)
        start = 1;
    if (end > len)
        end = len;

    if (end < start)
    {
        return "";
    }

    SYS_ASSERT(end >= 1 && start <= len);

    // translate into C talk
    start--;
    end--;

    int new_len = (end - start + 1);

    return std::string(s + start, new_len);
}

// strings.tonumber(s)
//
static double STRINGS_tonumber(const char* s)
{
    return double(atof(s));
}

static lua_vm_c* vm_lua_coal = nullptr;
static double ticrate = 35.0;
static double pi = 3.1415926535897932384;
static double evar = 2.7182818284590452354;

class lua_coal_api_c : public lua_module_c
{
public:
    lua_coal_api_c(lua_vm_c* vm) : lua_module_c(vm, "coal") {}

    void Open()
    {
        OpenSys();
        OpenMath();
        OpenStrings();
        OpenHud();
        OpenPlayer();
    }

private:
    void OpenSys()
    {
        lua_State* state = vm_->GetState();

        luabridge::getGlobalNamespace(state)
            // sys
            .beginNamespace("sys")
            .addFunction("error", SYS_error)
            .addFunction("print", SYS_print)
            .addFunction("debug_print", SYS_debug_print)
            .addFunction("edge_version", SYS_edge_version)
            .addProperty("TICRATE", &ticrate, false)
            .addProperty(
                "gametic", +[] { return gametic / (r_doubleframes.d ? 2 : 1); })
            .endNamespace();
    }

    void OpenMath()
    {
        lua_State* state = vm_->GetState();

        luabridge::getGlobalNamespace(state)
            .beginNamespace("math")
            .addFunction("rint", MATH_rint)
            .addFunction("floor", MATH_floor)
            .addFunction("ceil", MATH_ceil)
            .addFunction("random", MATH_random)
            .addFunction("random2", MATH_random2)
            .addFunction("cos", MATH_cos)
            .addFunction("sin", MATH_sin)
            .addFunction("tan", MATH_tan)
            .addFunction("acos", MATH_acos)
            .addFunction("asin", MATH_asin)
            .addFunction("atan", MATH_atan)
            .addFunction("atan2", MATH_atan2)
            .addFunction("log", MATH_log)
            // vec3 extensions
            .addFunction(
                "getx",
                +[](luabridge::LuaRef ref) {
                    SYS_ASSERT(ref.isTable());
                    SYS_ASSERT(ref[1].isNumber());
                    return ref[1].cast<double>().value();
                })
            .addFunction(
                "gety",
                +[](luabridge::LuaRef ref) {
                    SYS_ASSERT(ref.isTable());
                    SYS_ASSERT(ref[2].isNumber());
                    return ref[2].cast<double>().value();
                })
            .addFunction(
                "getz",
                +[](luabridge::LuaRef ref) {
                    SYS_ASSERT(ref.isTable());
                    SYS_ASSERT(ref[3].isNumber());
                    return ref[3].cast<double>().value();
                })
            .addFunction(
                "vlen",
                +[](luabridge::LuaRef ref) {
                    SYS_ASSERT(ref.isTable());
                    luabridge::LuaResult result = ref["length"](ref);
                    return result[0].cast<double>().value();
                })

            .addFunction(
                "normalize",
                +[](luabridge::LuaRef ref) {
                    SYS_ASSERT(ref.isTable());
                    return ref["normalize"](ref)[0];
                })

            .addFunction(
                "rand_range",
                +[](double low, double high) { return low + (high - low) * MATH_random(); })

            .addFunction(
                "abs",
                +[](double n) {
                    if (n < 0)
                    {
                        return 0 - n;
                    }
                    return n;
                })
            .addFunction(
                "sqrt", +[](double n) { return sqrt(n); })
            .addFunction(
                "min", +[](double a, double b) { return MIN(a, b); })
            .addFunction(
                "max", +[](double a, double b) { return MAX(a, b); })
            //.addProperty("pi", &pi, false) <--- already in math
            //.addProperty("e", &evar, false)
            .endNamespace();
    }

    void OpenStrings()
    {
        lua_State* state = vm_->GetState();

        luabridge::getGlobalNamespace(state)
            .beginNamespace("strings")
            .addFunction("len", STRINGS_len)
            .addFunction("sub", STRINGS_sub)
            .addFunction("tonumber", STRINGS_tonumber)
            .addFunction("find", STRINGS_find)
            .endNamespace();
    }

    void OpenHud()
    {
        lua_State* state = vm_->GetState();
        void LUA_Coal_OpenHud(lua_State * state);
        LUA_Coal_OpenHud(state);
    }

    void OpenPlayer()
    {
        lua_State* state = vm_->GetState();
        void LUA_Coal_OpenPlayer(lua_State * state);
        LUA_Coal_OpenPlayer(state);
    }
};

void LUA_Coal_Init()
{
    SYS_ASSERT(!vm_lua_coal);
    vm_lua_coal = LUA_CreateEdgeVM(LUA_VM_EDGE_COAL);
    vm_lua_coal->AddModule<lua_coal_api_c>();

    vm_lua_coal->DoFile("edge_defs/lua/test2.lua");
}

class pending_lua_script_c
{
public:
    int type = 0;
    std::string data = "";
    std::string source = "";
};

static std::vector<pending_lua_script_c> unread_scripts;

void LUA_Coal_AddScript(int type, std::string& data, const std::string& source)
{
    unread_scripts.push_back(pending_lua_script_c{type, "", source});

    // transfer the caller's data
    unread_scripts.back().data.swap(data);
}

void LUA_Coal_LoadScripts()
{
    for (auto& info : unread_scripts)
    {
        const char* name = info.source.c_str();
        char* data = (char*) info.data.c_str(); // FIXME make param to CompileFile be a std::string&

        I_Printf("Compiling: %s\n", name);

        vm_lua_coal->DoString(data);
    }

    unread_scripts.clear();

    if (W_IsLumpInPwad("STBAR"))
    {
        luabridge::getGlobal(vm_lua_coal->GetState(), "hud")["custom_stbar"] = true;
    }
}
