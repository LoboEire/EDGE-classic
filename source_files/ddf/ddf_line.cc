//----------------------------------------------------------------------------
//  EDGE Data Definition File Code (Linedefs)
//----------------------------------------------------------------------------
//
//  Copyright (c) 1999-2024 The EDGE Team.
//
//  This program is free software; you can redistribute it and/or
//  modify it under the terms of the GNU General Public License
//  as published by the Free Software Foundation; either version 3
//  of the License, or (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//----------------------------------------------------------------------------
//
// Line Definitions Setup and Parser Code
//
// -KM- 1998/09/01 Written.
// -ACB- 1998/09/06 Beautification: cleaned up so I can read it :).
// -KM- 1998/10/29 New types of linedefs added: colourmap, sound, friction,
// gravity
//                  auto, singlesided, music, lumpcheck
//                  Removed sector movement to ddf_main.c, so can be accesed by
//                  ddf_sect.c
//
// -ACB- 2001/02/04 DDFGetSecHeightReference moved to p_plane.c
//

#include "ddf_line.h"

#include <limits.h>
#include <string.h>

#include "AlmostEquals.h"
#include "ddf_local.h"
#include "epi_str_util.h"

// -KM- 1999/01/29 Improved scrolling.
// Scrolling
enum ScrollDirections
{
    kScrollDirectionNone       = 0,
    kScrollDirectionVertical   = 1,
    kScrollDirectionUp         = 2,
    kScrollDirectionHorizontal = 4,
    kScrollDirectionLeft       = 8
};

LineTypeContainer linetypes; // <-- User-defined

static LineType *default_linetype;

static void DDFLineGetTrigType(const char *info, void *storage);
static void DDFLineGetActivators(const char *info, void *storage);
static void DDFLineGetSecurity(const char *info, void *storage);
static void DDFLineGetScroller(const char *info, void *storage);
static void DDFLineGetScrollPart(const char *info, void *storage);
static void DDFLineGetExtraFloor(const char *info, void *storage);
static void DDFLineGetEFControl(const char *info, void *storage);
static void DDFLineGetTeleportSpecial(const char *info, void *storage);
static void DDFLineGetRadTrig(const char *info, void *storage);
static void DDFLineGetSpecialFlags(const char *info, void *storage);
static void DDFLineGetSlideType(const char *info, void *storage);
static void DDFLineGetLineEffect(const char *info, void *storage);
static void DDFLineGetScrollType(const char *info, void *storage);
static void DDFLineGetSectorEffect(const char *info, void *storage);
static void DDFLineGetPortalEffect(const char *info, void *storage);
static void DDFLineGetSlopeType(const char *info, void *storage);

static void DDFLineMakeCrush(const char *info);

static PlaneMoverDefinition dummy_floor;

const DDFCommandList floor_commands[] = {DDF_FIELD("TYPE", dummy_floor, type_, DDFSectGetMType),
                                         DDF_FIELD("SPEED_UP", dummy_floor, speed_up_, DDFMainGetFloat),
                                         DDF_FIELD("SPEED_DOWN", dummy_floor, speed_down_, DDFMainGetFloat),
                                         DDF_FIELD("DEST_REF", dummy_floor, destref_, DDFSectGetDestRef),
                                         DDF_FIELD("DEST_OFFSET", dummy_floor, dest_, DDFMainGetFloat),
                                         DDF_FIELD("OTHER_REF", dummy_floor, otherref_, DDFSectGetDestRef),
                                         DDF_FIELD("OTHER_OFFSET", dummy_floor, other_, DDFMainGetFloat),
                                         DDF_FIELD("CRUSH_DAMAGE", dummy_floor, crush_damage_, DDFMainGetNumeric),
                                         DDF_FIELD("TEXTURE", dummy_floor, tex_, DDFMainGetLumpName),
                                         DDF_FIELD("PAUSE_TIME", dummy_floor, wait_, DDFMainGetTime),
                                         DDF_FIELD("WAIT_TIME", dummy_floor, prewait_, DDFMainGetTime),
                                         DDF_FIELD("SFX_START", dummy_floor, sfxstart_, DDFMainLookupSound),
                                         DDF_FIELD("SFX_UP", dummy_floor, sfxup_, DDFMainLookupSound),
                                         DDF_FIELD("SFX_DOWN", dummy_floor, sfxdown_, DDFMainLookupSound),
                                         DDF_FIELD("SFX_STOP", dummy_floor, sfxstop_, DDFMainLookupSound),
                                         DDF_FIELD("SCROLL_ANGLE", dummy_floor, scroll_angle_, DDFMainGetAngle),
                                         DDF_FIELD("SCROLL_SPEED", dummy_floor, scroll_speed_, DDFMainGetFloat),
                                         DDF_FIELD("IGNORE_TEXTURE", dummy_floor, ignore_texture_, DDFMainGetBoolean),

                                         {nullptr, nullptr, 0, nullptr}};

static LadderDefinition dummy_ladder;

const DDFCommandList ladder_commands[] = {DDF_FIELD("HEIGHT", dummy_ladder, height_, DDFMainGetFloat),
                                          {nullptr, nullptr, 0, nullptr}};

static SlidingDoor dummy_slider;

const DDFCommandList slider_commands[] = {DDF_FIELD("TYPE", dummy_slider, type_, DDFLineGetSlideType),
                                          DDF_FIELD("SPEED", dummy_slider, speed_, DDFMainGetFloat),
                                          DDF_FIELD("PAUSE_TIME", dummy_slider, wait_, DDFMainGetTime),
                                          DDF_FIELD("SEE_THROUGH", dummy_slider, see_through_, DDFMainGetBoolean),
                                          DDF_FIELD("DISTANCE", dummy_slider, distance_, DDFMainGetPercent),
                                          DDF_FIELD("SFX_START", dummy_slider, sfx_start_, DDFMainLookupSound),
                                          DDF_FIELD("SFX_OPEN", dummy_slider, sfx_open_, DDFMainLookupSound),
                                          DDF_FIELD("SFX_CLOSE", dummy_slider, sfx_close_, DDFMainLookupSound),
                                          DDF_FIELD("SFX_STOP", dummy_slider, sfx_stop_, DDFMainLookupSound),

                                          {nullptr, nullptr, 0, nullptr}};

static LineType *dynamic_line;

// these bits logically belong with buffer_line:
static float            scrolling_speed;
static ScrollDirections scrolling_dir;

static LineType dummy_line;

