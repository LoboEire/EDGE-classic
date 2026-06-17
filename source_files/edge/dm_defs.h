//----------------------------------------------------------------------------
//  EDGE Basic Definitions File
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
//  Based on the DOOM source code, released by Id Software under the
//  following copyright:
//
//    Copyright (C) 1993-1996 by id Software, Inc.
//
//----------------------------------------------------------------------------

#pragma once

#include <stdint.h>

#include "epi_str_hash.h"

//
// Global parameters/defines.
//

// The current state of the game: whether we are
// playing, or gazing at the intermission screen/final animation

enum GameState
{
    kGameStateNothing = 0,
    kGameStateTitleScreen,
    kGameStateLevel,
    kGameStateIntermission,
    kGameStateFinale
};

//
// Difficulty/skill settings/filters.
//

enum SkillLevel
{
    kSkillInvalid     = -1,
    kSkillBaby        = 0,
    kSkillEasy        = 1,
    kSkillMedium      = 2,
    kSkillHard        = 3,
    kSkillNightmare   = 4,
    kTotalSkillLevels = 5
};

// -KM- 1998/12/16 Added gameflags typedef here.
enum AutoAimState
{
    kAutoAimOff,
    kAutoAimVertical,
    kAutoAimVerticalSnap,
    kAutoAimFull,
    kAutoAimFullSnap
};

struct GameFlags
{
    // checkparm of -nomonsters
    bool no_monsters;

    // checkparm of -fast
    bool fast_monsters;

    bool enemies_respawn;
    bool enemy_respawn_mode;
    bool items_respawn;

    bool true_3d_gameplay;
    int  menu_gravity_factor;
    bool more_blood;

    bool         jump;
    bool         crouch;
    bool         mouselook;
    AutoAimState autoaim;

    bool cheats;
    bool have_extra;
    bool limit_zoom;

    bool kicking;
    bool weapon_switch;
    bool pass_missile;
    bool team_damage;
};

//
// DOOM keyboard definition.
// This is the stuff configured by Setup.Exe.
// Most key data are simple ascii (uppercased).
//

constexpr uint8_t kTab       = 9;
constexpr uint8_t kEnter     = 13;
constexpr uint8_t kEscape    = 27;
constexpr uint8_t kSpace     = 32;
constexpr uint8_t kBackspace = 127;

constexpr uint8_t kTilde      = ('`');
constexpr uint8_t kEquals     = ('=');
constexpr uint8_t kMinus      = ('-');
constexpr uint8_t kRightArrow = (0x80 + 0x2e);
constexpr uint8_t kLeftArrow  = (0x80 + 0x2c);
constexpr uint8_t kUpArrow    = (0x80 + 0x2d);
constexpr uint8_t kDownArrow  = (0x80 + 0x2f);

constexpr uint8_t kRightControl = (0x80 + 0x1d);
constexpr uint8_t kRightShift   = (0x80 + 0x36);
constexpr uint8_t kRightAlt     = (0x80 + 0x38);
constexpr uint8_t kLeftAlt      = kRightAlt;
constexpr uint8_t kHome         = (0x80 + 0x47);
constexpr uint8_t kPageUp       = (0x80 + 0x49);
constexpr uint8_t kEnd          = (0x80 + 0x4f);
constexpr uint8_t kPageDown     = (0x80 + 0x51);
constexpr uint8_t kInsert       = (0x80 + 0x52);
constexpr uint8_t kDelete       = (0x80 + 0x53);

constexpr uint8_t kFunction1  = (0x80 + 0x3b);
constexpr uint8_t kFunction2  = (0x80 + 0x3c);
constexpr uint8_t kFunction3  = (0x80 + 0x3d);
constexpr uint8_t kFunction4  = (0x80 + 0x3e);
constexpr uint8_t kFunction5  = (0x80 + 0x3f);
constexpr uint8_t kFunction6  = (0x80 + 0x40);
constexpr uint8_t kFunction7  = (0x80 + 0x41);
constexpr uint8_t kFunction8  = (0x80 + 0x42);
constexpr uint8_t kFunction9  = (0x80 + 0x43);
constexpr uint8_t kFunction10 = (0x80 + 0x44);
constexpr uint8_t kFunction11 = (0x80 + 0x57);
constexpr uint8_t kFunction12 = (0x80 + 0x58);

