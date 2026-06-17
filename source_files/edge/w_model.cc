#include "w_model.h"

#include "e_main.h"
#include "epi.h"
#include "epi_str_compare.h"
#include "epi_str_util.h"
#include "p_local.h"
#include "r_image.h"
#include "r_model.h"
#include "r_things.h"
#include "w_files.h"
#include "w_wad.h"

extern ConsoleVariable precache_all_models;

static ModelDefinition **models;
static int               total_models = 0;

ModelDefinition::ModelDefinition(const char *prefix) : radius_(1.0f), model_(nullptr)
{
    strcpy(name_, prefix);

    for (int i = 0; i < kMaximumModelSkins; i++)
        skins_[i] = nullptr;
}

ModelDefinition::~ModelDefinition()
{
}

static void FindModelFrameNames(ModelData *md, int model_num)
{
    int missing = 0;

    LogDebug("Finding frame names for model '%s'...\n", ddf_model_names[model_num].c_str());

    for (int stnum = 1; stnum < num_states; stnum++)
    {
        State *st = &states[stnum];

        if (st->sprite != model_num)
            continue;

        if (!(st->flags & kStateFrameFlagModel))
            continue;

        if (!(st->flags & kStateFrameFlagUnmapped))
            continue;

        EPI_ASSERT(st->model_frame);

        st->frame = ModelFindFrame(md, st->model_frame);

        if (st->frame >= 0)
        {
            st->flags &= ~kStateFrameFlagUnmapped;
        }
        else
        {
            missing++;
            LogPrint("-- no such frame '%s'\n", st->model_frame);
        }
    }

    if (missing > 0)
        FatalError("Failed to find %d frames for model '%s' (see EDGE.LOG)\n", missing,
                   ddf_model_names[model_num].c_str());
}

ModelDefinition *LoadModelFromLump(int model_num)
{
    std::string basename = ddf_model_names[model_num];

    ModelDefinition *def = new ModelDefinition(basename.c_str());

    std::string lumpname;
    std::string packname;
    std::string skinname;

    int        lump_num  = -1;
    int        pack_num  = -1;
    epi::File *f         = nullptr;
    bool       pack_file = false;

    lumpname = epi::StringFormat("%sMD3", basename.c_str());
    lump_num = CheckDataFileIndexForName(lumpname.c_str());
    packname = epi::StringFormat("%s.md3", basename.c_str());
    pack_num = CheckPackFilesForName(packname);
    if (pack_num == -1)
    {
        packname = epi::StringFormat("%sMD3.md3", basename.c_str());
        pack_num = CheckPackFilesForName(packname);
    }

    if (lump_num > -1 || pack_num > -1)
    {
        if (pack_num > lump_num)
        {
            f = OpenFileFromPack(packname);
            if (f)
            {
                LogDebug("Loading MD3 model from pack file : %s\n", packname.c_str());
                def->model_ = ModelLoad(f, def->radius_);
                pack_file   = true;
            }
        }
        else
        {
            LogDebug("Loading MD3 model from lump : %s\n", lumpname.c_str());
            f = LoadLumpAsFile(lumpname.c_str());
            if (f)
                def->model_ = ModelLoad(f, def->radius_);
        }
    }

    if (!f)
    {
        lumpname = epi::StringFormat("%sMD2", basename.c_str());
        lump_num = CheckDataFileIndexForName(lumpname.c_str());
        packname = epi::StringFormat("%s.md2", basename.c_str());
        pack_num = CheckPackFilesForName(packname);
        if (pack_num == -1)
        {
            packname = epi::StringFormat("%sMD2.md2", basename.c_str());
            pack_num = CheckPackFilesForName(packname);
        }
        if (lump_num > -1 || pack_num > -1)
        {
            if (pack_num > lump_num)
            {
                f = OpenFileFromPack(packname);
                if (f)
                {
                    LogDebug("Loading MD2 model from pack file : %s\n", packname.c_str());
                    def->model_ = ModelLoad(f, def->radius_);
                    pack_file   = true;
                }
            }
            else
            {
                LogDebug("Loading MD2 model from lump : %s\n", lumpname.c_str());
                f = LoadLumpAsFile(lumpname.c_str());
                if (f)
                    def->model_ = ModelLoad(f, def->radius_);
            }
        }
    }

    if (!f)
    {
        lumpname = epi::StringFormat("%sMDL", basename.c_str());
        lump_num = CheckDataFileIndexForName(lumpname.c_str());
        packname = epi::StringFormat("%s.mdl", basename.c_str());
        pack_num = CheckPackFilesForName(packname);
        if (pack_num == -1)
        {
            packname = epi::StringFormat("%sMDL.mdl", basename.c_str());
            pack_num = CheckPackFilesForName(packname);
        }
        if (lump_num > -1 || pack_num > -1)
        {
            if (pack_num > lump_num)
            {
                f = OpenFileFromPack(packname);
                if (f)
                {
                    LogDebug("Loading MDL model from pack file : %s\n", packname.c_str());
                    def->model_ = ModelLoad(f, def->radius_);
                    pack_file   = true;
                }
            }
            else
            {
                LogDebug("Loading MDL model from lump : %s\n", lumpname.c_str());
                f = LoadLumpAsFile(lumpname.c_str());
                if (f)
                    def->model_ = ModelLoad(f, def->radius_);
            }
        }
    }

    if (!f)
        FatalError("Missing model lump for: %s\n!", basename.c_str());

    EPI_ASSERT(def->model_);

    delete f;

    if (def->model_->skin_id_list_.empty())
    {
        for (int i = 0; i < 10; i++)
        {
            if (pack_file)
            {
                skinname       = epi::StringFormat("%s%d", basename.c_str(), i);
                def->skins_[i] = ImageLookup(skinname.c_str(), kImageNamespaceSprite, kImageLookupNull);
                if (!def->skins_[i])
                {
                    skinname       = epi::StringFormat("%sSKN%d", basename.c_str(), i);
                    def->skins_[i] = ImageLookup(skinname.c_str(), kImageNamespaceSprite, kImageLookupNull);
                }
            }
            else
            {
                skinname       = epi::StringFormat("%sSKN%d", basename.c_str(), i);
                def->skins_[i] = ImageLookup(skinname.c_str(), kImageNamespaceSprite, kImageLookupNull);
            }
        }

        if (!def->skins_[1])
        {
            if (pack_file)
                FatalError("Missing model skin: %s1\n", basename.c_str());
            else
                FatalError("Missing model skin: %sSKN1\n", basename.c_str());
        }
    }

    FindModelFrameNames(def->model_, model_num);

    return def;
}