static const DDFCommandList linedef_commands[] = {
    // sub-commands
    DDF_SUB_LIST("FLOOR", dummy_line, f_, floor_commands),
    DDF_SUB_LIST("CEILING", dummy_line, c_, floor_commands),
    DDF_SUB_LIST("SLIDER", dummy_line, s_, slider_commands),
    DDF_SUB_LIST("LADDER", dummy_line, ladder_, ladder_commands),

    DDF_FIELD("NEWTRIGGER", dummy_line, newtrignum_, DDFMainGetNumeric),
    DDF_FIELD("ACTIVATORS", dummy_line, obj_, DDFLineGetActivators),
    DDF_FIELD("TYPE", dummy_line, type_, DDFLineGetTrigType),
    DDF_FIELD("KEYS", dummy_line, keys_, DDFLineGetSecurity),
    DDF_FIELD("FAILED_MESSAGE", dummy_line, failedmessage_, DDFMainGetString),
    DDF_FIELD("FAILED_SFX", dummy_line, failed_sfx_, DDFMainLookupSound),
    DDF_FIELD("COUNT", dummy_line, count_, DDFMainGetNumeric),

    DDF_FIELD("DONUT", dummy_line, d_.dodonut_, DDFMainGetBoolean),
    DDF_FIELD("DONUT_IN_SFX", dummy_line, d_.d_sfxin_, DDFMainLookupSound),
    DDF_FIELD("DONUT_IN_SFXSTOP", dummy_line, d_.d_sfxinstop_, DDFMainLookupSound),
    DDF_FIELD("DONUT_OUT_SFX", dummy_line, d_.d_sfxout_, DDFMainLookupSound),
    DDF_FIELD("DONUT_OUT_SFXSTOP", dummy_line, d_.d_sfxoutstop_, DDFMainLookupSound),

    DDF_FIELD("TELEPORT", dummy_line, t_.teleport_, DDFMainGetBoolean),
    DDF_FIELD("TELEPORT_DELAY", dummy_line, t_.delay_, DDFMainGetTime),
    DDF_FIELD("TELEIN_EFFECTOBJ", dummy_line, t_.inspawnobj_ref_, DDFMainGetString),
    DDF_FIELD("TELEOUT_EFFECTOBJ", dummy_line, t_.outspawnobj_ref_, DDFMainGetString),
    DDF_FIELD("TELEPORT_SPECIAL", dummy_line, t_.special_, DDFLineGetTeleportSpecial),

    DDF_FIELD("LIGHT_TYPE", dummy_line, l_.type_, DDFSectGetLighttype),
    DDF_FIELD("LIGHT_LEVEL", dummy_line, l_.level_, DDFMainGetNumeric),
    DDF_FIELD("LIGHT_DARK_TIME", dummy_line, l_.darktime_, DDFMainGetTime),
    DDF_FIELD("LIGHT_BRIGHT_TIME", dummy_line, l_.brighttime_, DDFMainGetTime),
    DDF_FIELD("LIGHT_CHANCE", dummy_line, l_.chance_, DDFMainGetPercent),
    DDF_FIELD("LIGHT_SYNC", dummy_line, l_.sync_, DDFMainGetTime),
    DDF_FIELD("LIGHT_STEP", dummy_line, l_.step_, DDFMainGetNumeric),
    DDF_FIELD("EXIT", dummy_line, e_exit_, DDFSectGetExit),
    DDF_FIELD("HUB_EXIT", dummy_line, hub_exit_, DDFMainGetNumeric),

    DDF_FIELD("SCROLL_XSPEED", dummy_line, s_xspeed_, DDFMainGetFloat),
    DDF_FIELD("SCROLL_YSPEED", dummy_line, s_yspeed_, DDFMainGetFloat),
    DDF_FIELD("SCROLL_PARTS", dummy_line, scroll_parts_, DDFLineGetScrollPart),
    DDF_FIELD("USE_COLOURMAP", dummy_line, use_colourmap_, DDFMainGetColourmap),
    DDF_FIELD("GRAVITY", dummy_line, gravity_, DDFMainGetFloat),
    DDF_FIELD("FRICTION", dummy_line, friction_, DDFMainGetFloat),
    DDF_FIELD("VISCOSITY", dummy_line, viscosity_, DDFMainGetFloat),
    DDF_FIELD("DRAG", dummy_line, drag_, DDFMainGetFloat),
    DDF_FIELD("AMBIENT_SOUND", dummy_line, ambient_sfx_, DDFMainLookupSound),
    DDF_FIELD("ACTIVATE_SOUND", dummy_line, activate_sfx_, DDFMainLookupSound),
    DDF_FIELD("MUSIC", dummy_line, music_, DDFMainGetNumeric),
    DDF_FIELD("AUTO", dummy_line, autoline_, DDFMainGetBoolean),
    DDF_FIELD("SINGLESIDED", dummy_line, singlesided_, DDFMainGetBoolean),
    DDF_FIELD("EXTRAFLOOR_TYPE", dummy_line, ef_.type_, DDFLineGetExtraFloor),
    DDF_FIELD("EXTRAFLOOR_CONTROL", dummy_line, ef_.control_, DDFLineGetEFControl),
    DDF_FIELD("TRANSLUCENCY", dummy_line, translucency_, DDFMainGetPercent),
    DDF_FIELD("WHEN_APPEAR", dummy_line, appear_, DDFMainGetWhenAppear),
    DDF_FIELD("SPECIAL", dummy_line, special_flags_, DDFLineGetSpecialFlags),
    DDF_FIELD("RADIUS_TRIGGER", dummy_line, trigger_effect_, DDFLineGetRadTrig),
    DDF_FIELD("LINE_EFFECT", dummy_line, line_effect_, DDFLineGetLineEffect),
    DDF_FIELD("SCROLL_TYPE", dummy_line, scroll_type_, DDFLineGetScrollType),
    DDF_FIELD("LINE_PARTS", dummy_line, line_parts_, DDFLineGetScrollPart),
    DDF_FIELD("SECTOR_EFFECT", dummy_line, sector_effect_, DDFLineGetSectorEffect),
    DDF_FIELD("PORTAL_TYPE", dummy_line, portal_effect_, DDFLineGetPortalEffect),
    DDF_FIELD("SLOPE_TYPE", dummy_line, slope_type_, DDFLineGetSlopeType),
    DDF_FIELD("COLOUR", dummy_line, fx_color_, DDFMainGetRGB),

    // -AJA- backwards compatibility cruft...
    DDF_FIELD("EXTRAFLOOR_TRANSLUCENCY", dummy_line, translucency_, DDFMainGetPercent),

    // Lobo: 2022
    DDF_FIELD("EFFECT_OBJECT", dummy_line, effectobject_ref_, DDFMainGetString),
    DDF_FIELD("GLASS", dummy_line, glass_, DDFMainGetBoolean),
    DDF_FIELD("BROKEN_TEXTURE", dummy_line, brokentex_, DDFMainGetLumpName),

    {nullptr, nullptr, 0, nullptr}};

struct ScrollKludge
{
    const char      *s;
    ScrollDirections dir;
};

static ScrollKludge s_scroll[] = {{"NONE", kScrollDirectionNone},
                                  {"UP", (ScrollDirections)(kScrollDirectionVertical | kScrollDirectionUp)},
                                  {"DOWN", kScrollDirectionVertical},
                                  {"LEFT", (ScrollDirections)(kScrollDirectionHorizontal | kScrollDirectionLeft)},
                                  {"RIGHT", kScrollDirectionHorizontal},
                                  {nullptr, kScrollDirectionNone}};

static struct // FIXME: APPLIES TO NEXT 3 TABLES !
{
    const char *s;
    int         n;
}

