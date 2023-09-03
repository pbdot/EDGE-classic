

#include "i_defs.h"

#include "types.h"

#include "p_local.h"
#include "p_mobj.h"
#include "r_state.h"

#include "dm_state.h"
#include "e_main.h"
#include "g_game.h"

#include "e_player.h"
#include "hu_font.h"
#include "hu_draw.h"
#include "r_modes.h"
#include "r_image.h"
#include "r_sky.h"

#include "f_interm.h" //Lobo: need this to get access to wi_stats
#include "rad_trig.h" //Lobo: need this to access RTS

#include <charconv>

#include "flat.h" // DDFFLAT - Dasho
#include "s_sound.h" // play_footstep() - Dasho

#include "lua_edge.h"

static lua_State* player_state = nullptr;
player_t* lua_ui_player_who = nullptr;

// AuxStringReplaceAll("Our_String", std::string("_"), std::string(" "));
//
std::string AuxStringReplaceAll(std::string str, const std::string& from, const std::string& to)
{
    size_t start_pos = 0;
    while ((start_pos = str.find(from, start_pos)) != std::string::npos)
    {
        str.replace(start_pos, from.length(), to);
        start_pos += to.length(); // Handles case where 'to' is a substring of 'from'
    }
    return str;
}

// GetMobjBenefits(mobj);
//
std::string GetMobjBenefits(mobj_t* obj, bool KillBenefits = false)
{
    std::string temp_string;
    temp_string.clear();
    benefit_t* list;
    int temp_num = 0;

    if (KillBenefits)
        list = obj->info->kill_benefits;
    else
        list = obj->info->pickup_benefits;

    for (; list != NULL; list = list->next)
    {
        switch (list->type)
        {
        case BENEFIT_Weapon:
            // If it's a weapon all bets are off: we'll want to parse
            // it differently, not here.
            temp_string = "WEAPON=1";
            break;

        case BENEFIT_Ammo:
            temp_string += "AMMO" + std::to_string((int) list->sub.type + 1);
            temp_string += "=" + std::to_string((int) list->amount);
            break;

        case BENEFIT_Health: // only benefit without a sub.type so just give it 01
            temp_string += "HEALTH01=" + std::to_string((int) list->amount);
            break;

        case BENEFIT_Armour:
            temp_string += "ARMOUR" + std::to_string((int) list->sub.type + 1);
            temp_string += "=" + std::to_string((int) list->amount);
            break;

        case BENEFIT_Inventory:
            temp_string += "INVENTORY";
            if ((list->sub.type + 1) < 10)
                temp_string += "0";
            temp_string += std::to_string((int) list->sub.type + 1);
            temp_string += "=" + std::to_string((int) list->amount);
            break;

        case BENEFIT_Counter:
            temp_string += "COUNTER";
            if ((list->sub.type + 1) < 10)
                temp_string += "0";
            temp_string += std::to_string((int) list->sub.type + 1);
            temp_string += "=" + std::to_string((int) list->amount);
            break;

        case BENEFIT_Key:
            temp_string += "KEY";
            temp_num = log2((int) list->sub.type);
            temp_num++;
            temp_string += std::to_string(temp_num);
            break;

        case BENEFIT_Powerup:
            temp_string += "POWERUP" + std::to_string((int) list->sub.type + 1);
            break;

        default:
            break;
        }
    }
    return temp_string;
}

// GetQueryInfoFromMobj(mobj, whatinfo)
//
std::string GetQueryInfoFromMobj(mobj_t* obj, int whatinfo)
{
    int temp_num = 0;
    std::string temp_string;
    temp_string.clear();

    switch (whatinfo)
    {
    case 1: // name
        if (obj)
        {
            // try CAST_TITLE first
            temp_string = language[obj->info->cast_title];

            if (temp_string.empty()) // fallback to DDFTHING entry name
            {
                temp_string = obj->info->name;
                temp_string = AuxStringReplaceAll(temp_string, std::string("_"), std::string(" "));
            }
        }
        break;

    case 2: // current health
        if (obj)
        {
            temp_num = obj->health;
            temp_string = std::to_string(temp_num);
        }
        break;

    case 3: // spawn health
        if (obj)
        {
            temp_num = obj->info->spawnhealth;
            temp_string = std::to_string(temp_num);
        }
        break;

    case 4: // pickup_benefits
        if (obj)
        {
            temp_string = GetMobjBenefits(obj, false);
        }
        break;

    case 5: // kill_benefits
        if (obj)
        {
            temp_string = GetMobjBenefits(obj, true);
        }
        break;
    }

    if (temp_string.empty())
        return ("");

    return (temp_string.c_str());
}

