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

#include "f_interm.h"
#include "rad_trig.h"

#include <charconv>

#include "flat.h"
#include "s_sound.h"

#include "AlmostEquals.h"

#include "lua_compat.h"

player_t *ui_player_who = NULL;

//------------------------------------------------------------------------
//  PLAYER MODULE
//------------------------------------------------------------------------

// player.num_players()
//
static int PL_num_players(lua_State *L)
{
    lua_pushinteger(L, numplayers);
    return 1;
}

// player.set_who(index)
//
static int PL_set_who(lua_State *L)
{
    int index = luaL_checknumber(L, 1);

    if (index < 0 || index >= numplayers)
        I_Error("player.set_who: bad index value: %d (numplayers=%d)\n", index, numplayers);

    if (index == 0)
    {
        ui_player_who = players[consoleplayer];
        return 0;
    }

    int who = displayplayer;

    for (; index > 1; index--)
    {
        do
        {
            who = (who + 1) % MAXPLAYERS;
        } while (players[who] == NULL);
    }

    ui_player_who = players[who];

    return 0;
}

// player.is_bot()
//
static int PL_is_bot(lua_State *L)
{
    lua_pushboolean(L, (ui_player_who->playerflags & PFL_Bot) ? 1 : 0);
    return 1;
}

// player.get_name()
//
static int PL_get_name(lua_State *L)
{
    lua_pushstring(L, ui_player_who->playername);
    return 1;
}

// player.get_pos()
//
static int PL_get_pos(lua_State *L)
{
    epi::vec3_c v;

    v.x = ui_player_who->mo->x;
    v.y = ui_player_who->mo->y;
    v.z = ui_player_who->mo->z;

    LUA_PushVector3(L, v);
    return 1;
}

// player.get_angle()
//
static int PL_get_angle(lua_State *L)
{
    float value = ANG_2_FLOAT(ui_player_who->mo->angle);

    if (value > 360.0f)
        value -= 360.0f;
    if (value < 0)
        value += 360.0f;

    lua_pushnumber(L, value);
    return 1;
}

// player.get_mlook()
//
static int PL_get_mlook(lua_State *L)
{
    float value = ANG_2_FLOAT(ui_player_who->mo->vertangle);

    if (value > 180.0f)
        value -= 360.0f;

    lua_pushnumber(L, value);
    return 1;
}

// player.health()
//
static int PL_health(lua_State *L)
{
    float h = ui_player_who->health * 100 / ui_player_who->mo->info->spawnhealth;

    if (h < 98)
        h += 0.99f;

    lua_pushinteger(L, floor(h));
    return 1;
}

// player.armor(type)
//
static int PL_armor(lua_State *L)
{
    int kind = (int)luaL_checknumber(L, 1);

    if (kind < 1 || kind > NUMARMOUR)
        I_Error("player.armor: bad armor index: %d\n", kind);

    kind--;
    // lua_pushnumber(L, floor(ui_player_who->armours[kind] + 0.99));

    float a = ui_player_who->armours[kind];
    if (a < 98)
        a += 0.99f;

    lua_pushinteger(L, floor(a));
    return 1;
}

// player.total_armor(type)
//
static int PL_total_armor(lua_State *L)
{
    // lua_pushnumber(L, floor(ui_player_who->totalarmour + 0.99));

    float a = ui_player_who->totalarmour;
    if (a < 98)
        a += 0.99f;

    lua_pushinteger(L, floor(a));
    return 1;
}

// player.frags()
//
static int PL_frags(lua_State *L)
{
    lua_pushinteger(L, ui_player_who->frags);
    return 1;
}

// player.under_water()
//
static int PL_under_water(lua_State *L)
{
    lua_pushboolean(L, ui_player_who->underwater ? 1 : 0);
    return 1;
}

// player.on_ground()
//
static int PL_on_ground(lua_State *L)
{
    // not a 3D floor?
    if (ui_player_who->mo->subsector->sector->exfloor_used == 0)
    {
        // on the edge above water/lava/etc? Handles edge walker case
        if (!AlmostEquals(ui_player_who->mo->floorz, ui_player_who->mo->subsector->sector->f_h) &&
            !ui_player_who->mo->subsector->sector->floor_vertex_slope)
            lua_pushboolean(L, 0);
        else
        {
            // touching the floor? Handles jumping or flying
            if (ui_player_who->mo->z <= ui_player_who->mo->floorz)
                lua_pushboolean(L, 1);
            else
                lua_pushboolean(L, 0);
        }
    }
    else
    {
        if (ui_player_who->mo->z <= ui_player_who->mo->floorz)
            lua_pushboolean(L, 1);
        else
            lua_pushboolean(L, 0);
    }

    return 1;
}

// player.is_swimming()
//
static int PL_is_swimming(lua_State *L)
{
    lua_pushboolean(L, ui_player_who->swimming ? 1 : 0);
    return 1;
}

// player.is_jumping()
//
static int PL_is_jumping(lua_State *L)
{
    lua_pushboolean(L, (ui_player_who->jumpwait > 0) ? 1 : 0);
    return 1;
}

