//----------------------------------------------------------------------------
//  EDGE Data Definition File Code (Game settings)
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
// Overall Game Setup and Parser Code
//

#include <string.h>

#include "ddf_local.h"

GameDefinitionContainer gamedefs;

static GameDefinition *dynamic_gamedef;

static IntermissionAnimationInfo buffer_animdef;
static IntermissionFrameInfo     buffer_framedef;

static void DDFGameGetPic(const char *info, void *storage);
static void DDFGameGetAnim(const char *info, void *storage);
static void DDFGameGetMap(const char *info, void *storage);
static void DDFGameGetLighting(const char *info, void *storage);

static GameDefinition dummy_gamedef;

static const DDFCommandList gamedef_commands[] = {
    DDF_FIELD("INTERMISSION_GRAPHIC", dummy_gamedef, background_, DDFMainGetLumpName),
    DDF_FIELD("INTERMISSION_CAMERA", dummy_gamedef, bg_camera_, DDFMainGetString),
    DDF_FIELD("INTERMISSION_MUSIC", dummy_gamedef, music_, DDFMainGetNumeric),
    DDF_FIELD("SPLAT_GRAPHIC", dummy_gamedef, splatpic_, DDFMainGetLumpName),
    DDF_FIELD("YAH1_GRAPHIC", dummy_gamedef, you_are_here_[0], DDFMainGetLumpName),
    DDF_FIELD("YAH2_GRAPHIC", dummy_gamedef, you_are_here_[1], DDFMainGetLumpName),
    DDF_FIELD("PERCENT_SOUND", dummy_gamedef, percent_, DDFMainLookupSound),
    DDF_FIELD("DONE_SOUND", dummy_gamedef, done_, DDFMainLookupSound),
    DDF_FIELD("ENDMAP_SOUND", dummy_gamedef, endmap_, DDFMainLookupSound),
    DDF_FIELD("NEXTMAP_SOUND", dummy_gamedef, next_map_, DDFMainLookupSound),
    DDF_FIELD("ACCEL_SOUND", dummy_gamedef, accel_snd_, DDFMainLookupSound),
    DDF_FIELD("FRAG_SOUND", dummy_gamedef, frag_snd_, DDFMainLookupSound),
    DDF_FIELD("FIRSTMAP", dummy_gamedef, firstmap_, DDFMainGetLumpName),
    DDF_FIELD("NAME_GRAPHIC", dummy_gamedef, namegraphic_, DDFMainGetLumpName),
    DDF_FIELD("TITLE_MOVIE", dummy_gamedef, titlemovie_, DDFMainGetString),
    DDF_FIELD("TITLE_MUSIC", dummy_gamedef, titlemusic_, DDFMainGetNumeric),
    DDF_FIELD("TITLE_TIME", dummy_gamedef, titletics_, DDFMainGetTime),
    DDF_FIELD("SPECIAL_MUSIC", dummy_gamedef, special_music_, DDFMainGetNumeric),
    DDF_FIELD("LIGHTING", dummy_gamedef, lighting_, DDFGameGetLighting),
    DDF_FIELD("DESCRIPTION", dummy_gamedef, description_, DDFMainGetString),
    DDF_FIELD("NO_SKILL_MENU", dummy_gamedef, no_skill_menu_, DDFMainGetBoolean),
    DDF_FIELD("DEFAULT_DAMAGE_FLASH", dummy_gamedef, default_damage_flash_, DDFMainGetRGB),

    {nullptr, nullptr, 0, nullptr}};

//
//  DDF PARSE ROUTINES
//

static void GameStartEntry(const char *name, bool extend)
{
    if (!name || !name[0])
    {
        DDFWarnError("New game entry is missing a name!");
        name = "GAME_WITH_NO_NAME";
    }

    // instantiate the static entries
    buffer_animdef.Default();
    buffer_framedef.Default();

    // replaces an existing entry?
    dynamic_gamedef = gamedefs.Lookup(name);

    if (extend)
    {
        if (!dynamic_gamedef)
            DDFError("Unknown game to extend: %s\n", name);
        return;
    }

    if (dynamic_gamedef)
    {
        dynamic_gamedef->Default();
        return;
    }

    // not found, create a new one
    dynamic_gamedef        = new GameDefinition;
    dynamic_gamedef->name_ = name;

    gamedefs.push_back(dynamic_gamedef);
}

static void GameDoTemplate(const char *contents)
{
    GameDefinition *other = gamedefs.Lookup(contents);

    if (!other || other == dynamic_gamedef)
        DDFError("Unknown game template: '%s'\n", contents);

    dynamic_gamedef->CopyDetail(*other);
}