// FIXME: use keytype_names (in ddf_mobj.c)
s_keys[] = {{"NONE", kDoorKeyNone},

            {"BLUE_CARD", kDoorKeyBlueCard},
            {"YELLOW_CARD", kDoorKeyYellowCard},
            {"RED_CARD", kDoorKeyRedCard},
            {"BLUE_SKULL", kDoorKeyBlueSkull},
            {"YELLOW_SKULL", kDoorKeyYellowSkull},
            {"RED_SKULL", kDoorKeyRedSkull},
            {"GREEN_CARD", kDoorKeyGreenCard},
            {"GREEN_SKULL", kDoorKeyGreenSkull},

            {"GOLD_KEY", kDoorKeyGoldKey},
            {"SILVER_KEY", kDoorKeySilverKey},
            {"BRASS_KEY", kDoorKeyBrassKey},
            {"COPPER_KEY", kDoorKeyCopperKey},
            {"STEEL_KEY", kDoorKeySteelKey},
            {"WOODEN_KEY", kDoorKeyWoodenKey},
            {"FIRE_KEY", kDoorKeyFireKey},
            {"WATER_KEY", kDoorKeyWaterKey},

            // backwards compatibility
            {"REQUIRES_ALL", kDoorKeyStrictlyAllKeys | kDoorKeyBlueCard | kDoorKeyYellowCard | kDoorKeyRedCard |
                                 kDoorKeyBlueSkull | kDoorKeyYellowSkull | kDoorKeyRedSkull}},

    s_trigger[] = {{"WALK", kLineTriggerWalkable},
                   {"PUSH", kLineTriggerPushable},
                   {"SHOOT", kLineTriggerShootable},
                   {"MANUAL", kLineTriggerManual}},

    s_activators[] = {{"PLAYER", kTriggerActivatorPlayer},
                      {"MONSTER", kTriggerActivatorMonster},
                      {"OTHER", kTriggerActivatorOther},
                      {"NOBOT", kTriggerActivatorNoBot},

                      // obsolete stuff
                      {"MISSILE", 0}};

//
//  DDF PARSE ROUTINES
//

static void LinedefStartEntry(const char *name, bool extend)
{
    int number = HMM_MAX(0, atoi(name));

    if (number == 0)
        DDFError("Bad linetype number in lines.ddf: %s\n", name);

    scrolling_dir   = kScrollDirectionNone;
    scrolling_speed = 1.0f;

    dynamic_line = linetypes.Lookup(number);

    if (extend)
    {
        if (!dynamic_line)
            DDFError("Unknown linetype to extend: %s\n", name);
        return;
    }

    // replaces an existing entry?
    if (dynamic_line)
    {
        dynamic_line->Default();
        return;
    }

    // not found, create a new one
    dynamic_line          = new LineType;
    dynamic_line->number_ = number;

    linetypes.push_back(dynamic_line);
}

static void LinedefDoTemplate(const char *contents)
{
    int number = HMM_MAX(0, atoi(contents));
    if (number == 0)
        DDFError("Bad linetype number for template: %s\n", contents);

    LineType *other = linetypes.Lookup(number);

    if (!other || other == dynamic_line)
        DDFError("Unknown linetype template: '%s'\n", contents);

    dynamic_line->CopyDetail(*other);
}

static void LinedefParseField(const char *field, const char *contents, int index, bool is_last)
{
#if (DDF_DEBUG)
    LogDebug("LINEDEF_PARSE: %s = %s;\n", field, contents);
#endif
    EPI_UNUSED(index);
    EPI_UNUSED(is_last);
    if (DDFCompareName(field, "TEMPLATE") == 0)
    {
        LinedefDoTemplate(contents);
        return;
    }

    // ignored for backwards compatibility
    if (DDFCompareName(field, "SECSPECIAL") == 0)
        return;

    // -AJA- backwards compatibility cruft...
    if (DDFCompareName(field, "CRUSH") == 0)
    {
        DDFLineMakeCrush(contents);
        return;
    }
    else if (DDFCompareName(field, "SCROLL") == 0)
    {
        DDFLineGetScroller(contents, &scrolling_dir);
        return;
    }
    else if (DDFCompareName(field, "SCROLLING_SPEED") == 0)
    {
        scrolling_speed = atof(contents);
        return;
    }

    if (DDFMainParseField(linedef_commands, field, contents, (uint8_t *)dynamic_line))
        return; // OK

    DDFWarnError("Unknown lines.ddf command: %s\n", field);
}

static void LinedefFinishEntry(void)
{
    // -KM- 1999/01/29 Convert old style scroller to new.
    if (scrolling_dir & kScrollDirectionVertical)
    {
        if (scrolling_dir & kScrollDirectionUp)
            dynamic_line->s_yspeed_ = scrolling_speed;
        else
            dynamic_line->s_yspeed_ = -scrolling_speed;
    }

    if (scrolling_dir & kScrollDirectionHorizontal)
    {
        if (scrolling_dir & kScrollDirectionLeft)
            dynamic_line->s_xspeed_ = scrolling_speed;
        else
            dynamic_line->s_xspeed_ = -scrolling_speed;
    }

    // backwards compat: COUNT=0 means no limit on triggering
    if (dynamic_line->count_ == 0)
        dynamic_line->count_ = -1;

    if (dynamic_line->hub_exit_ > 0)
        dynamic_line->e_exit_ = kExitTypeHub;

    // check stuff...

    if (dynamic_line->ef_.type_ != kExtraFloorTypeNone)
    {
        // AUTO is no longer needed for extrafloors
        dynamic_line->autoline_ = false;

        if ((dynamic_line->ef_.type_ & kExtraFloorTypeFlooder) && (dynamic_line->ef_.type_ & kExtraFloorTypeNoShade))
        {
            DDFWarnError("FLOODER and NOSHADE tags cannot be used together.\n");
            dynamic_line->ef_.type_ = (ExtraFloorType)(dynamic_line->ef_.type_ & ~kExtraFloorTypeFlooder);
        }

        if (!(dynamic_line->ef_.type_ & kExtraFloorTypePresent))
        {
            DDFWarnError("Extrafloor type missing THIN, THICK or LIQUID.\n");
            dynamic_line->ef_.type_ = kExtraFloorTypeNone;
        }
    }

    if (!AlmostEquals(dynamic_line->friction_, kFloatUnused) && dynamic_line->friction_ < 0.05f)
    {
        DDFWarnError("Friction value too low (%1.2f), it would prevent "
                     "all movement.\n",
                     dynamic_line->friction_);
        dynamic_line->friction_ = 0.05f;
    }

    if (!AlmostEquals(dynamic_line->viscosity_, kFloatUnused) && dynamic_line->viscosity_ > 0.95f)
    {
        DDFWarnError("Viscosity value too high (%1.2f), it would prevent "
                     "all movement.\n",
                     dynamic_line->viscosity_);
        dynamic_line->viscosity_ = 0.95f;
    }

    // TODO: check more stuff...
}

static void LinedefClearAll(void)
{
    // 100% safe to delete all the linetypes
    linetypes.Reset();
}

void DDFReadLines(const std::string &data)
{
    DDFReadInfo lines;

    lines.tag      = "LINES";
    lines.lumpname = "DDFLINE";

    lines.start_entry  = LinedefStartEntry;
    lines.parse_field  = LinedefParseField;
    lines.finish_entry = LinedefFinishEntry;
    lines.clear_all    = LinedefClearAll;

    DDFMainReadFile(&lines, data);
}

void DDFLinedefInit(void)
{
    linetypes.Reset();

    default_linetype          = new LineType();
    default_linetype->number_ = 0;
}

void DDFLinedefCleanUp(void)
{
    for (auto l : linetypes)
    {
        cur_ddf_entryname = epi::StringFormat("[%d]  (lines.ddf)", l->number_);

        l->t_.inspawnobj_ = l->t_.inspawnobj_ref_ != "" ? mobjtypes.Lookup(l->t_.inspawnobj_ref_.c_str()) : nullptr;

        l->t_.outspawnobj_ = l->t_.outspawnobj_ref_ != "" ? mobjtypes.Lookup(l->t_.outspawnobj_ref_.c_str()) : nullptr;

        // Lobo: 2021
        l->effectobject_ = l->effectobject_ref_ != "" ? mobjtypes.Lookup(l->effectobject_ref_.c_str()) : nullptr;

        cur_ddf_entryname.clear();
    }

    linetypes.shrink_to_fit();
}