static void COAL_SetPsprite(player_t* p, int position, int stnum, weapondef_c* info = NULL)
{
    pspdef_t* psp = &p->psprites[position];

    if (stnum == S_NULL)
    {
        // object removed itself
        psp->state = psp->next_state = NULL;
        return;
    }

    // state is old? -- Mundo hack for DDF inheritance
    if (info && stnum < info->state_grp.back().first)
    {
        state_t* st = &states[stnum];

        if (st->label)
        {
            statenum_t new_state = DDF_StateFindLabel(info->state_grp, st->label, true /* quiet */);
            if (new_state != S_NULL)
                stnum = new_state;
        }
    }

    state_t* st = &states[stnum];

    // model interpolation stuff
    if (psp->state && (st->flags & SFF_Model) && (psp->state->flags & SFF_Model) &&
        (st->sprite == psp->state->sprite) && st->tics > 1)
    {
        p->weapon_last_frame = psp->state->frame;
    }
    else
        p->weapon_last_frame = -1;

    psp->state = st;
    psp->tics = st->tics;
    psp->next_state = (st->nextstate == S_NULL) ? NULL : (states + st->nextstate);

    // call action routine

    p->action_psp = position;

    if (st->action)
        (*st->action)(p->mo);
}

//
// P_SetPspriteDeferred
//
// -AJA- 2004/11/05: This is preferred method, doesn't run any actions,
//       which (ideally) should only happen during P_MovePsprites().
//
void COAL_SetPspriteDeferred(player_t* p, int position, int stnum)
{
    pspdef_t* psp = &p->psprites[position];

    if (stnum == S_NULL || psp->state == NULL)
    {
        COAL_SetPsprite(p, position, stnum);
        return;
    }

    psp->tics = 0;
    psp->next_state = (states + stnum);
}

// GetQueryInfoFromWeapon(mobj, whatinfo, [secattackinfo])
//
std::string GetQueryInfoFromWeapon(mobj_t* obj, int whatinfo, bool secattackinfo = false)
{
    int temp_num = 0;
    std::string temp_string;
    temp_string.clear();

    if (!obj->info->pickup_benefits)
        return "";
    if (!obj->info->pickup_benefits->sub.weap)
        return "";
    if (obj->info->pickup_benefits->type != BENEFIT_Weapon)
        return "";

    weapondef_c* objWep = obj->info->pickup_benefits->sub.weap;
    if (!objWep)
        return "";

    int attacknum = 0; // default to primary attack
    if (secattackinfo)
        attacknum = 1;

    atkdef_c* objAtck = objWep->attack[attacknum];
    if (!objAtck && whatinfo > 2)
        return ""; // no attack to get info about (only should happen with secondary attacks)

    const damage_c* damtype;

    float temp_num2;

    switch (whatinfo)
    {
    case 1: // name
        temp_string = objWep->name;
        temp_string = AuxStringReplaceAll(temp_string, std::string("_"), std::string(" "));
        break;

    case 2: // ZOOM_FACTOR
        temp_num2 = 90.0f / objWep->zoom_fov;
        temp_string = std::to_string(temp_num2);
        break;

    case 3: // AMMOTYPE
        temp_num = (objWep->ammo[attacknum]) + 1;
        temp_string = std::to_string(temp_num);
        break;

    case 4: // AMMOPERSHOT
        temp_num = objWep->ammopershot[attacknum];
        temp_string = std::to_string(temp_num);
        break;

    case 5: // CLIPSIZE
        temp_num = objWep->clip_size[attacknum];
        temp_string = std::to_string(temp_num);
        break;

    case 6: // DAMAGE Nominal
        damtype = &objAtck->damage;
        temp_num = damtype->nominal;
        temp_string = std::to_string(temp_num);
        break;

    case 7: // DAMAGE Max
        damtype = &objAtck->damage;
        temp_num = damtype->linear_max;
        temp_string = std::to_string(temp_num);
        break;

    case 8: // Range
        temp_num = objAtck->range;
        temp_string = std::to_string(temp_num);
        break;

    case 9: // AUTOMATIC
        if (objWep->autofire[attacknum])
            temp_string = "1";
        else
            temp_string = "0";
        break;
    }

    if (temp_string.empty())
        return ("");

    return (temp_string.c_str());
}