static void GameParseField(const char *field, const char *contents, int index, bool is_last)
{
#if (DDF_DEBUG)
    LogDebug("GAME_PARSE: %s = %s;\n", field, contents);
#endif
    EPI_UNUSED(index);
    EPI_UNUSED(is_last);
    if (DDFCompareName(field, "TEMPLATE") == 0)
    {
        GameDoTemplate(contents);
        return;
    }

    // handle some special fields...
    if (DDFCompareName(field, "TITLE_GRAPHIC") == 0)
    {
        DDFGameGetPic(contents, nullptr);
        return;
    }
    else if (DDFCompareName(field, "MAP") == 0)
    {
        DDFGameGetMap(contents, nullptr);
        return;
    }
    else if (DDFCompareName(field, "ANIM") == 0)
    {
        DDFGameGetAnim(contents, &buffer_framedef);
        return;
    }

    if (DDFMainParseField(gamedef_commands, field, contents, (uint8_t *)dynamic_gamedef))
        return; // OK

    DDFWarnError("Unknown games.ddf command: %s\n", field);
}

static void GameFinishEntry(void)
{
    // TODO: check stuff...
}

static void GameClearAll(void)
{
    // 100% safe to delete all game entries
    for (GameDefinition *game : gamedefs)
    {
        delete game;
        game = nullptr;
    }
    gamedefs.clear();
}

void DDFReadGames(const std::string &data)
{
    DDFReadInfo games;

    games.tag      = "GAMES";
    games.lumpname = "DDFGAME";

    games.start_entry  = GameStartEntry;
    games.parse_field  = GameParseField;
    games.finish_entry = GameFinishEntry;
    games.clear_all    = GameClearAll;

    DDFMainReadFile(&games, data);
}

void DDFGameInit(void)
{
    GameClearAll();
}

void DDFGameCleanUp(void)
{
    if (gamedefs.empty())
        FatalError("There are no games defined in DDF !\n");
}

static void DDFGameAddFrame(void)
{
    IntermissionFrameInfo *f = new IntermissionFrameInfo(buffer_framedef);

    buffer_animdef.frames_.push_back(f);

    buffer_framedef.Default();
}

static void DDFGameAddAnim(void)
{
    IntermissionAnimationInfo *a = new IntermissionAnimationInfo(buffer_animdef);

    if (a->level_[0])
        a->type_ = IntermissionAnimationInfo::kIntermissionAnimationInfoLevel;
    else
        a->type_ = IntermissionAnimationInfo::kIntermissionAnimationInfoNormal;

    dynamic_gamedef->anims_.push_back(a);

    buffer_animdef.Default();
}

static void ParseFrame(const char *info, IntermissionFrameInfo *f)
{
    const char *p = strchr(info, ':');
    if (!p || p == info)
        DDFError("Bad frame def: '%s' (missing pic name)\n", info);

    f->pic_ = std::string(info, p - info);

    p++;

    if (sscanf(p, " %d : %d : %d ", &f->tics_, &f->x_, &f->y_) != 3)
        DDFError("Bad frame definition: '%s'\n", info);
}

static void DDFGameGetAnim(const char *info, void *storage)
{
    IntermissionFrameInfo *f = (IntermissionFrameInfo *)storage;

    if (DDFCompareName(info, "#END") == 0)
    {
        DDFGameAddAnim();
        return;
    }

    const char *p = info;

    if (info[0] == '#')
    {
        if (buffer_animdef.frames_.size() > 0)
            DDFError("Invalid # command: '%s'\n", info);

        p = strchr(info, ':');
        if (!p || p <= info + 1)
            DDFError("Invalid # command: '%s'\n", info);

        buffer_animdef.level_ = std::string(info + 1, p - (info + 1));

        p++;
    }

    ParseFrame(p, f);

    // this assumes 'f' points to buffer_framedef
    DDFGameAddFrame();
}

static void ParseMap(const char *info, IntermissionMapPositionInfo *mp)
{
    const char *p = strchr(info, ':');
    if (!p || p == info)
        DDFError("Bad map def: '%s' (missing level name)\n", info);

    mp->name_ = std::string(info, p - info);

    p++;

    if (sscanf(p, " %d : %d ", &mp->x_, &mp->y_) != 2)
        DDFError("Bad map definition: '%s'\n", info);
}