//
// DDFLineGetScroller
//
// Check for scroll types
//
void DDFLineGetScroller(const char *info, void *storage)
{
    EPI_UNUSED(storage);
    for (int i = 0; s_scroll[i].s; i++)
    {
        if (DDFCompareName(info, s_scroll[i].s) == 0)
        {
            scrolling_dir = (ScrollDirections)(scrolling_dir | s_scroll[i].dir);
            return;
        }
    }
    DDFWarnError("Unknown scroll direction %s\n", info);
}

//
// DDFLineGetSecurity
//
// Get Red/Blue/Yellow
//
void DDFLineGetSecurity(const char *info, void *storage)
{
    DoorKeyType *var = (DoorKeyType *)storage;

    bool required = false;

    if (info[0] == '+')
    {
        required = true;
        info++;
    }
    else if (*var & kDoorKeyStrictlyAllKeys)
    {
        // -AJA- when there is at least one required key, then the
        // non-required keys don't have any effect.
        return;
    }

    for (int i = sizeof(s_keys) / sizeof(s_keys[0]); i--;)
    {
        if (DDFCompareName(info, s_keys[i].s) == 0)
        {
            *var = (DoorKeyType)(*var | s_keys[i].n);

            if (required)
                *var = (DoorKeyType)(*var | kDoorKeyStrictlyAllKeys);

            return;
        }
    }

    DDFWarnError("Unknown key type %s\n", info);
}

//
// DDFLineGetTrigType
//
// Check for walk/push/shoot
//
void DDFLineGetTrigType(const char *info, void *storage)
{
    LineTrigger *var = (LineTrigger *)storage;

    for (int i = sizeof(s_trigger) / sizeof(s_trigger[0]); i--;)
    {
        if (DDFCompareName(info, s_trigger[i].s) == 0)
        {
            *var = (LineTrigger)s_trigger[i].n;
            return;
        }
    }

    DDFWarnError("Unknown Trigger type %s\n", info);
}

//
// DDFLineGetActivators
//
// Get player/monsters/missiles
//
void DDFLineGetActivators(const char *info, void *storage)
{
    TriggerActivator *var = (TriggerActivator *)storage;

    for (int i = sizeof(s_activators) / sizeof(s_activators[0]); i--;)
    {
        if (DDFCompareName(info, s_activators[i].s) == 0)
        {
            *var = (TriggerActivator)(*var | s_activators[i].n);
            return;
        }
    }

    DDFWarnError("Unknown Activator type %s\n", info);
}

static DDFSpecialFlags extrafloor_types[] = {
    // definers:
    {"THIN", kExtraFloorThinDefaults, 0},
    {"THICK", kExtraFloorThickDefaults, 0},
    {"LIQUID", kExtraFloorLiquidDefaults, 0},

    // modifiers:
    {"SEE_THROUGH", kExtraFloorTypeSeeThrough, 0},
    {"WATER", kExtraFloorTypeWater, 0},
    {"SHADE", kExtraFloorTypeNoShade, 1},
    {"FLOODER", kExtraFloorTypeFlooder, 0},
    {"SIDE_UPPER", kExtraFloorTypeSideUpper, 0},
    {"SIDE_LOWER", kExtraFloorTypeSideLower, 0},
    {"SIDE_MIDY", kExtraFloorTypeSideMidY, 0},
    {"BOOMTEX", kExtraFloorTypeBoomTex, 0},

    // backwards compatibility...
    {"FALL_THROUGH", kExtraFloorTypeLiquid, 0},
    {"SHOOT_THROUGH", 0, 0},
    {nullptr, 0, 0}};

//
// DDFLineGetExtraFloor
//
// Gets the extra floor type(s).
//
// -AJA- 1999/06/21: written.
// -AJA- 2000/03/27: updated for simpler system.
//
void DDFLineGetExtraFloor(const char *info, void *storage)
{
    ExtraFloorType *var = (ExtraFloorType *)storage;

    if (DDFCompareName(info, "NONE") == 0)
    {
        *var = kExtraFloorTypeNone;
        return;
    }

    int flag_value;

    switch (DDFMainCheckSpecialFlag(info, extrafloor_types, &flag_value, true, false))
    {
    case kDDFCheckFlagPositive:
        *var = (ExtraFloorType)(*var | flag_value);
        break;

    case kDDFCheckFlagNegative:
        *var = (ExtraFloorType)(*var & ~flag_value);
        break;

    case kDDFCheckFlagUser:
    case kDDFCheckFlagUnknown:
        DDFWarnError("Unknown Extrafloor Type: %s\n", info);
        break;
    }
}

static DDFSpecialFlags ef_control_types[] = {
    {"NONE", kExtraFloorControlNone, 0}, {"REMOVE", kExtraFloorControlRemove, 0}, {nullptr, 0, 0}};

//
// DDFLineGetEFControl
//
void DDFLineGetEFControl(const char *info, void *storage)
{
    ExtraFloorControl *var = (ExtraFloorControl *)storage;

    int flag_value;

    switch (DDFMainCheckSpecialFlag(info, ef_control_types, &flag_value, false, false))
    {
    case kDDFCheckFlagPositive:
    case kDDFCheckFlagNegative:
        *var = (ExtraFloorControl)flag_value;
        break;

    case kDDFCheckFlagUser:
    case kDDFCheckFlagUnknown:
        DDFWarnError("Unknown CONTROL_EXTRAFLOOR tag: %s", info);
        break;
    }
}

static constexpr int kTeleportSpecialAllSame =
    ((TeleportSpecial)(kTeleportSpecialRelative | kTeleportSpecialSameHeight | kTeleportSpecialSameSpeed |
                       kTeleportSpecialSameOffset));

static constexpr int kTeleportSpecialPreserve =
    ((TeleportSpecial)(kTeleportSpecialSameAbsDir | kTeleportSpecialSameHeight | kTeleportSpecialSameSpeed));

static DDFSpecialFlags teleport_specials[] = {{"RELATIVE", kTeleportSpecialRelative, 0},
                                              {"SAME_HEIGHT", kTeleportSpecialSameHeight, 0},
                                              {"SAME_SPEED", kTeleportSpecialSameSpeed, 0},
                                              {"SAME_OFFSET", kTeleportSpecialSameOffset, 0},
                                              {"ALL_SAME", kTeleportSpecialAllSame, 0},

                                              {"LINE", kTeleportSpecialLine, 0},
                                              {"FLIPPED", kTeleportSpecialFlipped, 0},
                                              {"SILENT", kTeleportSpecialSilent, 0},

                                              // these modes are deprecated (kept for B.C.)
                                              {"SAME_DIR", kTeleportSpecialSameAbsDir, 0},
                                              {"ROTATE", kTeleportSpecialRotate, 0},
                                              {"PRESERVE", kTeleportSpecialPreserve, 0},

                                              {nullptr, 0, 0}};