// player.is_crouching()
//
static int PL_is_crouching(lua_State *L)
{
    lua_pushboolean(L, (ui_player_who->mo->extendedflags & EF_CROUCHING) ? 1 : 0);
    return 1;
}

// player.is_attacking()
//
static int PL_is_attacking(lua_State *L)
{
    lua_pushboolean(L, (ui_player_who->attackdown[0] || ui_player_who->attackdown[1]) ? 1 : 0);
    return 1;
}

// player.is_rampaging()
//
static int PL_is_rampaging(lua_State *L)
{
    lua_pushboolean(L, (ui_player_who->attackdown_count >= 70) ? 1 : 0);
    return 1;
}

// player.is_grinning()
//
static int PL_is_grinning(lua_State *L)
{
    lua_pushboolean(L, (ui_player_who->grin_count > 0) ? 1 : 0);
    return 1;
}

// player.is_using()
//
static int PL_is_using(lua_State *L)
{
    lua_pushboolean(L, ui_player_who->usedown ? 1 : 0);
    return 1;
}

// player.is_zoomed()
//
static int PL_is_zoomed(lua_State *L)
{
    lua_pushboolean(L, viewiszoomed ? 1 : 0);
    return 1;
}

// player.is_action1()
//
static int PL_is_action1(lua_State *L)
{
    lua_pushboolean(L, ui_player_who->actiondown[0] ? 1 : 0);
    return 1;
}

// player.is_action2()
//
static int PL_is_action2(lua_State *L)
{
    lua_pushboolean(L, ui_player_who->actiondown[1] ? 1 : 0);
    return 1;
}

// player.move_speed()
//
static int PL_move_speed(lua_State *L)
{
    lua_pushnumber(L, ui_player_who->actual_speed);
    return 1;
}

// player.air_in_lungs()
//
static int PL_air_in_lungs(lua_State *L)
{
    if (ui_player_who->air_in_lungs <= 0)
    {
        lua_pushnumber(L, 0);
        return 1;
    }

    float value = ui_player_who->air_in_lungs * 100.0f / ui_player_who->mo->info->lung_capacity;

    value = CLAMP(0.0f, value, 100.0f);

    lua_pushnumber(L, value);
    return 1;
}

// player.has_key(key)
//
static int PL_has_key(lua_State *L)
{
    int key = (int)luaL_checknumber(L, 1);

    if (key < 1 || key > 16)
        I_Error("player.has_key: bad key number: %d\n", key);

    key--;

    int value = (ui_player_who->cards & (1 << key)) ? 1 : 0;

    lua_pushboolean(L, value);
    return 1;
}

// player.has_power(power)
//
static int PL_has_power(lua_State *L)
{
    int power = (int)luaL_checknumber(L, 1);

    if (power < 1 || power > NUMPOWERS)
        I_Error("player.has_power: bad powerup number: %d\n", power);

    power--;

    int value = (ui_player_who->powers[power] > 0) ? 1 : 0;

    // special check for GOD mode
    if (power == PW_Invulnerable && (ui_player_who->cheats & CF_GODMODE))
        value = 1;

    lua_pushboolean(L, value);
    return 1;
}

// player.power_left(power)
//
static int PL_power_left(lua_State *L)
{
    int power = (int)luaL_checknumber(L, 1);

    if (power < 1 || power > NUMPOWERS)
        I_Error("player.power_left: bad powerup number: %d\n", power);

    power--;

    float value = ui_player_who->powers[power];

    if (value > 0)
        value /= TICRATE;

    lua_pushnumber(L, value);
    return 1;
}

// player.has_weapon_slot(slot)
//
static int PL_has_weapon_slot(lua_State *L)
{
    int slot = (int)luaL_checknumber(L, 1);

    if (slot < 0 || slot > 9)
        I_Error("player.has_weapon_slot: bad slot number: %d\n", slot);

    int value = ui_player_who->avail_weapons[slot] ? 1 : 0;

    lua_pushboolean(L, value);
    return 1;
}

// player.cur_weapon_slot()
//
static int PL_cur_weapon_slot(lua_State *L)
{
    int slot;

    if (ui_player_who->ready_wp < 0)
        slot = -1;
    else
        slot = ui_player_who->weapons[ui_player_who->ready_wp].info->bind_key;

    lua_pushinteger(L, slot);
    return 1;
}

// player.has_weapon(name)
//
static int PL_has_weapon(lua_State *L)
{
    const char *name = luaL_checkstring(L, 1);

    for (int j = 0; j < MAXWEAPONS; j++)
    {
        playerweapon_t *pw = &ui_player_who->weapons[j];

        if (pw->owned && !(pw->flags & PLWEP_Removing) && DDF_CompareName(name, pw->info->name.c_str()) == 0)
        {
            lua_pushboolean(L, 1);
            return 1;
        }
    }

    lua_pushboolean(L, 0);
    return 1;
}

// player.cur_weapon()
//
static int PL_cur_weapon(lua_State *L)
{
    if (ui_player_who->pending_wp >= 0)
    {
        lua_pushstring(L, "change");
        return 1;
    }

    if (ui_player_who->ready_wp < 0)
    {
        lua_pushstring(L, "none");
        return 1;
    }

    weapondef_c *info = ui_player_who->weapons[ui_player_who->ready_wp].info;

    lua_pushstring(L, info->name.c_str());
    return 1;
}