static void DDFGameGetMap(const char *info, void *storage)
{
    EPI_UNUSED(storage);
    IntermissionMapPositionInfo *mp = new IntermissionMapPositionInfo();

    ParseMap(info, mp);

    dynamic_gamedef->mappos_.push_back(mp);
}

static void DDFGameGetPic(const char *info, void *storage)
{
    EPI_UNUSED(storage);
    dynamic_gamedef->titlepics_.push_back(info);
}

static DDFSpecialFlags lighting_names[] = {{"DOOM", kLightingModelDoom, 0},
                                           {"DOOMISH", kLightingModelDoomish, 0},
                                           {"FLAT", kLightingModelFlat, 0},
                                           {"VERTEX", kLightingModelVertex, 0},
                                           {nullptr, 0, 0}};

void DDFGameGetLighting(const char *info, void *storage)
{
    int flag_value;

    if (kDDFCheckFlagPositive != DDFMainCheckSpecialFlag(info, lighting_names, &flag_value, false, false))
    {
        DDFWarnError("GAMES.DDF LIGHTING: Unknown model: %s", info);
        return;
    }

    ((LightingModel *)storage)[0] = (LightingModel)flag_value;
}

// --> world intermission mappos class

//
// wi_mapposdef_c Constructor
//
IntermissionMapPositionInfo::IntermissionMapPositionInfo()
{
}

//
// wi_mapposdef_c Copy constructor
//
IntermissionMapPositionInfo::IntermissionMapPositionInfo(const IntermissionMapPositionInfo &rhs)
{
    Copy(rhs);
}

//
// wi_mapposdef_c Destructor
//
IntermissionMapPositionInfo::~IntermissionMapPositionInfo()
{
}

//
// wi_mapposdef_c::Copy()
//
void IntermissionMapPositionInfo::Copy(const IntermissionMapPositionInfo &src)
{
    name_ = src.name_;
    x_    = src.x_;
    y_    = src.y_;
}

//
// wi_mapposdef_c assignment operator
//
IntermissionMapPositionInfo &IntermissionMapPositionInfo::operator=(const IntermissionMapPositionInfo &rhs)
{
    if (&rhs != this)
        Copy(rhs);

    return *this;
}

// --> world intermission mappos container class

//
// wi_mapposdef_container_c Constructor
//
IntermissionMapPositionInfoContainer::IntermissionMapPositionInfoContainer()
{
}

//
// wi_mapposdef_container_c Copy constructor
//
IntermissionMapPositionInfoContainer::IntermissionMapPositionInfoContainer(
    const IntermissionMapPositionInfoContainer &rhs)
    : std::vector<IntermissionMapPositionInfo *>()
{
    Copy(rhs);
}

//
// wi_mapposdef_container_c Destructor
//
IntermissionMapPositionInfoContainer::~IntermissionMapPositionInfoContainer()
{
    for (std::vector<IntermissionMapPositionInfo *>::iterator iter = begin(), iter_end = end(); iter != iter_end;
         iter++)
    {
        IntermissionMapPositionInfo *wi = *iter;
        delete wi;
        wi = nullptr;
    }
}

//
// wi_mapposdef_container_c::Copy()
//
void IntermissionMapPositionInfoContainer::Copy(const IntermissionMapPositionInfoContainer &src)
{
    for (IntermissionMapPositionInfo *wi : src)
    {
        if (wi)
        {
            IntermissionMapPositionInfo *wi2 = new IntermissionMapPositionInfo(*wi);
            push_back(wi2);
        }
    }
}

//
// wi_mapposdef_container_c assignment operator
//
IntermissionMapPositionInfoContainer &IntermissionMapPositionInfoContainer::operator=(
    const IntermissionMapPositionInfoContainer &rhs)
{
    if (&rhs != this)
    {
        for (std::vector<IntermissionMapPositionInfo *>::iterator iter = begin(), iter_end = end(); iter != iter_end;
             iter++)
        {
            IntermissionMapPositionInfo *wi = *iter;
            delete wi;
            wi = nullptr;
        }
        clear();
        Copy(rhs);
    }

    return *this;
}

// --> world intermission framedef class

//
// wi_framedef_c Constructor
//
IntermissionFrameInfo::IntermissionFrameInfo()
{
    Default();
}

//
// wi_framedef_c Copy constructor
//
IntermissionFrameInfo::IntermissionFrameInfo(const IntermissionFrameInfo &rhs)
{
    Copy(rhs);
}

//
// wi_framedef_c Destructor
//
IntermissionFrameInfo::~IntermissionFrameInfo()
{
}