//
// DDFLineGetTeleportSpecial
//
// Gets the teleporter special flags.
//
// -AJA- 1999/07/12: written.
//
void DDFLineGetTeleportSpecial(const char *info, void *storage)
{
    TeleportSpecial *var = (TeleportSpecial *)storage;

    int flag_value;

    switch (DDFMainCheckSpecialFlag(info, teleport_specials, &flag_value, true, false))
    {
    case kDDFCheckFlagPositive:
        *var = (TeleportSpecial)(*var | flag_value);
        break;

    case kDDFCheckFlagNegative:
        *var = (TeleportSpecial)(*var & ~flag_value);
        break;

    case kDDFCheckFlagUser:
    case kDDFCheckFlagUnknown:
        DDFWarnError("DDFLineGetTeleportSpecial: Unknown Special: %s\n", info);
        break;
    }
}

static DDFSpecialFlags scrollpart_specials[] = {{"RIGHT_UPPER", kScrollingPartRightUpper, 0},
                                                {"RIGHT_MIDDLE", kScrollingPartRightMiddle, 0},
                                                {"RIGHT_LOWER", kScrollingPartRightLower, 0},
                                                {"RIGHT", kScrollingPartRight, 0},
                                                {"LEFT_UPPER", kScrollingPartLeftUpper, 0},
                                                {"LEFT_MIDDLE", kScrollingPartLeftMiddle, 0},
                                                {"LEFT_LOWER", kScrollingPartLeftLower, 0},
                                                {"LEFT", kScrollingPartLeft, 0},
                                                {"LEFT_REVERSE_X", kScrollingPartLeftRevX, 0},
                                                {"LEFT_REVERSE_Y", kScrollingPartLeftRevY, 0},
                                                {nullptr, 0, 0}};

//
// DDFLineGetScrollPart
//
// Gets the scroll part flags.
//
// -AJA- 1999/07/12: written.
//
void DDFLineGetScrollPart(const char *info, void *storage)
{
    int            flag_value;
    ScrollingPart *dest = (ScrollingPart *)storage;

    if (DDFCompareName(info, "NONE") == 0)
    {
        (*dest) = kScrollingPartNone;
        return;
    }

    switch (DDFMainCheckSpecialFlag(info, scrollpart_specials, &flag_value, true, false))
    {
    case kDDFCheckFlagPositive:
        (*dest) = (ScrollingPart)((*dest) | flag_value);
        break;

    case kDDFCheckFlagNegative:
        (*dest) = (ScrollingPart)((*dest) & ~flag_value);
        break;

    case kDDFCheckFlagUser:
    case kDDFCheckFlagUnknown:
        DDFWarnError("DDFLineGetScrollPart: Unknown Part: %s", info);
        break;
    }
}

//----------------------------------------------------------------------------

static DDFSpecialFlags line_specials[] = {{"MUST_REACH", kLineSpecialMustReach, 0},
                                          {"SWITCH_SEPARATE", kLineSpecialSwitchSeparate, 0},
                                          {"BACK_SECTOR", kLineSpecialBackSector, 0},
                                          {nullptr, 0, 0}};

//
// DDFLineGetSpecialFlags
//
// Gets the line special flags.
//
void DDFLineGetSpecialFlags(const char *info, void *storage)
{
    LineSpecial *var = (LineSpecial *)storage;

    int flag_value;

    switch (DDFMainCheckSpecialFlag(info, line_specials, &flag_value, true, false))
    {
    case kDDFCheckFlagPositive:
        *var = (LineSpecial)(*var | flag_value);
        break;

    case kDDFCheckFlagNegative:
        *var = (LineSpecial)(*var & ~flag_value);
        break;

    case kDDFCheckFlagUser:
    case kDDFCheckFlagUnknown:
        DDFWarnError("Unknown line special: %s", info);
        break;
    }
}

//
// DDFLineGetRadTrig
//
// Gets the line's radius trigger effect.
//
static void DDFLineGetRadTrig(const char *info, void *storage)
{
    int *trigger_effect = (int *)storage;

    if (DDFCompareName(info, "ENABLE_TAGGED") == 0)
    {
        *trigger_effect = +1;
        return;
    }
    if (DDFCompareName(info, "DISABLE_TAGGED") == 0)
    {
        *trigger_effect = -1;
        return;
    }

    DDFWarnError("DDFLineGetRadTrig: Unknown effect: %s\n", info);
}

static const DDFSpecialFlags slidingdoor_names[] = {{"NONE", kSlidingDoorTypeNone, 0},
                                                    {"LEFT", kSlidingDoorTypeLeft, 0},
                                                    {"RIGHT", kSlidingDoorTypeRight, 0},
                                                    {"CENTER", kSlidingDoorTypeCenter, 0},
                                                    {"CENTRE", kSlidingDoorTypeCenter, 0}, // synonym
                                                    {nullptr, 0, 0}};

//
// DDFLineGetSlideType
//
static void DDFLineGetSlideType(const char *info, void *storage)
{
    if (kDDFCheckFlagPositive != DDFMainCheckSpecialFlag(info, slidingdoor_names, (int *)storage, false, false))
    {
        DDFWarnError("DDFLineGetSlideType: Unknown slider: %s\n", info);
    }
}

static DDFSpecialFlags line_effect_names[] = {{"TRANSLUCENT", kLineEffectTypeTranslucency, 0},
                                              {"VECTOR_SCROLL", kLineEffectTypeVectorScroll, 0},
                                              {"OFFSET_SCROLL", kLineEffectTypeOffsetScroll, 0},

                                              {"SCALE_TEX", kLineEffectTypeScale, 0},
                                              {"SKEW_TEX", kLineEffectTypeSkew, 0},
                                              {"LIGHT_WALL", kLineEffectTypeLightWall, 0},

                                              {"UNBLOCK_THINGS", kLineEffectTypeUnblockThings, 0},
                                              {"BLOCK_SHOTS", kLineEffectTypeBlockShots, 0},
                                              {"BLOCK_SIGHT", kLineEffectTypeBlockSight, 0},
                                              {"SKY_TRANSFER", kLineEffectTypeSkyTransfer, 0}, // Lobo 2022
                                              {"TAGGED_OFFSET_SCROLL", kLineEffectTypeTaggedOffsetScroll, 0},   // MBF21
                                              {"BLOCK_LAND_MONSTERS", kLineEffectTypeBlockGroundedMonsters, 0}, // MBF21
                                              {"BLOCK_PLAYERS", kLineEffectTypeBlockPlayers, 0},                // MBF21
                                              {"STRETCH_TEX_WIDTH", kLineEffectTypeStretchWidth, 0},   // Lobo 2023
                                              {"STRETCH_TEX_HEIGHT", kLineEffectTypeStretchHeight, 0}, // Lobo 2023
                                              {nullptr, 0, 0}};

//
// Gets the line effect flags.
//
static void DDFLineGetLineEffect(const char *info, void *storage)
{
    LineEffectType *var = (LineEffectType *)storage;

    int flag_value;

    if (DDFCompareName(info, "NONE") == 0)
    {
        *var = kLineEffectTypeNONE;
        return;
    }

    switch (DDFMainCheckSpecialFlag(info, line_effect_names, &flag_value, true, false))
    {
    case kDDFCheckFlagPositive:
        *var = (LineEffectType)(*var | flag_value);
        break;

    case kDDFCheckFlagNegative:
        *var = (LineEffectType)(*var & ~flag_value);
        break;

    case kDDFCheckFlagUser:
    case kDDFCheckFlagUnknown:
        DDFWarnError("Unknown line effect type: %s", info);
        break;
    }
}

static DDFSpecialFlags scroll_type_names[] = {
    {"DISPLACE", BoomScrollerTypeDisplace, 0}, {"ACCEL", BoomScrollerTypeAccel, 0}, {nullptr, 0, 0}};