void LUA_Coal_OpenPlayer(lua_State* state)
{
    player_state = state;

    luabridge::LuaRef vec3 = luabridge::getGlobal(player_state, "vec3");
    luabridge::LuaRef inventory_event_handler = vec3(0, 0, 0)[0];

    luabridge::getGlobalNamespace(state)
        .beginNamespace("player")
        .addVariable("inventory_event_handler", &inventory_event_handler)
        .addVariable("STOP_TIME", 10)
        .addFunction(
            "num_players", +[] { return numplayers; })
        .addFunction(
            "set_who",
            +[](int index) {
                if (index < 0 || index >= numplayers)
                    I_Error(
                        "player.set_who: bad index value: %d (numplayers=%d)\n", index, numplayers);

                if (index == 0)
                {
                    lua_ui_player_who = players[consoleplayer];
                    return;
                }

                int who = displayplayer;

                for (; index > 1; index--)
                {
                    do
                    {
                        who = (who + 1) % MAXPLAYERS;
                    } while (players[who] == NULL);
                }

                lua_ui_player_who = players[who];
            })
        .addFunction(
            "is_bot", +[] { return lua_ui_player_who->playerflags & PFL_Bot ? true : false; })
        .addFunction(
            "get_name", +[] { return lua_ui_player_who->playername; })
        .addFunction(
            "get_pos",
            +[] {
                luabridge::LuaRef pos = luabridge::newTable(player_state);
                pos[1] = lua_ui_player_who->mo->x;
                pos[2] = lua_ui_player_who->mo->y;
                pos[3] = lua_ui_player_who->mo->z;
                return pos;
            })
        .addFunction(
            "get_angle",
            +[] {
                float value = ANG_2_FLOAT(lua_ui_player_who->mo->angle);

                if (value > 360.0f)
                    value -= 360.0f;
                if (value < 0)
                    value += 360.0f;
                return value;
            })
        .addFunction(
            "get_mlook",
            +[] {
                float value = ANG_2_FLOAT(lua_ui_player_who->mo->vertangle);

                if (value > 180.0f)
                    value -= 360.0f;

                return value;
            })
        .addFunction(
            "health",
            +[] {
                float h =
                    lua_ui_player_who->health * 100 / lua_ui_player_who->mo->info->spawnhealth;

                if (h < 98)
                    h += 0.99f;

                return floor(h);
            })
        .addFunction(
            "armor",
            +[](int kind) {
                if (kind < 1 || kind > NUMARMOUR)
                    I_Error("player.armor: bad armor index: %d\n", kind);

                kind--;

                float a = lua_ui_player_who->armours[kind];
                if (a < 98)
                    a += 0.99f;

                return floor(a);
            })
        .addFunction(
            "total_armor",
            +[]() {
                float a = lua_ui_player_who->totalarmour;
                if (a < 98)
                    a += 0.99f;

                return floor(a);
            })
        .addFunction(
            "ammo",
            +[](int ammo) {
                if (ammo < 1 || ammo > NUMAMMO)
                    I_Error("player.ammo: bad ammo number: %d\n", ammo);

                ammo--;

                return lua_ui_player_who->ammo[ammo].num;
            })
        .addFunction(
            "ammomax",
            +[](int ammo) {
                if (ammo < 1 || ammo > NUMAMMO)
                    I_Error("player.ammo: bad ammo number: %d\n", ammo);

                ammo--;

                return lua_ui_player_who->ammo[ammo].max;
            })
        .addFunction(
            "frags", +[]() { return lua_ui_player_who->frags; })
        .addFunction(
            "is_swimming", +[]() { return lua_ui_player_who->swimming ? true : false; })
        .addFunction(
            "is_jumping", +[]() { return lua_ui_player_who->jumpwait > 0 ? true : false; })
        .addFunction(
            "is_crouching",
            +[]() { return (lua_ui_player_who->mo->extendedflags & EF_CROUCHING) ? true : false; })
        .addFunction(
            "is_using", +[]() { return lua_ui_player_who->usedown ? true : false; })
        .addFunction(
            "is_action1", +[]() { return lua_ui_player_who->actiondown[0] ? true : false; })
        .addFunction(
            "is_action2", +[]() { return lua_ui_player_who->actiondown[1] ? true : false; })
        .addFunction(
            "is_attacking",
            +[]() {
                return (lua_ui_player_who->attackdown[0] || lua_ui_player_who->attackdown[1])
                           ? true
                           : false;
            })
        .addFunction(
            "is_rampaging",
            +[]() { return (lua_ui_player_who->attackdown_count >= 70) ? true : false; })
        .addFunction(
            "is_grinning", +[]() { return (lua_ui_player_who->grin_count >= 0) ? true : false; })
        .addFunction(
            "under_water", +[]() { return lua_ui_player_who->underwater ? true : false; })
        .addFunction(
            "on_ground",
            +[]() { // not a 3D floor?
                if (lua_ui_player_who->mo->subsector->sector->exfloor_used == 0)
                {
                    // on the edge above water/lava/etc? Handles edge walker case
                    if (lua_ui_player_who->mo->floorz !=
                        lua_ui_player_who->mo->subsector->sector->f_h)
                        return false;
                    else
                    {
                        // touching the floor? Handles jumping or flying
                        if (lua_ui_player_who->mo->z <= lua_ui_player_who->mo->floorz)
                            return true;
                        else
                            return false;
                    }
                }
                else
                {
                    if (lua_ui_player_who->mo->z <= lua_ui_player_who->mo->floorz)
                        return true;
                    else
                        return false;
                }
            })
        .addFunction(
            "move_speed", +[]() { return lua_ui_player_who->actual_speed; })
        .addFunction(
            "air_in_lungs",
            +[]() {
                if (lua_ui_player_who->air_in_lungs <= 0)
                {
                    return 0.0f;
                }

                float value = lua_ui_player_who->air_in_lungs * 100.0f /
                              lua_ui_player_who->mo->info->lung_capacity;

                value = CLAMP(0.0f, value, 100.0f);

                return value;
            })
        .addFunction(
            "has_key",
            +[](int key) {
                if (key < 1 || key > 16)
                    I_Error("player.has_key: bad key number: %d\n", key);

                key--;

                return lua_ui_player_who->cards & (1 << key) ? true : false;
            })
        .addFunction(
            "has_power",
            +[](int power) {
                if (power < 1 || power > NUMPOWERS)
                    I_Error("player.has_power: bad powerup number: %d\n", power);

                power--;

                bool value = (lua_ui_player_who->powers[power] > 0) ? true : false;

                // special check for GOD mode
                if (power == PW_Invulnerable && (lua_ui_player_who->cheats & CF_GODMODE))
                    value = true;

                return value;
            })
        .addFunction(
            "power_left",
            +[](int power) {
                if (power < 1 || power > NUMPOWERS)
                    I_Error("player.power_left: bad powerup number: %d\n", power);

                power--;

                float value = lua_ui_player_who->powers[power];

                if (value > 0)
                    value /= TICRATE;

                return value;
            })

        .addFunction(
            "has_weapon",
            +[](const char* name) {
                for (int j = 0; j < MAXWEAPONS; j++)
                {
                    playerweapon_t* pw = &lua_ui_player_who->weapons[j];

                    if (pw->owned && !(pw->flags & PLWEP_Removing) &&
                        DDF_CompareName(name, pw->info->name.c_str()) == 0)
                    {
                        return true;
                    }
                }

                return false;
            })
        .addFunction(
            "has_weapon_slot",
            +[](int slot) {
                if (slot < 0 || slot > 9)
                    I_Error("player.has_weapon_slot: bad slot number: %d\n", slot);

                return lua_ui_player_who->avail_weapons[slot] ? true : false;
            })
        .addFunction(
            "cur_weapon",
            +[]() {
                if (lua_ui_player_who->pending_wp >= 0)
                {
                    return "change";
                }

                if (lua_ui_player_who->ready_wp < 0)
                {
                    return "none";
                }

                weapondef_c* info = lua_ui_player_who->weapons[lua_ui_player_who->ready_wp].info;

                return info->name.c_str();
            })
        .addFunction(
            "cur_weapon_slot",
            +[]() {
                int slot;
                if (lua_ui_player_who->ready_wp < 0)
                    slot = -1;
                else
                    slot = lua_ui_player_who->weapons[lua_ui_player_who->ready_wp].info->bind_key;

                return slot;
            })
        .addFunction(
            "main_ammo",
            +[]() {
                int value = 0;

                if (lua_ui_player_who->ready_wp >= 0)
                {
                    playerweapon_t* pw = &lua_ui_player_who->weapons[lua_ui_player_who->ready_wp];

                    if (pw->info->ammo[0] != AM_NoAmmo)
                    {
                        if (pw->info->show_clip)
                        {
                            SYS_ASSERT(pw->info->ammopershot[0] > 0);

                            value = pw->clip_size[0] / pw->info->ammopershot[0];
                        }
                        else
                        {
                            value = lua_ui_player_who->ammo[pw->info->ammo[0]].num;

                            if (pw->info->clip_size[0] > 0)
                                value += pw->clip_size[0];
                        }
                    }
                }

                return value;
            })
        .addFunction(
            "ammo_type",
            +[](int atk) {
                if (atk < 1 || atk > 2)
                    I_Error("player.ammo_type: bad attack number: %d\n", atk);

                atk--;

                int value = 0;

                if (lua_ui_player_who->ready_wp >= 0)
                {
                    playerweapon_t* pw = &lua_ui_player_who->weapons[lua_ui_player_who->ready_wp];

                    value = 1 + (int) pw->info->ammo[atk];
                }

                return value;
            })
        .addFunction(
            "ammo_pershot",
            +[](int atk) {
                if (atk < 1 || atk > 2)
                    I_Error("player.ammo_pershot: bad attack number: %d\n", atk);

                atk--;

                int value = 0;

                if (lua_ui_player_who->ready_wp >= 0)
                {
                    playerweapon_t* pw = &lua_ui_player_who->weapons[lua_ui_player_who->ready_wp];

                    value = pw->info->ammopershot[atk];
                }

                return value;
            })
        .addFunction(
            "clip_ammo",
            +[](int atk) {
                if (atk < 1 || atk > 2)
                    I_Error("player.clip_ammo: bad attack number: %d\n", atk);

                atk--;

                int value = 0;

                if (lua_ui_player_who->ready_wp >= 0)
                {
                    playerweapon_t* pw = &lua_ui_player_who->weapons[lua_ui_player_who->ready_wp];

                    value = pw->clip_size[atk];
                }

                return value;
            })
        .addFunction(
            "clip_size",
            +[](int atk) {
                if (atk < 1 || atk > 2)
                    I_Error("player.clip_size: bad attack number: %d\n", atk);

                atk--;

                int value = 0;

                if (lua_ui_player_who->ready_wp >= 0)
                {
                    playerweapon_t* pw = &lua_ui_player_who->weapons[lua_ui_player_who->ready_wp];

                    value = pw->info->clip_size[atk];
                }

                return value;
            })
        .addFunction(
            "clip_is_shared",
            +[]() {
                bool value = false;

                if (lua_ui_player_who->ready_wp >= 0)
                {
                    playerweapon_t* pw = &lua_ui_player_who->weapons[lua_ui_player_who->ready_wp];

                    if (pw->info->shared_clip)
                        value = true;
                }

                return value;
            })
        .addFunction(
            "hurt_by",
            +[]() {
                if (lua_ui_player_who->damagecount <= 0)
                {
                    return "";
                }

                // getting hurt because of your own damn stupidity
                if (lua_ui_player_who->attacker == lua_ui_player_who->mo)
                    return "self";
                else if (lua_ui_player_who->attacker &&
                         (lua_ui_player_who->attacker->side & lua_ui_player_who->mo->side))
                    return "friend";
                else if (lua_ui_player_who->attacker)
                    return "enemy";
                else
                    return "other";
            })
        .addFunction(
            "hurt_mon",
            +[]() {
                if (lua_ui_player_who->damagecount > 0 && lua_ui_player_who->attacker &&
                    lua_ui_player_who->attacker != lua_ui_player_who->mo)
                {
                    return lua_ui_player_who->attacker->info->name.c_str();
                }
                return "";
            })
        .addFunction(
            "hurt_pain", +[]() { return lua_ui_player_who->damage_pain; })
        .addFunction(
            "hurt_dir",
            +[]() {
                int dir = 0;

                if (lua_ui_player_who->attacker &&
                    lua_ui_player_who->attacker != lua_ui_player_who->mo)
                {
                    mobj_t* badguy = lua_ui_player_who->attacker;
                    mobj_t* pmo = lua_ui_player_who->mo;

                    angle_t diff =
                        R_PointToAngle(pmo->x, pmo->y, badguy->x, badguy->y) - pmo->angle;

                    if (diff >= ANG45 && diff <= ANG135)
                    {
                        dir = -1;
                    }
                    else if (diff >= ANG225 && diff <= ANG315)
                    {
                        dir = +1;
                    }
                }

                return dir;
            })
        .addFunction(
            "hurt_angle",
            +[]() {
                float value = 0;

                if (lua_ui_player_who->attacker &&
                    lua_ui_player_who->attacker != lua_ui_player_who->mo)
                {
                    mobj_t* badguy = lua_ui_player_who->attacker;
                    mobj_t* pmo = lua_ui_player_who->mo;

                    angle_t real_a = R_PointToAngle(pmo->x, pmo->y, badguy->x, badguy->y);

                    value = ANG_2_FLOAT(real_a);

                    if (value > 360.0f)
                        value -= 360.0f;

                    if (value < 0)
                        value += 360.0f;
                }

                return value;
            })
        .addFunction(
            "kills", +[]() { return lua_ui_player_who->killcount; })
        .addFunction(
            "secrets", +[]() { return lua_ui_player_who->secretcount; })
        .addFunction(
            "items", +[]() { return lua_ui_player_who->itemcount; })
        .addFunction(
            "map_enemies", +[]() { return wi_stats.kills; })
        .addFunction(
            "map_secrets", +[]() { return wi_stats.secret; })
        .addFunction(
            "map_items", +[]() { return wi_stats.items; })
        .addFunction(
            "floor_flat",
            +[]() { // If no 3D floors, just return the flat
                if (lua_ui_player_who->mo->subsector->sector->exfloor_used == 0)
                {
                    return lua_ui_player_who->mo->subsector->sector->floor.image->name.c_str();
                }
                else
                {
                    // Start from the lowest exfloor and check if the player is standing on it, then
                    // return the control sector's flat
                    float player_floor_height = lua_ui_player_who->mo->floorz;
                    extrafloor_t* floor_checker =
                        lua_ui_player_who->mo->subsector->sector->bottom_ef;
                    for (extrafloor_t* ef = floor_checker; ef; ef = ef->higher)
                    {
                        if (player_floor_height + 1 > ef->top_h)
                        {
                            return ef->top->image->name.c_str();
                        }
                    }
                    // Fallback if nothing else satisfies these conditions=
                    return lua_ui_player_who->mo->subsector->sector->floor.image->name.c_str();
                }
            })
        .addFunction(
            "sector_tag", +[]() { return lua_ui_player_who->mo->subsector->sector->tag; })
        .addFunction(
            "play_footstep",
            +[](const char* flat) {
                if (!flat)
                    I_Error("player.play_footstep: No flat name given!\n");

                flatdef_c* current_flatdef = flatdefs.Find(flat);

                if (!current_flatdef)
                    return;

                if (!current_flatdef->footstep)
                    return;
                else
                {
                    // Probably need to add check to see if the sfx is valid - Dasho
                    S_StartFX(current_flatdef->footstep);
                }
            })
        .addFunction(
            "use_inventory",
            +[](int num) {
                std::string script_name = "INVENTORY";
                int inv = 0;
                if (!num)
                    I_Error("player.use_inventory: can't parse inventory number!\n");
                else
                    inv = (int) num;

                if (inv < 1 || inv > NUMINV)
                    I_Error("player.use_inventory: bad inventory number: %d\n", inv);

                if (inv < 10)
                    script_name.append("0").append(std::to_string(inv));
                else
                    script_name.append(std::to_string(inv));

                inv--;

                //******
                // If the same inventory script is already running then
                // don't start the same one again
                if (!RAD_IsActiveByTag(NULL, script_name.c_str()))
                {
                    if (lua_ui_player_who->inventory[inv].num > 0)
                    {
                        lua_ui_player_who->inventory[inv].num -= 1;
                        RAD_EnableByTag(NULL, script_name.c_str(), false);
                    }
                }
            })
        .addFunction(
            "inventorymax",
            +[](int inv) {
                if (inv < 1 || inv > NUMINV)
                    I_Error("player.inventory: bad inv number: %d\n", inv);

                inv--;

                return lua_ui_player_who->inventory[inv].max;
            })
        .addFunction(
            "rts_enable_tagged",
            +[](std::string name) {
                if (!name.empty())
                    RAD_EnableByTag(NULL, name.c_str(), false);
            })
        .addFunction(
            "counter",
            +[](int cntr) {
                if (cntr < 1 || cntr > NUMCOUNTER)
                    I_Error("player.counter: bad counter number: %d\n", cntr);

                cntr--;

                return lua_ui_player_who->counters[cntr].num;
            })
        .addFunction(
            "counter_max",
            +[](int cntr) {
                if (cntr < 1 || cntr > NUMCOUNTER)
                    I_Error("player.counter: bad counter number: %d\n", cntr);

                cntr--;

                return lua_ui_player_who->counters[cntr].max;
            })
        .addFunction(
            "set_counter",
            +[](int cntr, int amt) {
                if (cntr < 1 || cntr > NUMCOUNTER)
                    I_Error("player.set_counter: bad counter number: %d\n", cntr);

                cntr--;

                if (amt < 0)
                    I_Error("player.set_counter: target amount cannot be negative!\n");

                if (amt > lua_ui_player_who->counters[cntr].max)
                    I_Error("player.set_counter: target amount %d exceeds limit for counter number "
                            "%d\n",
                            amt,
                            cntr);

                lua_ui_player_who->counters[cntr].num = amt;
            })
        .addFunction(
            "query_object",
            +[](int maxdistance, int whatinfo) {
                if (!whatinfo)
                    I_Error("player.query_object: can't parse WhatInfo!\n");

                if (!maxdistance)
                    I_Error("player.query_object: can't parse MaxDistance!\n");

                if (whatinfo < 1 || whatinfo > 5)
                    I_Error("player.query_object: bad whatInfo number: %d\n", whatinfo);

                mobj_t* obj = GetMapTargetAimInfo(
                    lua_ui_player_who->mo, lua_ui_player_who->mo->angle, maxdistance);
                if (!obj)
                {
                    return "";
                }

                std::string temp_string;
                temp_string.clear();

                temp_string = GetQueryInfoFromMobj(obj, whatinfo);

                if (temp_string.empty())
                    return "";
                else
                    return temp_string.c_str();
            })
        .addFunction(
            "query_weapon",
            +[](int maxdistance, int whatinfo, int secattackinfo = 0) {
                if (!maxdistance)
                    I_Error("player.query_weapon: can't parse MaxDistance!\n");

                if (!whatinfo)
                    I_Error("player.query_weapon: can't parse WhatInfo!\n");

                if (whatinfo < 1 || whatinfo > 9)
                    I_Error("player.query_weapon: bad whatInfo number: %d\n", whatinfo);

                if (secattackinfo < 0 || secattackinfo > 1)
                    I_Error("player.query_weapon: bad secAttackInfo number: %d\n", whatinfo);

                mobj_t* obj = GetMapTargetAimInfo(
                    lua_ui_player_who->mo, lua_ui_player_who->mo->angle, maxdistance);
                if (!obj)
                {
                    return "";
                }

                std::string temp_string;
                temp_string.clear();

                if (secattackinfo == 1)
                    temp_string = GetQueryInfoFromWeapon(obj, whatinfo, true);
                else
                    temp_string = GetQueryInfoFromWeapon(obj, whatinfo);

                if (temp_string.empty())
                    return "";
                else
                    return temp_string.c_str();
            })
        .addFunction(
            "is_zoomed", +[]() { return viewiszoomed ? true : false; })
        .addFunction(
            "weapon_state",
            +[](const char* weapon_name, const char* weapon_state) {
                if (lua_ui_player_who->pending_wp >= 0)
                {
                    return false;
                }

                if (lua_ui_player_who->ready_wp < 0)
                {
                    return false;
                }

                weapondef_c* oldWep = weapondefs.Lookup(weapon_name);
                if (!oldWep)
                {
                    I_Error("player.weapon_state: Unknown weapon name '%s'.\n", weapon_name);
                }

                int pw_index;

                // see if player owns this kind of weapon
                for (pw_index = 0; pw_index < MAXWEAPONS; pw_index++)
                {
                    if (!lua_ui_player_who->weapons[pw_index].owned)
                        continue;

                    if (lua_ui_player_who->weapons[pw_index].info == oldWep)
                        break;
                }

                if (pw_index == MAXWEAPONS) // we dont have the weapon
                {
                    return false;
                }

                lua_ui_player_who->ready_wp = (weapon_selection_e) pw_index; // insta-switch to it

                statenum_t state =
                    DDF_StateFindLabel(oldWep->state_grp, weapon_state, true /* quiet */);
                if (state == S_NULL)
                    I_Error("player.weapon_state: frame '%s' in [%s] not found!\n",
                            weapon_state,
                            weapon_name);

                COAL_SetPspriteDeferred(lua_ui_player_who, ps_weapon, state); // refresh the sprite

                return true;
            })
        .addFunction(
            "sector_light",
            +[]() { return lua_ui_player_who->mo->subsector->sector->props.lightlevel; })
        .addFunction(
            "sector_floor_height",
            +[]() { // If no 3D floors, just return the current sector floor height
                if (lua_ui_player_who->mo->subsector->sector->exfloor_used == 0)
                {
                    return lua_ui_player_who->mo->subsector->sector->f_h;
                }
                else
                {
                    // Start from the lowest exfloor and check if the player is standing on it,
                    //  then return the control sector floor height
                    float current_floor = 0;
                    float player_floor_height = lua_ui_player_who->mo->floorz;
                    extrafloor_t* floor_checker =
                        lua_ui_player_who->mo->subsector->sector->bottom_ef;
                    for (extrafloor_t* ef = floor_checker; ef; ef = ef->higher)
                    {
                        if (current_floor > ef->top_h)
                        {
                            return ef->top_h;
                        }

                        if (player_floor_height + 1 > ef->top_h)
                        {
                            current_floor = ef->top_h;
                        }
                    }
                    return current_floor;
                }
            })
        .addFunction(
            "sector_ceiling_height",
            +[]() { // If no 3D floors, just return the current sector ceiling height
                if (lua_ui_player_who->mo->subsector->sector->exfloor_used == 0)
                {
                    return lua_ui_player_who->mo->subsector->sector->c_h;
                }
                else
                {
                    // Start from the lowest exfloor and check if the player is standing on it,
                    //   then return the control sector ceiling height
                    float highest_ceiling = 0;
                    float player_floor_height = lua_ui_player_who->mo->floorz;
                    extrafloor_t* floor_checker =
                        lua_ui_player_who->mo->subsector->sector->bottom_ef;
                    for (extrafloor_t* ef = floor_checker; ef; ef = ef->higher)
                    {
                        if (player_floor_height + 1 > ef->top_h)
                        {
                            highest_ceiling = ef->top_h;
                        }
                        if (highest_ceiling < ef->top_h)
                        {
                            return ef->bottom_h;
                        }
                    }
                    // Fallback if nothing else satisfies these conditions
                    return lua_ui_player_who->mo->subsector->sector->c_h;
                }

            })
        .addFunction(
            "is_outside",
            +[]() {
                // Doesn't account for extrafloors by design. Reasoning is that usually
                //  extrafloors will be platforms, not roofs...
                if (lua_ui_player_who->mo->subsector->sector->ceil.image !=
                    skyflatimage) // is it outdoors?
                    return false;
                else
                    return true;
            })
        .endNamespace()
        .beginNamespace("mapobject")
        .addFunction(
            "query_tagged",
            +[](int whattag, int whatinfo) {
                mobj_t* mo;

                int index = 0;
                std::string temp_value;
                temp_value.clear();

                for (mo = mobjlisthead; mo; mo = mo->next, index++)
                {
                    if (mo->tag == whattag)
                    {
                        temp_value = GetQueryInfoFromMobj(mo, whatinfo);
                        break;
                    }
                }

                if (temp_value.empty())
                    return "";
                else
                    return temp_value.c_str();
            })
        .addFunction(
            "count",
            +[](int thingid) {
                if (!thingid)
                    I_Error("mapobjects.count: can't parse thing id/type!\n");

                mobj_t* mo;

                int index = 0;
                double thingcount = 0;

                for (mo = mobjlisthead; mo; mo = mo->next, index++)
                {
                    if (mo->info->number == thingid && mo->health > 0)
                        thingcount++;
                }

                return thingcount;
            })
        .endNamespace();
}