static void LUA_SetPsprite(player_t *p, int position, int stnum, weapondef_c *info = NULL)
{
    pspdef_t *psp = &p->psprites[position];

    if (stnum == S_NULL)
    {
        // object removed itself
        psp->state = psp->next_state = NULL;
        return;
    }

    // state is old? -- Mundo hack for DDF inheritance
    if (info && stnum < info->state_grp.back().first)
    {
        state_t *st = &states[stnum];

        if (st->label)
        {
            statenum_t new_state = DDF_StateFindLabel(info->state_grp, st->label, true /* quiet */);
            if (new_state != S_NULL)
                stnum = new_state;
        }
    }

    state_t *st = &states[stnum];

    // model interpolation stuff
    if (psp->state && (st->flags & SFF_Model) && (psp->state->flags & SFF_Model) &&
        (st->sprite == psp->state->sprite) && st->tics > 1)
    {
        p->weapon_last_frame = psp->state->frame;
    }
    else
        p->weapon_last_frame = -1;

    psp->state      = st;
    psp->tics       = st->tics;
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
void LUA_SetPspriteDeferred(player_t *p, int position, int stnum)
{
    pspdef_t *psp = &p->psprites[position];

    if (stnum == S_NULL || psp->state == NULL)
    {
        LUA_SetPsprite(p, position, stnum);
        return;
    }

    psp->tics       = 0;
    psp->next_state = (states + stnum);
}

// player.weapon_state()
//
static int PL_weapon_state(lua_State *L)
{
    const char *weapon_name  = luaL_checkstring(L, 1);
    const char *weapon_state = luaL_checkstring(L, 2);

    if (ui_player_who->pending_wp >= 0)
    {
        lua_pushboolean(L, 0);
        return 1;
    }

    if (ui_player_who->ready_wp < 0)
    {
        lua_pushboolean(L, 0);
        return 1;
    }

    // weapondef_c *info = ui_player_who->weapons[ui_player_who->ready_wp].info;
    weapondef_c *oldWep = weapondefs.Lookup(weapon_name);
    if (!oldWep)
    {
        I_Error("player.weapon_state: Unknown weapon name '%s'.\n", weapon_name);
    }

    int pw_index;

    // see if player owns this kind of weapon
    for (pw_index = 0; pw_index < MAXWEAPONS; pw_index++)
    {
        if (!ui_player_who->weapons[pw_index].owned)
            continue;

        if (ui_player_who->weapons[pw_index].info == oldWep)
            break;
    }

    if (pw_index == MAXWEAPONS) // we dont have the weapon
    {
        lua_pushboolean(L, 0);
        return 1;
    }

    ui_player_who->ready_wp = (weapon_selection_e)pw_index; // insta-switch to it

    statenum_t state = DDF_StateFindLabel(oldWep->state_grp, weapon_state, true /* quiet */);
    if (state == S_NULL)
        I_Error("player.weapon_state: frame '%s' in [%s] not found!\n", weapon_state, weapon_name);
    // state += 1;

    LUA_SetPspriteDeferred(ui_player_who, ps_weapon, state); // refresh the sprite

    lua_pushboolean(L, 1);
    return 1;
}

// player.ammo(type)
//
static int PL_ammo(lua_State *L)
{
    int ammo = (int)luaL_checknumber(L, 1);

    if (ammo < 1 || ammo > NUMAMMO)
        I_Error("player.ammo: bad ammo number: %d\n", ammo);

    ammo--;

    lua_pushinteger(L, ui_player_who->ammo[ammo].num);
    return 1;
}

// player.ammomax(type)
//
static int PL_ammomax(lua_State *L)
{
    int ammo = (int)luaL_checknumber(L, 1);

    if (ammo < 1 || ammo > NUMAMMO)
        I_Error("player.ammomax: bad ammo number: %d\n", ammo);

    ammo--;

    lua_pushinteger(L, ui_player_who->ammo[ammo].max);
    return 1;
}

// player.inventory(type)
//
static int PL_inventory(lua_State *L)
{
    int inv = (int)luaL_checknumber(L, 1);

    if (inv < 1 || inv > NUMINV)
        I_Error("player.inventory: bad inv number: %d\n", inv);

    inv--;

    lua_pushinteger(L, ui_player_who->inventory[inv].num);
    return 1;
}

// player.inventorymax(type)
//
static int PL_inventorymax(lua_State *L)
{
    int inv = (int)luaL_checknumber(L, 1);

    if (inv < 1 || inv > NUMINV)
        I_Error("player.inventorymax: bad inv number: %d\n", inv);

    inv--;

    lua_pushinteger(L, ui_player_who->inventory[inv].max);
    return 1;
}

// player.counter(type)
//
static int PL_counter(lua_State *L)
{
    int cntr = (int)luaL_checknumber(L, 1);

    if (cntr < 1 || cntr > NUMCOUNTER)
        I_Error("player.counter: bad counter number: %d\n", cntr);

    cntr--;

    lua_pushinteger(L, ui_player_who->counters[cntr].num);
    return 1;
}

// player.counter_max(type)
//
static int PL_counter_max(lua_State *L)
{
    int cntr = (int)luaL_checknumber(L, 1);

    if (cntr < 1 || cntr > NUMCOUNTER)
        I_Error("player.counter_max: bad counter number: %d\n", cntr);

    cntr--;

    lua_pushinteger(L, ui_player_who->counters[cntr].max);
    return 1;
}

// player.set_counter(type, value)
//
static int PL_set_counter(lua_State *L)
{
    int cntr = (int)luaL_checknumber(L, 1);
    int amt  = (int)luaL_checknumber(L, 2);

    if (cntr < 1 || cntr > NUMCOUNTER)
        I_Error("player.set_counter: bad counter number: %d\n", cntr);

    cntr--;

    if (amt < 0)
        I_Error("player.set_counter: target amount cannot be negative!\n");

    if (amt > ui_player_who->counters[cntr].max)
        I_Error("player.set_counter: target amount %d exceeds limit for counter number %d\n", amt, cntr);

    ui_player_who->counters[cntr].num = amt;

    return 0;
}

// player.main_ammo(clip)
//
static int PL_main_ammo(lua_State *L)
{
    int value = 0;

    if (ui_player_who->ready_wp >= 0)
    {
        playerweapon_t *pw = &ui_player_who->weapons[ui_player_who->ready_wp];

        if (pw->info->ammo[0] != AM_NoAmmo)
        {
            if (pw->info->show_clip)
            {
                SYS_ASSERT(pw->info->ammopershot[0] > 0);

                value = pw->clip_size[0] / pw->info->ammopershot[0];
            }
            else
            {
                value = ui_player_who->ammo[pw->info->ammo[0]].num;

                if (pw->info->clip_size[0] > 0)
                    value += pw->clip_size[0];
            }
        }
    }

    lua_pushinteger(L, value);
    return 1;
}

// player.ammo_type(ATK)
//
static int PL_ammo_type(lua_State *L)
{
    int ATK = (int)luaL_checknumber(L, 1);

    if (ATK < 1 || ATK > 2)
        I_Error("player.ammo_type: bad attack number: %d\n", ATK);

    ATK--;

    int value = 0;

    if (ui_player_who->ready_wp >= 0)
    {
        playerweapon_t *pw = &ui_player_who->weapons[ui_player_who->ready_wp];

        value = 1 + (int)pw->info->ammo[ATK];
    }

    lua_pushinteger(L, value);
    return 1;
}

// player.ammo_pershot(ATK)
//
static int PL_ammo_pershot(lua_State *L)
{
    int ATK = (int)luaL_checknumber(L, 1);

    if (ATK < 1 || ATK > 2)
        I_Error("player.ammo_pershot: bad attack number: %d\n", ATK);

    ATK--;

    int value = 0;

    if (ui_player_who->ready_wp >= 0)
    {
        playerweapon_t *pw = &ui_player_who->weapons[ui_player_who->ready_wp];

        value = pw->info->ammopershot[ATK];
    }

    lua_pushinteger(L, value);
    return 1;
}

// player.clip_ammo(ATK)
//
static int PL_clip_ammo(lua_State *L)
{
    int ATK = (int)luaL_checknumber(L, 1);

    if (ATK < 1 || ATK > 2)
        I_Error("player.clip_ammo: bad attack number: %d\n", ATK);

    ATK--;

    int value = 0;

    if (ui_player_who->ready_wp >= 0)
    {
        playerweapon_t *pw = &ui_player_who->weapons[ui_player_who->ready_wp];

        value = pw->clip_size[ATK];
    }

    lua_pushinteger(L, value);
    return 1;
}

// player.clip_size(ATK)
//
static int PL_clip_size(lua_State *L)
{
    int ATK = (int)luaL_checknumber(L, 1);

    if (ATK < 1 || ATK > 2)
        I_Error("player.clip_size: bad attack number: %d\n", ATK);

    ATK--;

    int value = 0;

    if (ui_player_who->ready_wp >= 0)
    {
        playerweapon_t *pw = &ui_player_who->weapons[ui_player_who->ready_wp];

        value = pw->info->clip_size[ATK];
    }

    lua_pushinteger(L, value);
    return 1;
}

// player.clip_is_shared()
//
static int PL_clip_is_shared(lua_State *L)
{
    int value = 0;

    if (ui_player_who->ready_wp >= 0)
    {
        playerweapon_t *pw = &ui_player_who->weapons[ui_player_who->ready_wp];

        if (pw->info->shared_clip)
            value = 1;
    }

    lua_pushboolean(L, value);
    return 1;
}

// player.hurt_by()
//
static int PL_hurt_by(lua_State *L)
{
    if (ui_player_who->damagecount <= 0)
    {
        lua_pushstring(L, "");
        return 1;
    }

    // getting hurt because of your own damn stupidity
    if (ui_player_who->attacker == ui_player_who->mo)
        lua_pushstring(L, "self");
    else if (ui_player_who->attacker && (ui_player_who->attacker->side & ui_player_who->mo->side))
        lua_pushstring(L, "friend");
    else if (ui_player_who->attacker)
        lua_pushstring(L, "enemy");
    else
        lua_pushstring(L, "other");

    return 1;
}

// player.hurt_mon()
//
static int PL_hurt_mon(lua_State *L)
{
    if (ui_player_who->damagecount > 0 && ui_player_who->attacker && ui_player_who->attacker != ui_player_who->mo)
    {
        lua_pushstring(L, ui_player_who->attacker->info->name.c_str());
        return 1;
    }

    lua_pushstring(L, "");
    return 1;
}

// player.hurt_pain()
//
static int PL_hurt_pain(lua_State *L)
{
    lua_pushinteger(L, ui_player_who->damage_pain);
    return 1;
}

// player.hurt_dir()
//
static int PL_hurt_dir(lua_State *L)
{
    int dir = 0;

    if (ui_player_who->attacker && ui_player_who->attacker != ui_player_who->mo)
    {
        mobj_t *badguy = ui_player_who->attacker;
        mobj_t *pmo    = ui_player_who->mo;

        angle_t diff = R_PointToAngle(pmo->x, pmo->y, badguy->x, badguy->y) - pmo->angle;

        if (diff >= ANG45 && diff <= ANG135)
        {
            dir = -1;
        }
        else if (diff >= ANG225 && diff <= ANG315)
        {
            dir = +1;
        }
    }

    lua_pushinteger(L, dir);
    return 1;
}

// player.hurt_angle()
//
static int PL_hurt_angle(lua_State *L)
{
    float value = 0;

    if (ui_player_who->attacker && ui_player_who->attacker != ui_player_who->mo)
    {
        mobj_t *badguy = ui_player_who->attacker;
        mobj_t *pmo    = ui_player_who->mo;

        angle_t real_a = R_PointToAngle(pmo->x, pmo->y, badguy->x, badguy->y);

        value = ANG_2_FLOAT(real_a);

        if (value > 360.0f)
            value -= 360.0f;

        if (value < 0)
            value += 360.0f;
    }

    lua_pushinteger(L, value);
    return 1;
}

// player.kills()
// Lobo: November 2021
static int PL_kills(lua_State *L)
{
    lua_pushinteger(L, ui_player_who->killcount);
    return 1;
}

// player.secrets()
// Lobo: November 2021
static int PL_secrets(lua_State *L)
{
    lua_pushinteger(L, ui_player_who->secretcount);
    return 1;
}

// player.items()
// Lobo: November 2021
static int PL_items(lua_State *L)
{
    lua_pushinteger(L, ui_player_who->itemcount);
    return 1;
}

// player.map_enemies()
// Lobo: November 2021
static int PL_map_enemies(lua_State *L)
{
    lua_pushinteger(L, wi_stats.kills);
    return 1;
}

// player.map_secrets()
// Lobo: November 2021
static int PL_map_secrets(lua_State *L)
{
    lua_pushinteger(L, wi_stats.secret);
    return 1;
}

// player.map_items()
// Lobo: November 2021
static int PL_map_items(lua_State *L)
{
    lua_pushinteger(L, wi_stats.items);
    return 1;
}

// player.floor_flat()
// Lobo: November 2021
static int PL_floor_flat(lua_State *L)
{
    // If no 3D floors, just return the flat
    if (ui_player_who->mo->subsector->sector->exfloor_used == 0)
    {
        lua_pushstring(L, ui_player_who->mo->subsector->sector->floor.image->name.c_str());
    }
    else
    {
        // Start from the lowest exfloor and check if the player is standing on it, then return the control sector's
        // flat
        float         player_floor_height = ui_player_who->mo->floorz;
        extrafloor_t *floor_checker       = ui_player_who->mo->subsector->sector->bottom_ef;
        for (extrafloor_t *ef = floor_checker; ef; ef = ef->higher)
        {
            if (player_floor_height + 1 > ef->top_h)
            {
                lua_pushstring(L, ef->top->image->name.c_str());
                return 1;
            }
        }
        // Fallback if nothing else satisfies these conditions
        lua_pushstring(L, ui_player_who->mo->subsector->sector->floor.image->name.c_str());
    }

    return 1;
}

// player.sector_tag()
// Lobo: November 2021
static int PL_sector_tag(lua_State *L)
{
    lua_pushinteger(L, ui_player_who->mo->subsector->sector->tag);
    return 1;
}

// player.play_footstep(flat name)
// Dasho: January 2022
// Now uses the new DDFFLAT construct
static int PL_play_footstep(lua_State *L)
{
    const char *flat = luaL_checkstring(L, 1);
    if (!flat)
        I_Error("player.play_footstep: No flat name given!\n");

    flatdef_c *current_flatdef = flatdefs.Find(flat);

    if (!current_flatdef)
    {
        lua_pushboolean(L, 0);
        return 1;
    }

    if (!current_flatdef->footstep)
    {
        lua_pushboolean(L, 0);
        return 1;
    }
    else
    {
        // Probably need to add check to see if the sfx is valid - Dasho
        S_StartFX(current_flatdef->footstep);
        lua_pushboolean(L, 1);
    }

    return 1;
}

// player.use_inventory(type)
//
static int PL_use_inventory(lua_State *L)
{
    int         inv         = (int)luaL_checknumber(L, 1);
    std::string script_name = "INVENTORY";

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
        if (ui_player_who->inventory[inv].num > 0)
        {
            ui_player_who->inventory[inv].num -= 1;
            RAD_EnableByTag(NULL, script_name.c_str(), false);
        }
    }

    return 0;
}