//
// wi_framedef_c::Copy()
//
void IntermissionFrameInfo::Copy(const IntermissionFrameInfo &src)
{
    pic_  = src.pic_;
    tics_ = src.tics_;
    x_    = src.x_;
    y_    = src.y_;
}

//
// wi_framedef_c::Default()
//
void IntermissionFrameInfo::Default()
{
    pic_.clear();
    tics_ = 0;
    x_ = y_ = 0;
}

//
// wi_framedef_c assignment operator
//
IntermissionFrameInfo &IntermissionFrameInfo::operator=(const IntermissionFrameInfo &rhs)
{
    if (&rhs != this)
        Copy(rhs);

    return *this;
}

// --> world intermission framedef container class

//
// wi_framedef_container_c Constructor
//
IntermissionFrameInfoContainer::IntermissionFrameInfoContainer()
{
}

//
// wi_framedef_container_c Copy constructor
//
IntermissionFrameInfoContainer::IntermissionFrameInfoContainer(const IntermissionFrameInfoContainer &rhs)
    : std::vector<IntermissionFrameInfo *>()
{
    Copy(rhs);
}

//
// wi_framedef_container_c Destructor
//
IntermissionFrameInfoContainer::~IntermissionFrameInfoContainer()
{
    for (std::vector<IntermissionFrameInfo *>::iterator iter = begin(), iter_end = end(); iter != iter_end; iter++)
    {
        IntermissionFrameInfo *wi = *iter;
        delete wi;
        wi = nullptr;
    }
}

//
// wi_framedef_container_c::Copy()
//
void IntermissionFrameInfoContainer::Copy(const IntermissionFrameInfoContainer &src)
{
    for (IntermissionFrameInfo *f : src)
    {
        if (f)
        {
            IntermissionFrameInfo *f2 = new IntermissionFrameInfo(*f);
            push_back(f2);
        }
    }
}

//
// wi_framedef_container_c assignment operator
//
IntermissionFrameInfoContainer &IntermissionFrameInfoContainer::operator=(const IntermissionFrameInfoContainer &rhs)
{
    if (&rhs != this)
    {
        for (std::vector<IntermissionFrameInfo *>::iterator iter = begin(), iter_end = end(); iter != iter_end; iter++)
        {
            IntermissionFrameInfo *wi = *iter;
            delete wi;
            wi = nullptr;
        }
        clear();
        Copy(rhs);
    }

    return *this;
}

// --> world intermission animdef class

//
// wi_animdef_c Constructor
//
IntermissionAnimationInfo::IntermissionAnimationInfo()
{
    Default();
}

//
// wi_animdef_c Copy constructor
//
IntermissionAnimationInfo::IntermissionAnimationInfo(const IntermissionAnimationInfo &rhs)
{
    Copy(rhs);
}

//
// wi_animdef_c Destructor
//
IntermissionAnimationInfo::~IntermissionAnimationInfo()
{
}

//
// void Copy()
//
void IntermissionAnimationInfo::Copy(const IntermissionAnimationInfo &src)
{
    type_   = src.type_;
    level_  = src.level_;
    frames_ = src.frames_;
}

//
// wi_animdef_c::Default()
//
void IntermissionAnimationInfo::Default()
{
    type_ = kIntermissionAnimationInfoNormal;
    level_.clear();

    for (IntermissionFrameInfo *frame : frames_)
    {
        delete frame;
        frame = nullptr;
    }
    frames_.clear();
}

//
// wi_animdef_c assignment operator
//
IntermissionAnimationInfo &IntermissionAnimationInfo::operator=(const IntermissionAnimationInfo &rhs)
{
    if (&rhs != this)
        Copy(rhs);

    return *this;
}

// --> world intermission anim container class

//
// wi_animdef_container_c Constructor
//
IntermissionAnimationInfoContainer::IntermissionAnimationInfoContainer()
{
}

//
// wi_animdef_container_c Copy constructor
//
IntermissionAnimationInfoContainer::IntermissionAnimationInfoContainer(const IntermissionAnimationInfoContainer &rhs)
    : std::vector<IntermissionAnimationInfo *>()
{
    Copy(rhs);
}

//
// wi_animdef_container_c Destructor
//
IntermissionAnimationInfoContainer::~IntermissionAnimationInfoContainer()
{
    for (std::vector<IntermissionAnimationInfo *>::iterator iter = begin(), iter_end = end(); iter != iter_end; iter++)
    {
        IntermissionAnimationInfo *wi = *iter;
        delete wi;
        wi = nullptr;
    }
}