constexpr uint8_t kKeypad0      = (0x80 + 0x60);
constexpr uint8_t kKeypad1      = (0x80 + 0x61);
constexpr uint8_t kKeypad2      = (0x80 + 0x62);
constexpr uint8_t kKeypad3      = (0x80 + 0x63);
constexpr uint8_t kKeypad4      = (0x80 + 0x64);
constexpr uint8_t kKeypad5      = (0x80 + 0x65);
constexpr uint8_t kKeypad6      = (0x80 + 0x66);
constexpr uint8_t kKeypad7      = (0x80 + 0x67);
constexpr uint8_t kKeypad8      = (0x80 + 0x68);
constexpr uint8_t kKeypad9      = (0x80 + 0x69);
constexpr uint8_t kKeypadDot    = (0x80 + 0x6a);
constexpr uint8_t kKeypadPlus   = (0x80 + 0x6b);
constexpr uint8_t kKeypadMinus  = (0x80 + 0x6c);
constexpr uint8_t kKeypadStar   = (0x80 + 0x6d);
constexpr uint8_t kKeypadSlash  = (0x80 + 0x6e);
constexpr uint8_t kKeypadEquals = (0x80 + 0x6f);
constexpr uint8_t kKeypadEnter  = (0x80 + 0x70);

constexpr uint8_t kPrintScreen = (0x80 + 0x54);
constexpr uint8_t kNumberLock  = (0x80 + 0x45);
constexpr uint8_t kScrollLock  = (0x80 + 0x46);
constexpr uint8_t kCapsLock    = (0x80 + 0x7e);
constexpr uint8_t kPause       = (0x80 + 0x7f);

// Values from here on aren't actually keyboard keys, but buttons
// on joystick or mice.

constexpr uint16_t kMouse1         = (0x100);
constexpr uint16_t kMouse2         = (0x101);
constexpr uint16_t kMouse3         = (0x102);
constexpr uint16_t kMouse4         = (0x103);
constexpr uint16_t kMouse5         = (0x104);
constexpr uint16_t kMouse6         = (0x105);
constexpr uint16_t kMouseWheelUp   = (0x10e);
constexpr uint16_t kMouseWheelDown = (0x10f);

constexpr uint16_t kGamepadA             = (0x110 + 1);
constexpr uint16_t kGamepadB             = (0x110 + 2);
constexpr uint16_t kGamepadX             = (0x110 + 3);
constexpr uint16_t kGamepadY             = (0x110 + 4);
constexpr uint16_t kGamepadBack          = (0x110 + 5);
constexpr uint16_t kGamepadGuide         = (0x110 + 6);
constexpr uint16_t kGamepadStart         = (0x110 + 7);
constexpr uint16_t kGamepadLeftStick     = (0x110 + 8);
constexpr uint16_t kGamepadRightStick    = (0x110 + 9);
constexpr uint16_t kGamepadLeftShoulder  = (0x110 + 10);
constexpr uint16_t kGamepadRightShoulder = (0x110 + 11);
constexpr uint16_t kGamepadUp            = (0x110 + 12);
constexpr uint16_t kGamepadDown          = (0x110 + 13);
constexpr uint16_t kGamepadLeft          = (0x110 + 14);
constexpr uint16_t kGamepadRight         = (0x110 + 15);
constexpr uint16_t kGamepadMisc1         = (0x110 + 12);
constexpr uint16_t kGamepadPaddle1       = (0x110 + 13);
constexpr uint16_t kGamepadPaddle2       = (0x110 + 14);
constexpr uint16_t kGamepadPaddle3       = (0x110 + 15);
constexpr uint16_t kGamepadPaddle4       = (0x110 + 16);
constexpr uint16_t kGamepadTouchpad      = (0x110 + 17);
constexpr uint16_t kGamepadTriggerLeft   = (0x110 + 18);
constexpr uint16_t kGamepadTriggerRight  = (0x110 + 19);

// Pseudo-keycodes for program functions - Dasho;
constexpr uint16_t kScreenshot    = (0x110 + 29);
constexpr uint16_t kSaveGame      = (0x110 + 30);
constexpr uint16_t kLoadGame      = (0x110 + 31);
constexpr uint16_t kSoundControls = (0x110 + 32);
constexpr uint16_t kOptionsMenu   = (0x110 + 33);
constexpr uint16_t kQuickSave     = (0x110 + 34);
constexpr uint16_t kEndGame       = (0x110 + 35);