//
// Gets the scroll type flags.
//
static void DDFLineGetScrollType(const char *info, void *storage)
{
    BoomScrollerType *var = (BoomScrollerType *)storage;

    int flag_value;

    if (DDFCompareName(info, "NONE") == 0)
    {
        *var = BoomScrollerTypeNone;
        return;
    }

    switch (DDFMainCheckSpecialFlag(info, scroll_type_names, &flag_value, true, false))
    {
    case kDDFCheckFlagPositive:
        *var = (BoomScrollerType)(*var | flag_value);
        break;

    case kDDFCheckFlagNegative:
        *var = (BoomScrollerType)(*var & ~flag_value);
        break;

    case kDDFCheckFlagUser:
    case kDDFCheckFlagUnknown:
        DDFWarnError("Unknown scroll type: %s", info);
        break;
    }
}

static DDFSpecialFlags sector_effect_names[] = {
    {"LIGHT_FLOOR", kSectorEffectTypeLightFloor, 0},   {"LIGHT_CEILING", kSectorEffectTypeLightCeiling, 0},
    {"SCROLL_FLOOR", kSectorEffectTypeScrollFloor, 0}, {"SCROLL_CEILING", kSectorEffectTypeScrollCeiling, 0},

    {"PUSH_THINGS", kSectorEffectTypePushThings, 0},   {"SET_FRICTION", kSectorEffectTypeSetFriction, 0},
    {"WIND_FORCE", kSectorEffectTypeWindForce, 0},     {"CURRENT_FORCE", kSectorEffectTypeCurrentForce, 0},
    {"POINT_FORCE", kSectorEffectTypePointForce, 0},

    {"RESET_FLOOR", kSectorEffectTypeResetFloor, 0},   {"RESET_CEILING", kSectorEffectTypeResetCeiling, 0},
    {"ALIGN_FLOOR", kSectorEffectTypeAlignFloor, 0},   {"ALIGN_CEILING", kSectorEffectTypeAlignCeiling, 0},
    {"SCALE_FLOOR", kSectorEffectTypeScaleFloor, 0},   {"SCALE_CEILING", kSectorEffectTypeScaleCeiling, 0},

    {"BOOM_HEIGHTS", kSectorEffectTypeBoomHeights, 0}, {nullptr, 0, 0}};

//
// Gets the sector effect flags.
//
static void DDFLineGetSectorEffect(const char *info, void *storage)
{
    SectorEffectType *var = (SectorEffectType *)storage;

    int flag_value;

    if (DDFCompareName(info, "NONE") == 0)
    {
        *var = kSectorEffectTypeNone;
        return;
    }

    switch (DDFMainCheckSpecialFlag(info, sector_effect_names, &flag_value, true, false))
    {
    case kDDFCheckFlagPositive:
        *var = (SectorEffectType)(*var | flag_value);
        break;

    case kDDFCheckFlagNegative:
        *var = (SectorEffectType)(*var & ~flag_value);
        break;

    case kDDFCheckFlagUser:
    case kDDFCheckFlagUnknown:
        DDFWarnError("Unknown sector effect type: %s", info);
        break;
    }
}

static DDFSpecialFlags portal_effect_names[] = {{"STANDARD", kPortalEffectTypeStandard, 0},
                                                {"MIRROR", kPortalEffectTypeMirror, 0},
                                                {"CAMERA", kPortalEffectTypeCamera, 0},

                                                {nullptr, 0, 0}};

//
// Gets the portal effect flags.
//
static void DDFLineGetPortalEffect(const char *info, void *storage)
{
    PortalEffectType *var = (PortalEffectType *)storage;

    int flag_value;

    if (DDFCompareName(info, "NONE") == 0)
    {
        *var = kPortalEffectTypeNone;
        return;
    }

    switch (DDFMainCheckSpecialFlag(info, portal_effect_names, &flag_value, true, false))
    {
    case kDDFCheckFlagPositive:
        *var = (PortalEffectType)(*var | flag_value);
        break;

    case kDDFCheckFlagNegative:
        *var = (PortalEffectType)(*var & ~flag_value);
        break;

    case kDDFCheckFlagUser:
    case kDDFCheckFlagUnknown:
        DDFWarnError("Unknown portal type: %s", info);
        break;
    }
}

static DDFSpecialFlags slope_type_names[] = {{"FAKE_FLOOR", kSlopeTypeDetailFloor, 0},
                                             {"FAKE_CEILING", kSlopeTypeDetailCeiling, 0},

                                             {nullptr, 0, 0}};

static void DDFLineGetSlopeType(const char *info, void *storage)
{
    SlopeType *var = (SlopeType *)storage;

    int flag_value;

    if (DDFCompareName(info, "NONE") == 0)
    {
        *var = kSlopeTypeNONE;
        return;
    }

    switch (DDFMainCheckSpecialFlag(info, slope_type_names, &flag_value, true, false))
    {
    case kDDFCheckFlagPositive:
        *var = (SlopeType)(*var | flag_value);
        break;

    case kDDFCheckFlagNegative:
        *var = (SlopeType)(*var & ~flag_value);
        break;

    case kDDFCheckFlagUser:
    case kDDFCheckFlagUnknown:
        DDFWarnError("Unknown slope type: %s", info);
        break;
    }
}

static void DDFLineMakeCrush(const char *info)
{
    EPI_UNUSED(info);
    dynamic_line->f_.crush_damage_ = 10;
    dynamic_line->c_.crush_damage_ = 10;
}

//----------------------------------------------------------------------------

// --> Donut definition class

//
// donutdef_c Constructor
//
DonutDefinition::DonutDefinition()
{
}

//
// donutdef_c Copy constructor
//
DonutDefinition::DonutDefinition(const DonutDefinition &rhs)
{
    Copy(rhs);
}

//
// donutdef_c Destructor
//
DonutDefinition::~DonutDefinition()
{
}

//
// donutdef_c::Copy()
//
void DonutDefinition::Copy(const DonutDefinition &src)
{
    dodonut_ = src.dodonut_;

    // FIXME! Strip out the d_ since we're not trying to
    // to differentiate them now?
    d_sfxin_      = src.d_sfxin_;
    d_sfxinstop_  = src.d_sfxinstop_;
    d_sfxout_     = src.d_sfxout_;
    d_sfxoutstop_ = src.d_sfxoutstop_;
}

//
// donutdef_c::Default()
//
void DonutDefinition::Default()
{
    dodonut_      = false;
    d_sfxin_      = nullptr;
    d_sfxinstop_  = nullptr;
    d_sfxout_     = nullptr;
    d_sfxoutstop_ = nullptr;
}

//
// donutdef_c assignment operator
//
DonutDefinition &DonutDefinition::operator=(const DonutDefinition &rhs)
{
    if (&rhs != this)
        Copy(rhs);

    return *this;
}

// --> Extrafloor definition class

//
// extrafloordef_c Constructor
//
ExtraFloorDefinition::ExtraFloorDefinition()
{
}

//
// extrafloordef_c Copy constructor
//
ExtraFloorDefinition::ExtraFloorDefinition(const ExtraFloorDefinition &rhs)
{
    Copy(rhs);
}

//
// extrafloordef_c Destructor
//
ExtraFloorDefinition::~ExtraFloorDefinition()
{
}

//
// extrafloordef_c::Copy()
//
void ExtraFloorDefinition::Copy(const ExtraFloorDefinition &src)
{
    control_ = src.control_;
    type_    = src.type_;
}