// player.rts_enable_tagged(tag)
//
static int PL_rts_enable_tagged(lua_State *L)
{
    std::string name = luaL_checkstring(L, 1);

    if (!name.empty())
        RAD_EnableByTag(NULL, name.c_str(), false);

    return 0;
}

// AuxStringReplaceAll("Our_String", std::string("_"), std::string(" "));
//
std::string AuxStringReplaceAll(std::string str, const std::string &from, const std::string &to)
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
std::string GetMobjBenefits(mobj_t *obj, bool KillBenefits = false)
{
    std::string temp_string;
    temp_string.clear();
    benefit_t *list;
    int        temp_num = 0;

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
            temp_string += "AMMO" + std::to_string((int)list->sub.type + 1);
            temp_string += "=" + std::to_string((int)list->amount);
            break;

        case BENEFIT_Health: // only benefit without a sub.type so just give it 01
            temp_string += "HEALTH01=" + std::to_string((int)list->amount);
            break;

        case BENEFIT_Armour:
            temp_string += "ARMOUR" + std::to_string((int)list->sub.type + 1);
            temp_string += "=" + std::to_string((int)list->amount);
            break;

        case BENEFIT_Inventory:
            temp_string += "INVENTORY";
            if ((list->sub.type + 1) < 10)
                temp_string += "0";
            temp_string += std::to_string((int)list->sub.type + 1);
            temp_string += "=" + std::to_string((int)list->amount);
            break;

        case BENEFIT_Counter:
            temp_string += "COUNTER";
            if ((list->sub.type + 1) < 10)
                temp_string += "0";
            temp_string += std::to_string((int)list->sub.type + 1);
            temp_string += "=" + std::to_string((int)list->amount);
            break;

        case BENEFIT_Key:
            temp_string += "KEY";
            temp_num = log2((int)list->sub.type);
            temp_num++;
            temp_string += std::to_string(temp_num);
            break;

        case BENEFIT_Powerup:
            temp_string += "POWERUP" + std::to_string((int)list->sub.type + 1);
            break;

        default:
            break;
        }
    }
    return temp_string;
}