constexpr uint16_t kQuickLoad   = (0x110 + 37);
constexpr uint16_t kQuitEdge    = (0x110 + 38);
constexpr uint16_t kGammaToggle = (0x110 + 39);

// -KM- 1998/09/27 Analogue binding, added a fly axis
constexpr uint8_t kAxisDisable   = 0;
constexpr uint8_t kAxisTurn      = 1;
constexpr uint8_t kAxisMouselook = 2;
constexpr uint8_t kAxisForward   = 3;
constexpr uint8_t kAxisStrafe    = 4;
constexpr uint8_t kAxisFly       = 5; // includes SWIM up/down

// Indicate a leaf.
constexpr uint32_t kLeafSubsector = (uint32_t)(1 << 31);

/* ----- The wad structures ---------------------- */

// wad header
#pragma pack(push, 1)
struct RawWadHeader
{
    char magic[4];

    uint32_t total_entries;
    uint32_t directory_start;
};
#pragma pack(pop)

// directory entry
#pragma pack(push, 1)
struct RawWadEntry
{
    uint32_t position;
    uint32_t size;

    char name[8];
};
#pragma pack(pop)

// Lump order in a map WAD: each map needs a couple of lumps
// to provide a complete scene geometry description.
enum LumpOrder
{
    kLumpLabel = 0,  // A separator name, ExMx or MAPxx
    kLumpThings,     // Monsters, items..
    kLumpLinedefs,   // LineDefs, from editing
    kLumpSidedefs,   // SideDefs, from editing
    kLumpVertexes,   // Vertices, edited and BSP splits generated
    kLumpSegs,       // LineSegs, from LineDefs split by BSP
    kLumpSubSectors, // SubSectors, list of LineSegs
    kLumpNodes,      // BSP nodes
    kLumpSectors,    // Sectors, from editing
    kLumpReject,     // LUT, sector-sector visibility
    kLumpBlockmap,   // LUT, motion clipping, walls/grid element
    kLumpBehavior    // Hexen scripting stuff
};

/* ----- The level structures ---------------------- */
#pragma pack(push, 1)
struct RawVertex
{
    int16_t x, y;
};
#pragma pack(pop)

#pragma pack(push, 1)
struct RawV2Vertex
{
    int32_t x, y;
};
#pragma pack(pop)

#pragma pack(push, 1)
struct RawLinedef
{
    uint16_t start; // from this vertex...
    uint16_t end;   // ... to this vertex
    uint16_t flags; // linedef flags (impassible, etc)
    uint16_t type;  // special type (0 for none, 97 for teleporter, etc)
    int16_t  tag;   // this linedef activates the sector with same tag
    uint16_t right; // right sidedef
    uint16_t left;  // left sidedef (only if this line adjoins 2 sectors)
};
#pragma pack(pop)

#pragma pack(push, 1)
struct RawSidedef
{
    int16_t x_offset;      // X offset for texture
    int16_t y_offset;      // Y offset for texture

    char upper_texture[8]; // texture name for the part above
    char lower_texture[8]; // texture name for the part below
    char mid_texture[8];   // texture name for the regular part

    uint16_t sector;       // adjacent sector
};
#pragma pack(pop)

#pragma pack(push, 1)
struct RawSector
{
    int16_t floor_height;   // floor height
    int16_t ceiling_height; // ceiling height

    char floor_texture[8];  // floor texture
    char ceil_texture[8];   // ceiling texture

    uint16_t light;         // light level (0-255)
    uint16_t type;          // special type (0 = normal, 9 = secret, ...)
    int16_t  tag;           // sector activated by a linedef with same tag
};
#pragma pack(pop)

#pragma pack(push, 1)
struct RawThing
{
    int16_t  x, y;    // position of thing
    int16_t  angle;   // angle thing faces (degrees)
    uint16_t type;    // type of thing
    uint16_t options; // when appears, deaf, etc..
};
#pragma pack(pop)

/* ----- The BSP tree structures ----------------------- */
#pragma pack(push, 1)
struct RawBoundingBox
{
    int16_t maximum_y, minimum_y;
    int16_t minimum_x, maximum_x;
};
#pragma pack(pop)

#pragma pack(push, 1)
struct RawV5Node
{
    // this structure used by ZDoom nodes too

    int16_t        x, y;                           // starting point
    int16_t        delta_x, delta_y;               // offset to ending point
    RawBoundingBox bounding_box_1, bounding_box_2; // bounding rectangles
    uint32_t       right, left;                    // children: Node or SSector (if high bit is set)
};
#pragma pack(pop)