//
// extrafloordef_c::Default()
//
void ExtraFloorDefinition::Default()
{
    control_ = kExtraFloorControlNone;
    type_    = kExtraFloorTypeNone;
}

//
// extrafloordef_c assignment operator
//
ExtraFloorDefinition &ExtraFloorDefinition::operator=(const ExtraFloorDefinition &rhs)
{
    if (&rhs != this)
        Copy(rhs);

    return *this;
}

// --> Ladder definition class

//
// ladderdef_c Constructor
//
LadderDefinition::LadderDefinition()
{
}

//
// ladderdef_c Copy constructor
//
LadderDefinition::LadderDefinition(const LadderDefinition &rhs)
{
    Copy(rhs);
}

//
// ladderdef_c Destructor
//
LadderDefinition::~LadderDefinition()
{
}

//
// ladderdef_c::Copy()
//
void LadderDefinition::Copy(const LadderDefinition &src)
{
    height_ = src.height_;
}

//
// ladderdef_c::Default()
//
void LadderDefinition::Default()
{
    height_ = 0.0f;
}

//
// ladderdef_c assignment operator
//
LadderDefinition &LadderDefinition::operator=(const LadderDefinition &rhs)
{
    if (&rhs != this)
        Copy(rhs);

    return *this;
}

// --> Light effect definition class

//
// lightdef_c Constructor
//
LightSpecialDefinition::LightSpecialDefinition()
{
}

//
// lightdef_c Copy constructor
//
LightSpecialDefinition::LightSpecialDefinition(const LightSpecialDefinition &rhs)
{
    Copy(rhs);
}

//
// lightdef_c Destructor
//
LightSpecialDefinition::~LightSpecialDefinition()
{
}

//
// lightdef_c::Copy()
//
void LightSpecialDefinition::Copy(const LightSpecialDefinition &src)
{
    type_       = src.type_;
    level_      = src.level_;
    chance_     = src.chance_;
    darktime_   = src.darktime_;
    brighttime_ = src.brighttime_;
    sync_       = src.sync_;
    step_       = src.step_;
}

//
// lightdef_c::Default()
//
void LightSpecialDefinition::Default()
{
    type_       = kLightSpecialTypeNone;
    level_      = 64;
    chance_     = 0.5f;
    darktime_   = 0;
    brighttime_ = 0;
    sync_       = 0;
    step_       = 8;
}

//
// lightdef_c assignment operator
//
LightSpecialDefinition &LightSpecialDefinition::operator=(const LightSpecialDefinition &rhs)
{
    if (&rhs != this)
        Copy(rhs);

    return *this;
}

// --> Moving plane definition class

//
// movplanedef_c Constructor
//
PlaneMoverDefinition::PlaneMoverDefinition()
{
}

//
// movplanedef_c Copy constructor
//
PlaneMoverDefinition::PlaneMoverDefinition(const PlaneMoverDefinition &rhs)
{
    Copy(rhs);
}

//
// movplanedef_c Destructor
//
PlaneMoverDefinition::~PlaneMoverDefinition()
{
}

//
// movplanedef_c::Copy()
//
void PlaneMoverDefinition::Copy(const PlaneMoverDefinition &src)
{
    type_           = src.type_;
    is_ceiling_     = src.is_ceiling_;
    speed_up_       = src.speed_up_;
    speed_down_     = src.speed_down_;
    destref_        = src.destref_;
    dest_           = src.dest_;
    otherref_       = src.otherref_;
    other_          = src.other_;
    crush_damage_   = src.crush_damage_;
    tex_            = src.tex_;
    wait_           = src.wait_;
    prewait_        = src.prewait_;
    sfxstart_       = src.sfxstart_;
    sfxup_          = src.sfxup_;
    sfxdown_        = src.sfxdown_;
    sfxstop_        = src.sfxstop_;
    scroll_angle_   = src.scroll_angle_;
    scroll_speed_   = src.scroll_speed_;
    ignore_texture_ = src.ignore_texture_;
}

//
// movplanedef_c::Default()
//
void PlaneMoverDefinition::Default(PlaneMoverDefinition::PlaneMoverDefault def)
{
    type_ = kPlaneMoverUndefined;

    if (def == kPlaneMoverDefaultCeilingLine || def == kPlaneMoverDefaultCeilingSect)
        is_ceiling_ = true;
    else
        is_ceiling_ = false;

    switch (def)
    {
    case kPlaneMoverDefaultCeilingLine:
    case kPlaneMoverDefaultFloorLine: {
        speed_up_   = -1;
        speed_down_ = -1;
        break;
    }

    case kPlaneMoverDefaultDonutFloor: {
        speed_up_   = kFloorSpeedDefault / 2;
        speed_down_ = kFloorSpeedDefault / 2;
        break;
    }

    default: {
        speed_up_   = 0;
        speed_down_ = 0;
        break;
    }
    }

    destref_ = kTriggerHeightReferenceAbsolute;

    // FIXME!!! Why are we using INT_MAX with a fp number?
    dest_ = (def != kPlaneMoverDefaultDonutFloor) ? 0.0f : (float)INT_MAX;

    switch (def)
    {
    case kPlaneMoverDefaultCeilingLine: {
        otherref_ = (TriggerHeightReference)(kTriggerHeightReferenceCurrent | kTriggerHeightReferenceCeiling);
        break;
    }

    case kPlaneMoverDefaultFloorLine: {
        otherref_ = (TriggerHeightReference)(kTriggerHeightReferenceSurrounding | kTriggerHeightReferenceHighest |
                                             kTriggerHeightReferenceInclude);
        break;
    }

    default: {
        otherref_ = kTriggerHeightReferenceAbsolute;
        break;
    }
    }

    // FIXME!!! Why are we using INT_MAX with a fp number?
    other_ = (def != kPlaneMoverDefaultDonutFloor) ? 0.0f : (float)INT_MAX;

    crush_damage_ = 0;

    tex_.clear();

    wait_    = 0;
    prewait_ = 0;

    sfxstart_ = nullptr;
    sfxup_    = nullptr;
    sfxdown_  = nullptr;
    sfxstop_  = nullptr;

    scroll_angle_ = 0;
    scroll_speed_ = 0.0f;

    ignore_texture_ = false;
}

//
// movplanedef_c assignment operator
//
PlaneMoverDefinition &PlaneMoverDefinition::operator=(const PlaneMoverDefinition &rhs)
{
    if (&rhs != this)
        Copy(rhs);

    return *this;
}

// --> Sliding door definition class

//
// sliding_door_c Constructor
//
SlidingDoor::SlidingDoor()
{
}

//
// sliding_door_c Copy constructor
//
SlidingDoor::SlidingDoor(const SlidingDoor &rhs)
{
    Copy(rhs);
}

//
// sliding_door_c Destructor
//
SlidingDoor::~SlidingDoor()
{
}

//
// sliding_door_c::Copy()
//
void SlidingDoor::Copy(const SlidingDoor &src)
{
    type_        = src.type_;
    speed_       = src.speed_;
    wait_        = src.wait_;
    see_through_ = src.see_through_;
    distance_    = src.distance_;
    sfx_start_   = src.sfx_start_;
    sfx_open_    = src.sfx_open_;
    sfx_close_   = src.sfx_close_;
    sfx_stop_    = src.sfx_stop_;
}