// GetQueryInfoFromMobj(mobj, whatinfo)
//
std::string GetQueryInfoFromMobj(mobj_t *obj, int whatinfo)
{
    int         temp_num = 0;
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
            temp_num    = obj->health;
            temp_string = std::to_string(temp_num);
        }
        break;

    case 3: // spawn health
        if (obj)
        {
            temp_num    = obj->info->spawnhealth;
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

// GetQueryInfoFromWeapon(mobj, whatinfo, [secattackinfo])
//
std::string GetQueryInfoFromWeapon(mobj_t *obj, int whatinfo, bool secattackinfo = false)
{
    int         temp_num = 0;
    std::string temp_string;
    temp_string.clear();

    if (!obj->info->pickup_benefits)
        return "";
    if (!obj->info->pickup_benefits->sub.weap)
        return "";
    if (obj->info->pickup_benefits->type != BENEFIT_Weapon)
        return "";

    weapondef_c *objWep = obj->info->pickup_benefits->sub.weap;
    if (!objWep)
        return "";

    int attacknum = 0; // default to primary attack
    if (secattackinfo)
        attacknum = 1;

    atkdef_c *objAtck = objWep->attack[attacknum];
    if (!objAtck && whatinfo > 2)
        return ""; // no attack to get info about (only should happen with secondary attacks)

    const damage_c *damtype;

    float temp_num2;

    switch (whatinfo)
    {
    case 1: // name
        temp_string = objWep->name;
        temp_string = AuxStringReplaceAll(temp_string, std::string("_"), std::string(" "));
        break;

    case 2: // ZOOM_FACTOR
        temp_num2   = 90.0f / objWep->zoom_fov;
        temp_string = std::to_string(temp_num2);
        break;

    case 3: // AMMOTYPE
        temp_num    = (objWep->ammo[attacknum]) + 1;
        temp_string = std::to_string(temp_num);
        break;

    case 4: // AMMOPERSHOT
        temp_num    = objWep->ammopershot[attacknum];
        temp_string = std::to_string(temp_num);
        break;

    case 5: // CLIPSIZE
        temp_num    = objWep->clip_size[attacknum];
        temp_string = std::to_string(temp_num);
        break;

    case 6: // DAMAGE Nominal
        damtype     = &objAtck->damage;
        temp_num    = damtype->nominal;
        temp_string = std::to_string(temp_num);
        break;

    case 7: // DAMAGE Max
        damtype     = &objAtck->damage;
        temp_num    = damtype->linear_max;
        temp_string = std::to_string(temp_num);
        break;

    case 8: // Range
        temp_num    = objAtck->range;
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

// player.query_object(maxdistance,whatinfo)
//
static int PL_query_object(lua_State *L)
{
    int maxdistance = (int)luaL_checknumber(L, 1);
    int whatinfo    = (int)luaL_checknumber(L, 2);

    if (whatinfo < 1 || whatinfo > 5)
        I_Error("player.query_object: bad whatInfo number: %d\n", whatinfo);

    mobj_t *obj = GetMapTargetAimInfo(ui_player_who->mo, ui_player_who->mo->angle, maxdistance);
    if (!obj)
    {
        lua_pushstring(L, "");
        return 1;
    }

    std::string temp_string;
    temp_string.clear();

    temp_string = GetQueryInfoFromMobj(obj, whatinfo);

    if (temp_string.empty())
        lua_pushstring(L, "");
    else
        lua_pushstring(L, temp_string.c_str());

    return 1;
}

// mapobject.query_tagged(thing tag, whatinfo)
//
static int MO_query_tagged(lua_State *L)
{

    int whattag  = (int)luaL_checknumber(L, 1);
    int whatinfo = (int)luaL_checknumber(L, 2);

    mobj_t *mo;

    int         index = 0;
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
        lua_pushstring(L, "");
    else
        lua_pushstring(L, temp_value.c_str());

    return 1;
}

// mapobject.count(thing type/id)
//
static int MO_count(lua_State *L)
{
    int thingid = (int)luaL_checknumber(L, 1);

    mobj_t *mo;

    int    index      = 0;
    double thingcount = 0;

    for (mo = mobjlisthead; mo; mo = mo->next, index++)
    {
        if (mo->info->number == thingid && mo->health > 0)
            thingcount++;
    }

    lua_pushinteger(L, thingcount);
}

// player.query_weapon(maxdistance,whatinfo,[SecAttack])
//
static int PL_query_weapon(lua_State *L)
{

    int maxdistance   = (int)luaL_checknumber(L, 1);
    int whatinfo      = (int)luaL_checknumber(L, 1);
    int secattackinfo = (int)luaL_optnumber(L, 1, 0);

    if (whatinfo < 1 || whatinfo > 9)
        I_Error("player.query_weapon: bad whatInfo number: %d\n", whatinfo);

    if (secattackinfo < 0 || secattackinfo > 1)
        I_Error("player.query_weapon: bad secAttackInfo number: %d\n", whatinfo);

    mobj_t *obj = GetMapTargetAimInfo(ui_player_who->mo, ui_player_who->mo->angle, maxdistance);
    if (!obj)
    {
        lua_pushstring(L, "");
        return 1;
    }

    std::string temp_string;
    temp_string.clear();

    if (secattackinfo == 1)
        temp_string = GetQueryInfoFromWeapon(obj, whatinfo, true);
    else
        temp_string = GetQueryInfoFromWeapon(obj, whatinfo);

    if (temp_string.empty())
        lua_pushstring(L, "");
    else
        lua_pushstring(L, temp_string.c_str());

    return 1;
}

// player.sector_light()
// Lobo: May 2023
static int PL_sector_light(lua_State *L)
{
    lua_pushnumber(L, ui_player_who->mo->subsector->sector->props.lightlevel);
    return 1;
}

// player.sector_floor_height()
// Lobo: May 2023
static int PL_sector_floor_height(lua_State *L)
{
    // If no 3D floors, just return the current sector floor height
    if (ui_player_who->mo->subsector->sector->exfloor_used == 0)
    {
        lua_pushnumber(L, ui_player_who->mo->subsector->sector->f_h);
    }
    else
    {
        // Start from the lowest exfloor and check if the player is standing on it,
        //  then return the control sector floor height
        float         CurrentFloor        = 0;
        float         player_floor_height = ui_player_who->mo->floorz;
        extrafloor_t *floor_checker       = ui_player_who->mo->subsector->sector->bottom_ef;
        for (extrafloor_t *ef = floor_checker; ef; ef = ef->higher)
        {
            if (CurrentFloor > ef->top_h)
            {
                lua_pushnumber(L, ef->top_h);
                return 1;
            }

            if (player_floor_height + 1 > ef->top_h)
            {
                CurrentFloor = ef->top_h;
            }
        }
        lua_pushnumber(L, CurrentFloor);
    }

    return 1;
}

// player.sector_ceiling_height()
// Lobo: May 2023
static int PL_sector_ceiling_height(lua_State *L)
{
    // If no 3D floors, just return the current sector ceiling height
    if (ui_player_who->mo->subsector->sector->exfloor_used == 0)
    {
        lua_pushnumber(L, ui_player_who->mo->subsector->sector->c_h);
    }
    else
    {
        // Start from the lowest exfloor and check if the player is standing on it,
        //   then return the control sector ceiling height
        float         HighestCeiling      = 0;
        float         player_floor_height = ui_player_who->mo->floorz;
        extrafloor_t *floor_checker       = ui_player_who->mo->subsector->sector->bottom_ef;
        for (extrafloor_t *ef = floor_checker; ef; ef = ef->higher)
        {
            if (player_floor_height + 1 > ef->top_h)
            {
                HighestCeiling = ef->top_h;
            }
            if (HighestCeiling < ef->top_h)
            {
                lua_pushnumber(L, ef->bottom_h);
                return 1;
            }
        }
        // Fallback if nothing else satisfies these conditions
        lua_pushnumber(L, ui_player_who->mo->subsector->sector->c_h);
    }

    return 1;
}

// player.is_outside()
// Lobo: May 2023
static int PL_is_outside(lua_State *L)
{
    // Doesn't account for extrafloors by design. Reasoning is that usually
    //  extrafloors will be platforms, not roofs...
    if (ui_player_who->mo->subsector->sector->ceil.image != skyflatimage) // is it outdoors?
        lua_pushboolean(L, 0);
    else
        lua_pushboolean(L, 1);

    return 1;
}

static const luaL_Reg playerlib[] = {{"num_players", PL_num_players},
                                     {"set_who", PL_set_who},
                                     {"is_bot", PL_is_bot},
                                     {"get_name", PL_get_name},
                                     {"get_pos", PL_get_pos},
                                     {"get_angle", PL_get_angle},
                                     {"get_mlook", PL_get_mlook},

                                     {"health", PL_health},
                                     {"armor", PL_armor},
                                     {"total_armor", PL_total_armor},
                                     {"ammo", PL_ammo},
                                     {"ammomax", PL_ammomax},
                                     {"frags", PL_frags},

                                     {"is_swimming", PL_is_swimming},
                                     {"is_jumping", PL_is_jumping},
                                     {"is_crouching", PL_is_crouching},
                                     {"is_using", PL_is_using},
                                     {"is_action1", PL_is_action1},
                                     {"is_action2", PL_is_action2},
                                     {"is_attacking", PL_is_attacking},
                                     {"is_rampaging", PL_is_rampaging},
                                     {"is_grinning", PL_is_grinning},

                                     {"under_water", PL_under_water},
                                     {"on_ground", PL_on_ground},
                                     {"move_speed", PL_move_speed},
                                     {"air_in_lungs", PL_air_in_lungs},

                                     {"has_key", PL_has_key},
                                     {"has_power", PL_has_power},
                                     {"power_left", PL_power_left},
                                     {"has_weapon", PL_has_weapon},
                                     {"has_weapon_slot", PL_has_weapon_slot},
                                     {"cur_weapon", PL_cur_weapon},
                                     {"cur_weapon_slot", PL_cur_weapon_slot},

                                     {"main_ammo", PL_main_ammo},
                                     {"ammo_type", PL_ammo_type},
                                     {"ammo_pershot", PL_ammo_pershot},
                                     {"clip_ammo", PL_clip_ammo},
                                     {"clip_size", PL_clip_size},
                                     {"clip_is_shared", PL_clip_is_shared},

                                     {"hurt_by", PL_hurt_by},
                                     {"hurt_mon", PL_hurt_mon},
                                     {"hurt_pain", PL_hurt_pain},
                                     {"hurt_dir", PL_hurt_dir},
                                     {"hurt_angle", PL_hurt_angle},

                                     {"kills", PL_kills},
                                     {"secrets", PL_secrets},
                                     {"items", PL_items},
                                     {"map_enemies", PL_map_enemies},
                                     {"map_secrets", PL_map_secrets},
                                     {"map_items", PL_map_items},
                                     {"floor_flat", PL_floor_flat},
                                     {"sector_tag", PL_sector_tag},

                                     {"play_footstep", PL_play_footstep},

                                     {"use_inventory", PL_use_inventory},
                                     {"inventory", PL_inventory},
                                     {"inventorymax", PL_inventorymax},

                                     {"rts_enable_tagged", PL_rts_enable_tagged},

                                     {"counter", PL_counter},
                                     {"counter_max", PL_counter_max},
                                     {"set_counter", PL_set_counter},

                                     {"query_object", PL_query_object},
                                     {"query_weapon", PL_query_weapon},
                                     {"is_zoomed", PL_is_zoomed},
                                     {"weapon_state", PL_weapon_state},

                                     {"sector_light", PL_sector_light},
                                     {"sector_floor_height", PL_sector_floor_height},
                                     {"sector_ceiling_height", PL_sector_ceiling_height},
                                     {"is_outside", PL_is_outside},
                                     {NULL, NULL}};

static int luaopen_player(lua_State *L)
{
    luaL_newlib(L, playerlib);
    return 1;
}

static const luaL_Reg mapobjectlib[] = {{"query_tagged", MO_query_tagged}, {"count", MO_count}, {NULL, NULL}};

static int luaopen_mapobject(lua_State *L)
{
    luaL_newlib(L, playerlib);
    return 1;
}

void LUA_RegisterPlayerLibrary(lua_State *L)
{
    luaL_requiref(L, "player", luaopen_player, 1);
    lua_pop(L, 1);
    luaL_requiref(L, "mapobject", luaopen_mapobject, 1);
    lua_pop(L, 1);
}