/* ----- Graphical structures ---------------------- */
#pragma pack(push, 1)
struct RawPatchDefinition
{
    int16_t x_origin;
    int16_t y_origin;

    uint16_t pname;    // index into PNAMES
    uint16_t stepdir;  // NOT USED
    uint16_t colormap; // NOT USED
};
#pragma pack(pop)

#pragma pack(push, 1)
struct RawStrifePatchDefinition
{
    int16_t  x_origin;
    int16_t  y_origin;
    uint16_t pname; // index into PNAMES
};
#pragma pack(pop)

// Texture definition.
//
// Each texture is composed of one or more patches,
// with patches being lumps stored in the WAD.
//
#pragma pack(push, 1)
struct RawTexture
{
    char name[8];

    uint16_t masked; // NOT USED
    uint8_t  scale_x;
    uint8_t  scale_y;
    uint16_t width;
    uint16_t height;
    uint32_t column_dir; // NOT USED
    uint16_t patch_count;

    RawPatchDefinition patches[1];
};
#pragma pack(pop)

#pragma pack(push, 1)
struct RawStrifeTexture
{
    char name[8];

    uint16_t masked; // NOT USED
    uint8_t  scale_x;
    uint8_t  scale_y;
    uint16_t width;
    uint16_t height;
    uint16_t patch_count;

    RawStrifePatchDefinition patches[1];
};
#pragma pack(pop)

// Patches.
//
// A patch holds one or more columns.
// Patches are used for sprites and all masked pictures,
// and we compose textures from the TEXTURE1/2 lists
// of patches.
//
#pragma pack(push, 1)
struct Patch
{
    // bounding box size
    int16_t width;
    int16_t height;

    // pixels to the left of origin
    int16_t left_offset;

    // pixels below the origin
    int16_t top_offset;

    uint32_t column_offset[1]; // only [width] used
};
#pragma pack(pop)

//
// LineDef attributes.
//

enum LineFlag
{
    // solid, is an obstacle
    kLineFlagBlocking = 0x0001,

    // blocks monsters only
    kLineFlagBlockMonsters = 0x0002,

    // backside will not be present at all if not two sided
    kLineFlagTwoSided = 0x0004,

    // If a texture is pegged, the texture will have
    // the end exposed to air held constant at the
    // top or bottom of the texture (stairs or pulled
    // down things) and will move with a height change
    // of one of the neighbor sectors.
    //
    // Unpegged textures allways have the first row of
    // the texture at the top pixel of the line for both
    // top and bottom textures (use next to windows).

    // upper texture unpegged
    kLineFlagUpperUnpegged = 0x0008,

    // lower texture unpegged
    kLineFlagLowerUnpegged = 0x0010,

    // in AutoMap: don't map as two sided: IT'S A SECRET!
    kLineFlagSecret = 0x0020,

    // sound rendering: don't let sound cross two of these
    kLineFlagSoundBlock = 0x0040,

    // don't draw on the automap at all
    kLineFlagDontDraw = 0x0080,

    // set as if already seen, thus drawn in automap
    kLineFlagMapped = 0x0100,

    // -AJA- this one is from Boom. Allows multiple lines to
    //       be pushed simultaneously.
    kLineFlagBoomPassThrough = 0x0200,

    // 0x0400 is Eternity's 3DMidTex flag - Dasho

    // Clear extended line flags (BOOM or later spec); needed to repair
    // mapping/editor errors with historical maps (i.e., E2M7)
    kLineFlagClearBoomFlags = 0x0800,

    // MBF21
    kLineFlagBlockGroundedMonsters = 0x1000,

    // MBF21
    kLineFlagBlockPlayers = 0x2000,

    // ----- internal flags -----

    kLineFlagMirror = (1 << 16),

    // -AJA- These two from XDoom.
    // Dasho - Moved to internal flag range to make room for MBF21 stuff
    kLineFlagShootBlock = (1 << 17),

    kLineFlagSightBlock = (1 << 18),
};

constexpr int16_t kBoomGeneralizedLineFirst = 0x2f80;
constexpr int16_t kBoomGeneralizedLineLast  = 0x7fff;

inline bool IsBoomGeneralizedLine(int16_t line)
{
    return (line >= kBoomGeneralizedLineFirst && line <= kBoomGeneralizedLineLast);
}

//
// Sector attributes.
//

