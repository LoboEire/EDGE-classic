//----------------------------------------------------------------------------
//  EDGE Data Definition File Code (Main)
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

#pragma once

#include "ddf_colormap.h"
#include "ddf_types.h"
#include "epi.h"

class GameDefinition;

class FinaleDefinition
{
  public:
    FinaleDefinition();
    FinaleDefinition(const FinaleDefinition &rhs);
    ~FinaleDefinition();

  private:
    void Copy(const FinaleDefinition &src);

  public:
    void              Default(void);
    FinaleDefinition &operator=(const FinaleDefinition &rhs);

    // Text
    std::string     text_;
    std::string     text_back_;
    std::string     text_flat_;
    float           text_speed_;
    unsigned int    text_wait_;
    const Colormap *text_colmap_;

    // Movie
    std::string movie_;

    // Pic
    std::vector<std::string> pics_;
    unsigned int             picwait_;

    // Cast
    bool docast_;

    // Bunny
    bool dobunny_;

    // Music
    int music_;
};

enum MapFlag
{
    kMapFlagNone                = 0x0,
    kMapFlagJumping             = (1 << 0),
    kMapFlagMlook               = (1 << 1),
    kMapFlagCheats              = (1 << 2),
    kMapFlagItemRespawn         = (1 << 3),
    kMapFlagFastParm            = (1 << 4), // Fast Monsters
    kMapFlagResRespawn          = (1 << 5), // Resurrect Monsters (else Teleport)
    kMapFlagTrue3D              = (1 << 6), // True 3D Gameplay
    kMapFlagStomp               = (1 << 7), // Monsters can stomp players
    kMapFlagMoreBlood           = (1 << 8), // Make a bloody mess
    kMapFlagRespawn             = (1 << 9),
    kMapFlagAutoAimFull         = (1 << 10),
    kMapFlagAutoAimFullSnap     = (1 << 11),
    kMapFlagAutoAimVertical     = (1 << 12),
    kMapFlagAutoAimVerticalSnap = (1 << 13),
    kMapFlagResetPlayer         = (1 << 14), // Force player back to square #1
    kMapFlagExtras              = (1 << 15),
    kMapFlagLimitZoom           = (1 << 16), // Limit zoom to certain weapons
    kMapFlagCrouching           = (1 << 17),
    kMapFlagKicking             = (1 << 18), // Weapon recoil
    kMapFlagWeaponSwitch        = (1 << 19),
    kMapFlagPassMissile         = (1 << 20),
    kMapFlagTeamDamage          = (1 << 21),
};

enum SkyStretch
{
    kSkyStretchUnset   = -1,
    kSkyStretchMirror  = 0,
    kSkyStretchRepeat  = 1,
    kSkyStretchStretch = 2,
    kSkyStretchVanilla = 3,
};

enum IntermissionStyle
{
    // standard Doom intermission stats
    kIntermissionStyleDoom = 0,
    // no stats at all
    kIntermissionStyleNone = 1
};

class MapDefinition
{
  public:
    MapDefinition();
    ~MapDefinition();

  public:
    void Default(void);
    void CopyDetail(const MapDefinition &src);

    // Member vars....
    std::string name_;

    // level description, a reference to languages.ldf
    std::string description_;

    std::string namegraphic_;
    std::string leavingbggraphic_;
    std::string enteringbggraphic_;
    std::string lump_;
    std::string sky_;
    std::string surround_;

    int music_;

    int partime_;

    GameDefinition *episode_; // set during DDFCleanUp
    std::string     episode_name_;

    // flags come in two flavours: "force on" and "force off".  When not
    // forced, then the user is allowed to control it (not applicable to
    // all the flags, e.g. RESET_PLAYER).
    int force_on_;
    int force_off_;

    // name of the next normal level
    std::string next_mapname_;

    // name of the secret level
    std::string secretmapname_;

    // -KM- 1998/11/25 All lines with this trigger will be activated at
    // the level start. (MAP07)
    int autotag_;

    IntermissionStyle wistyle_;

    // -KM- 1998/11/25 Generalised finales.
    FinaleDefinition f_pre_;
    FinaleDefinition f_end_;

    // optional *MAPINFO field
    std::string author_;

    // sky stretch override
    SkyStretch forced_skystretch_;
    float      sky_scroll_x_;
    float      sky_scroll_y_;

    Colormap *indoor_fog_cmap_;
    RGBAColor indoor_fog_color_;
    float     indoor_fog_density_;
    Colormap *outdoor_fog_cmap_;
    RGBAColor outdoor_fog_color_;
    float     outdoor_fog_density_;

  private:
    // disable copy construct and assignment operator
    explicit MapDefinition(MapDefinition &rhs)
    {
        EPI_UNUSED(rhs);
    }
    MapDefinition &operator=(MapDefinition &rhs)
    {
        EPI_UNUSED(rhs);
        return *this;
    }
};

class MapDefinitionContainer : public std::vector<MapDefinition *>
{
  public:
    MapDefinitionContainer()
    {
    }
    ~MapDefinitionContainer()
    {
        for (std::vector<MapDefinition *>::iterator iter = begin(), iter_end = end(); iter != iter_end; iter++)
        {
            MapDefinition *map = *iter;
            delete map;
            map = nullptr;
        }
    }

  public:
    MapDefinition *Lookup(const char *name);
};

extern MapDefinitionContainer mapdefs; // -ACB- 2004/06/29 Implemented

void DDFReadLevels(const std::string &data);

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