//
// wi_animdef_container_c::Copy()
//
void IntermissionAnimationInfoContainer::Copy(const IntermissionAnimationInfoContainer &src)
{
    for (IntermissionAnimationInfo *a : src)
    {
        if (a)
        {
            IntermissionAnimationInfo *a2 = new IntermissionAnimationInfo(*a);
            push_back(a2);
        }
    }
}

//
// wi_animdef_container_c assignment operator
//
IntermissionAnimationInfoContainer &IntermissionAnimationInfoContainer::operator=(
    const IntermissionAnimationInfoContainer &rhs)
{
    if (&rhs != this)
    {
        for (std::vector<IntermissionAnimationInfo *>::iterator iter = begin(), iter_end = end(); iter != iter_end;
             iter++)
        {
            IntermissionAnimationInfo *wi = *iter;
            delete wi;
            wi = nullptr;
        }
        clear();
        Copy(rhs);
    }

    return *this;
}

// --> game definition class

//
// gamedef_c Constructor
//
GameDefinition::GameDefinition() : name_(), titlepics_()
{
    Default();
}

//
// gamedef_c Destructor
//
GameDefinition::~GameDefinition()
{
}

//
// gamedef_c::CopyDetail()
//
void GameDefinition::CopyDetail(const GameDefinition &src)
{
    anims_  = src.anims_;
    mappos_ = src.mappos_;

    background_ = src.background_;
    splatpic_   = src.splatpic_;

    you_are_here_[0] = src.you_are_here_[0];
    you_are_here_[1] = src.you_are_here_[1];

    bg_camera_ = src.bg_camera_;
    music_     = src.music_;

    percent_       = src.percent_;
    done_          = src.done_;
    endmap_        = src.endmap_;
    next_map_      = src.next_map_;
    accel_snd_     = src.accel_snd_;
    frag_snd_      = src.frag_snd_;
    no_skill_menu_ = src.no_skill_menu_;

    firstmap_    = src.firstmap_;
    namegraphic_ = src.namegraphic_;

    titlepics_  = src.titlepics_;
    titlemovie_ = src.titlemovie_;
    titlemusic_ = src.titlemusic_;
    titletics_  = src.titletics_;

    special_music_        = src.special_music_;
    lighting_             = src.lighting_;
    description_          = src.description_;
    default_damage_flash_ = src.default_damage_flash_;
}

//
// gamedef_c::Default()
//
void GameDefinition::Default()
{
    for (IntermissionAnimationInfo *a : anims_)
    {
        delete a;
        a = nullptr;
    }
    anims_.clear();
    for (IntermissionMapPositionInfo *m : mappos_)
    {
        delete m;
        m = nullptr;
    }
    mappos_.clear();

    background_.clear();
    splatpic_.clear();

    you_are_here_[0].clear();
    you_are_here_[1].clear();

    bg_camera_.clear();
    music_         = 0;
    no_skill_menu_ = false;

    percent_   = nullptr;
    done_      = nullptr;
    endmap_    = nullptr;
    next_map_  = nullptr;
    accel_snd_ = nullptr;
    frag_snd_  = nullptr;

    firstmap_.clear();
    namegraphic_.clear();

    titlepics_.clear();
    titlemovie_.clear();
    movie_played_ = false;
    titlemusic_   = 0;
    titletics_    = kTicRate * 4;

    special_music_ = 0;
    lighting_      = kLightingModelDoomish;
    description_.clear();
    default_damage_flash_ = kRGBARed;
}

// --> game definition container class

//
// gamedef_container_c Constructor
//
GameDefinitionContainer::GameDefinitionContainer()
{
}

//
// gamedef_container_c Destructor
//
GameDefinitionContainer::~GameDefinitionContainer()
{
    for (std::vector<GameDefinition *>::iterator iter = begin(), iter_end = end(); iter != iter_end; iter++)
    {
        GameDefinition *game = *iter;
        delete game;
        game = nullptr;
    }
}

//
// gamedef_c* gamedef_container_c::Lookup()
//
// Looks an gamedef by name, returns a fatal error if it does not exist.
//
GameDefinition *GameDefinitionContainer::Lookup(const char *refname)
{
    if (!refname || !refname[0])
        return nullptr;

    for (std::vector<GameDefinition *>::iterator iter = begin(), iter_end = end(); iter != iter_end; iter++)
    {
        GameDefinition *game = *iter;
        if (DDFCompareName(game->name_.c_str(), refname) == 0)
            return game;
    }

    return nullptr;
}

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