//
// sliding_door_c::Default()
//
void SlidingDoor::Default()
{
    type_        = kSlidingDoorTypeNone;
    speed_       = 4.0f;
    wait_        = 150;
    see_through_ = false;
    distance_    = 0.9f;
    sfx_start_   = nullptr;
    sfx_open_    = nullptr;
    sfx_close_   = nullptr;
    sfx_stop_    = nullptr;
}

//
// sliding_door_c assignment operator
//
SlidingDoor &SlidingDoor::operator=(const SlidingDoor &rhs)
{
    if (&rhs != this)
        Copy(rhs);

    return *this;
}

// --> Teleport point definition class

//
// teleportdef_c Constructor
//
TeleportDefinition::TeleportDefinition()
{
}

//
// teleportdef_c Copy constructor
//
TeleportDefinition::TeleportDefinition(const TeleportDefinition &rhs)
{
    Copy(rhs);
}

//
// teleportdef_c Destructor
//
TeleportDefinition::~TeleportDefinition()
{
}

//
// teleportdef_c::Copy()
//
void TeleportDefinition::Copy(const TeleportDefinition &src)
{
    teleport_ = src.teleport_;

    inspawnobj_     = src.inspawnobj_;
    inspawnobj_ref_ = src.inspawnobj_ref_;

    outspawnobj_     = src.outspawnobj_;
    outspawnobj_ref_ = src.outspawnobj_ref_;

    special_ = src.special_;
    delay_   = src.delay_;
}

//
// teleportdef_c::Default()
//
void TeleportDefinition::Default()
{
    teleport_ = false;

    inspawnobj_ = nullptr;
    inspawnobj_ref_.clear();

    outspawnobj_ = nullptr;
    outspawnobj_ref_.clear();

    delay_   = 0;
    special_ = kTeleportSpecialNone;
}

//
// teleportdef_c assignment operator
//
TeleportDefinition &TeleportDefinition::operator=(const TeleportDefinition &rhs)
{
    if (&rhs != this)
        Copy(rhs);

    return *this;
}

// --> Line definition type class

//
// LineType Constructor
//
LineType::LineType() : number_(0)
{
    Default();
}

//
// LineType Destructor
//
LineType::~LineType()
{
}

void LineType::CopyDetail(const LineType &src)
{
    newtrignum_ = src.newtrignum_;
    type_       = src.type_;
    obj_        = src.obj_;
    keys_       = src.keys_;
    count_      = src.count_;

    f_ = src.f_;
    c_ = src.c_;
    d_ = src.d_;
    s_ = src.s_;
    t_ = src.t_;
    l_ = src.l_;

    ladder_       = src.ladder_;
    e_exit_       = src.e_exit_;
    hub_exit_     = src.hub_exit_;
    s_xspeed_     = src.s_xspeed_;
    s_yspeed_     = src.s_yspeed_;
    scroll_parts_ = src.scroll_parts_;

    failedmessage_ = src.failedmessage_;
    failed_sfx_    = src.failed_sfx_;

    use_colourmap_ = src.use_colourmap_;
    gravity_       = src.gravity_;
    friction_      = src.friction_;
    viscosity_     = src.viscosity_;
    drag_          = src.drag_;
    ambient_sfx_   = src.ambient_sfx_;
    activate_sfx_  = src.activate_sfx_;
    music_         = src.music_;
    autoline_      = src.autoline_;
    singlesided_   = src.singlesided_;
    ef_            = src.ef_;
    translucency_  = src.translucency_;
    appear_        = src.appear_;

    special_flags_  = src.special_flags_;
    trigger_effect_ = src.trigger_effect_;
    line_effect_    = src.line_effect_;
    line_parts_     = src.line_parts_;
    scroll_type_    = src.scroll_type_;
    sector_effect_  = src.sector_effect_;
    portal_effect_  = src.portal_effect_;
    slope_type_     = src.slope_type_;
    fx_color_       = src.fx_color_;

    // lobo 2022
    effectobject_     = src.effectobject_;
    effectobject_ref_ = src.effectobject_ref_;
    glass_            = src.glass_;
    brokentex_        = src.brokentex_;
}

void LineType::Default(void)
{
    newtrignum_ = 0;
    type_       = kLineTriggerNone;
    obj_        = kTriggerActivatorNone;
    keys_       = kDoorKeyNone;
    count_      = -1;

    f_.Default(PlaneMoverDefinition::kPlaneMoverDefaultFloorLine);
    c_.Default(PlaneMoverDefinition::kPlaneMoverDefaultCeilingLine);

    d_.Default();      // Donut
    s_.Default();      // Sliding Door

    t_.Default();      // Teleport
    l_.Default();      // Light definition

    ladder_.Default(); // Ladder

    e_exit_       = kExitTypeNone;
    hub_exit_     = 0;
    s_xspeed_     = 0.0f;
    s_yspeed_     = 0.0f;
    scroll_parts_ = kScrollingPartNone;

    failedmessage_.clear();
    failed_sfx_ = nullptr;

    use_colourmap_ = nullptr;
    gravity_       = kFloatUnused;
    friction_      = kFloatUnused;
    viscosity_     = kFloatUnused;
    drag_          = kFloatUnused;
    ambient_sfx_   = nullptr;
    activate_sfx_  = nullptr;
    music_         = 0;
    autoline_      = false;
    singlesided_   = false;

    ef_.Default();

    translucency_   = 1.0f;
    appear_         = kAppearsWhenDefault;
    special_flags_  = kLineSpecialNone;
    trigger_effect_ = 0;
    line_effect_    = kLineEffectTypeNONE;
    line_parts_     = kScrollingPartNone;
    scroll_type_    = BoomScrollerTypeNone;
    sector_effect_  = kSectorEffectTypeNone;
    portal_effect_  = kPortalEffectTypeNone;
    slope_type_     = kSlopeTypeNONE;
    fx_color_       = kRGBABlack;

    // lobo 2022
    effectobject_ = nullptr;
    effectobject_ref_.clear();
    glass_ = false;
    brokentex_.clear();
}

// --> Line definition type container class

//
// LineTypeContainer Constructor
//
LineTypeContainer::LineTypeContainer()
{
    Reset();
}

//
// LineTypeContainer Destructor
//
LineTypeContainer::~LineTypeContainer()
{
    for (auto iter = begin(); iter != end(); iter++)
    {
        LineType *line = *iter;
        delete line;
        line = nullptr;
    }
}

//
// LineType* LineTypeContainer::Lookup()
//
// Looks an linetype by id, returns nullptr if line can't be found.
//
LineType *LineTypeContainer::Lookup(const int id)
{
    if (id == 0)
        return default_linetype;

    int slot = (((id) + kLookupCacheSize) % kLookupCacheSize);

    // check the cache
    if (lookup_cache_[slot] && lookup_cache_[slot]->number_ == id)
    {
        return lookup_cache_[slot];
    }

    for (auto iter = rbegin(); iter != rend(); iter++)
    {
        LineType *l = *iter;

        if (l->number_ == id)
        {
            // update the cache
            lookup_cache_[slot] = l;
            return l;
        }
    }

    return nullptr;
}

//
// LineTypeContainer::Reset()
//
// Clears down both the data and the cache
//
void LineTypeContainer::Reset()
{
    for (auto iter = begin(); iter != end(); iter++)
    {
        LineType *line = *iter;
        delete line;
        line = nullptr;
    }
    clear();
    EPI_CLEAR_MEMORY(lookup_cache_, LineType *, kLookupCacheSize);
}

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