void InitializeModels(void)
{
    total_models = (int)ddf_model_names.size();

    EPI_ASSERT(total_models >= 1);

    StartupProgressMessage("Setting up models...");

    LogPrint("InitializeModels: Setting up\n");

    models = new ModelDefinition *[total_models];

    if (precache_all_models.d_)
    {
        models[0] = nullptr;

        for (int i = 1; i < total_models; i++)
        {
            models[i] = LoadModelFromLump(i);
            for (int n = 0; n < 10; n++)
            {
                if (models[i] && models[i]->skins_[n])
                    ImagePrecache(models[i]->skins_[n]);
            }
        }
    }
    else
    {
        for (int i = 0; i < total_models; i++)
            models[i] = nullptr;
    }
}

ModelDefinition *GetModel(int model_num)
{
    EPI_ASSERT(model_num > 0);
    EPI_ASSERT(model_num < total_models);

    if (!models[model_num])
        models[model_num] = LoadModelFromLump(model_num);

    return models[model_num];
}

void PrecacheModels(void)
{
    if (total_models <= 0)
        return;

    uint8_t *model_present = new uint8_t[total_models];
    EPI_CLEAR_MEMORY(model_present, uint8_t, total_models);

    for (MapObject *mo = map_object_list_head; mo; mo = mo->next_)
    {
        EPI_ASSERT(mo->state_);

        if (!(mo->state_->flags & kStateFrameFlagModel))
            continue;

        int model = mo->state_->sprite;

        const char *model_name = nullptr;

        if (model >= 1 && model < total_models)
            model_name = ddf_model_names[model].c_str();

        if (model_name)
        {
            for (int i = 1; i < total_models; i++)
            {
                if (epi::StringCaseCompareMaxASCII(model_name, ddf_model_names[i], 4) == 0)
                    model_present[i] = 1;
            }
        }
    }

    for (int k = 1; k < num_states; k++)
    {
        if ((states[k].flags & (kStateFrameFlagWeapon | kStateFrameFlagModel)) !=
            (kStateFrameFlagWeapon | kStateFrameFlagModel))
            continue;

        int model = states[k].sprite;

        const char *model_name = nullptr;

        if (model >= 1 && model < total_models)
            model_name = ddf_model_names[model].c_str();

        if (model_name)
        {
            for (int i = 1; i < total_models; i++)
            {
                if (epi::StringCaseCompareMaxASCII(model_name, ddf_model_names[i], 4) == 0)
                    model_present[i] = 1;
            }
        }
    }

    for (int i = 1; i < total_models; i++)
    {
        if (model_present[i])
        {
            LogDebug("Precaching model: %s\n", ddf_model_names[i].c_str());

            const ModelDefinition *def = GetModel(i);

            for (int n = 0; n < 10; n++)
            {
                if (def && def->skins_[n])
                    ImagePrecache(def->skins_[n]);
            }
        }
    }

    delete[] model_present;
}

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
