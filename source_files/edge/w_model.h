#pragma once

#include "r_defs.h"
#include "r_model.h"

constexpr uint8_t kMaximumModelSkins = 10;

class ModelDefinition
{
  public:
    char name_[6];

    float radius_;

    ModelData *model_;

    const Image *skins_[kMaximumModelSkins];

  public:
    ModelDefinition(const char *prefix);
    ~ModelDefinition();
};

void InitializeModels(void);

void PrecacheModels(void);

ModelDefinition *GetModel(int model_num);

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