enum BoomSectorFlag
{
    kBoomSectorFlagTypeMask   = 0x001F,
    kBoomSectorFlagDamageMask = 0x0060,
    kBoomSectorFlagSecret     = 0x0080,
    kBoomSectorFlagFriction   = 0x0100,
    kBoomSectorFlagPush       = 0x0200,
    kBoomSectorFlagNoSounds   = 0x0400,
    kBoomSectorFlagQuietPlane = 0x0800
};

constexpr int16_t kBoomFlagBits = 0x0FE0;

//
// Thing attributes.
//

enum ThingOption
{
    kThingEasy            = 1,
    kThingMedium          = 2,
    kThingHard            = 4,
    kThingAmbush          = 8,
    kThingNotSinglePlayer = 16,
    kThingNotDeathmatch   = 32,
    kThingNotCooperative  = 64,
    kThingFriend          = 128,
    kThingReserved        = 256,
};

constexpr int16_t kExtrafloorMask     = 0x3C00;
constexpr uint8_t kExtrafloorBitShift = 10;

// Known string hashes for parsing UDMF

namespace udmf
{

// generic keys
EPI_KNOWN_STRINGHASH(kSpecial, "SPECIAL")
EPI_KNOWN_STRINGHASH(kID, "ID")
EPI_KNOWN_STRINGHASH(kX, "X")
EPI_KNOWN_STRINGHASH(kY, "Y")
EPI_KNOWN_STRINGHASH(kSector, "SECTOR")
EPI_KNOWN_STRINGHASH(kThing, "THING")
EPI_KNOWN_STRINGHASH(kVertex, "VERTEX")
EPI_KNOWN_STRINGHASH(kLinedef, "LINEDEF")
EPI_KNOWN_STRINGHASH(kSidedef, "SIDEDEF")

// vertexes
EPI_KNOWN_STRINGHASH(kZFloor, "ZFLOOR")
EPI_KNOWN_STRINGHASH(kZCeiling, "ZCEILING")

// linedefs
EPI_KNOWN_STRINGHASH(kArg0, "ARG0")
EPI_KNOWN_STRINGHASH(kV1, "V1")
EPI_KNOWN_STRINGHASH(kV2, "V2")
EPI_KNOWN_STRINGHASH(kSideFront, "SIDEFRONT")
EPI_KNOWN_STRINGHASH(kSideBack, "SIDEBACK")
EPI_KNOWN_STRINGHASH(kBlocking, "BLOCKING")
EPI_KNOWN_STRINGHASH(kBlockMonsters, "BLOCKMONSTERS")
EPI_KNOWN_STRINGHASH(kTwoSided, "TWOSIDED")
EPI_KNOWN_STRINGHASH(kDontPegTop, "DONTPEGTOP")
EPI_KNOWN_STRINGHASH(kDontPegBottom, "DONTPEGBOTTOM")
EPI_KNOWN_STRINGHASH(kSecret, "SECRET")
EPI_KNOWN_STRINGHASH(kBlockSound, "BLOCKSOUND")
EPI_KNOWN_STRINGHASH(kDontDraw, "DONTDRAW")
EPI_KNOWN_STRINGHASH(kMapped, "MAPPED")
EPI_KNOWN_STRINGHASH(kPassUse, "PASSUSE")
EPI_KNOWN_STRINGHASH(kBlockPlayers, "BLOCKPLAYERS")
EPI_KNOWN_STRINGHASH(kBlockSight, "BLOCKSIGHT")

// sidedefs
EPI_KNOWN_STRINGHASH(kOffsetX, "OFFSETX")
EPI_KNOWN_STRINGHASH(kOffsetY, "OFFSETY")
EPI_KNOWN_STRINGHASH(kOffsetX_Bottom, "OFFSETX_BOTTOM")
EPI_KNOWN_STRINGHASH(kOffsetX_Mid, "OFFSETX_MID")
EPI_KNOWN_STRINGHASH(kOffsetX_Top, "OFFSETX_TOP")
EPI_KNOWN_STRINGHASH(kOffsetY_Bottom, "OFFSETY_BOTTOM")
EPI_KNOWN_STRINGHASH(kOffsetY_Mid, "OFFSETY_MID")
EPI_KNOWN_STRINGHASH(kOffsetY_Top, "OFFSETY_TOP")
EPI_KNOWN_STRINGHASH(kScaleX_Bottom, "SCALEX_BOTTOM")
EPI_KNOWN_STRINGHASH(kScaleX_Mid, "SCALEX_MID")
EPI_KNOWN_STRINGHASH(kScaleX_Top, "SCALEX_TOP")
EPI_KNOWN_STRINGHASH(kScaleY_Bottom, "SCALEY_BOTTOM")
EPI_KNOWN_STRINGHASH(kScaleY_Mid, "SCALEY_MID")
EPI_KNOWN_STRINGHASH(kScaleY_Top, "SCALEY_TOP")
EPI_KNOWN_STRINGHASH(kTextureTop, "TEXTURETOP")
EPI_KNOWN_STRINGHASH(kTextureBottom, "TEXTUREBOTTOM")
EPI_KNOWN_STRINGHASH(kTextureMiddle, "TEXTUREMIDDLE")

// sectors
EPI_KNOWN_STRINGHASH(kHeightFloor, "HEIGHTFLOOR")
EPI_KNOWN_STRINGHASH(kHeightCeiling, "HEIGHTCEILING")
EPI_KNOWN_STRINGHASH(kTextureFloor, "TEXTUREFLOOR")
EPI_KNOWN_STRINGHASH(kTextureCeiling, "TEXTURECEILING")
EPI_KNOWN_STRINGHASH(kLightLevel, "LIGHTLEVEL")
EPI_KNOWN_STRINGHASH(kLightColor, "LIGHTCOLOR")
EPI_KNOWN_STRINGHASH(kFadeColor, "FADECOLOR")
EPI_KNOWN_STRINGHASH(kFogDensity, "FOGDENSITY")
EPI_KNOWN_STRINGHASH(kXPanningFloor, "XPANNINGFLOOR")
EPI_KNOWN_STRINGHASH(kYPanningFloor, "YPANNINGFLOOR")
EPI_KNOWN_STRINGHASH(kXPanningCeiling, "XPANNINGCEILING")
EPI_KNOWN_STRINGHASH(kYPanningCeiling, "YPANNINGCEILING")
EPI_KNOWN_STRINGHASH(kXScaleFloor, "XSCALEFLOOR")
EPI_KNOWN_STRINGHASH(kYScaleFloor, "YSCALEFLOOR")
EPI_KNOWN_STRINGHASH(kXScaleCeiling, "XSCALECEILING")
EPI_KNOWN_STRINGHASH(kYScaleCeiling, "YSCALECEILING")
EPI_KNOWN_STRINGHASH(kAlphaFloor, "ALPHAFLOOR")
EPI_KNOWN_STRINGHASH(kAlphaCeiling, "ALPHACEILING")
EPI_KNOWN_STRINGHASH(kRotationFloor, "ROTATIONFLOOR")
EPI_KNOWN_STRINGHASH(kRotationCeiling, "ROTATIONCEILING")
EPI_KNOWN_STRINGHASH(kGravity, "GRAVITY")
EPI_KNOWN_STRINGHASH(kReverbPreset, "REVERBPRESET")

// things
EPI_KNOWN_STRINGHASH(kHeight, "HEIGHT")
EPI_KNOWN_STRINGHASH(kAngle, "ANGLE")
EPI_KNOWN_STRINGHASH(kType, "TYPE")
EPI_KNOWN_STRINGHASH(kSkill1, "SKILL1")
EPI_KNOWN_STRINGHASH(kSkill2, "SKILL2")
EPI_KNOWN_STRINGHASH(kSkill3, "SKILL3")
EPI_KNOWN_STRINGHASH(kSkill4, "SKILL4")
EPI_KNOWN_STRINGHASH(kSkill5, "SKILL5")
EPI_KNOWN_STRINGHASH(kAmbush, "AMBUSH")
EPI_KNOWN_STRINGHASH(kSingle, "SINGLE")
EPI_KNOWN_STRINGHASH(kDM, "DM")
EPI_KNOWN_STRINGHASH(kCoop, "COOP")
EPI_KNOWN_STRINGHASH(kFriend, "FRIEND")
EPI_KNOWN_STRINGHASH(kHealth, "HEALTH")
EPI_KNOWN_STRINGHASH(kAlpha, "ALPHA")
EPI_KNOWN_STRINGHASH(kScale, "SCALE")
EPI_KNOWN_STRINGHASH(kScaleX, "SCALEX")
EPI_KNOWN_STRINGHASH(kScaleY, "SCALEY")

} // namespace udmf

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
