#pragma once

#include <stdint.h>
#include <vector>

#include "epi_file.h"
#include "p_mobj.h"
#include "r_defs.h"

struct ModelFrame
{
    float      *xs;
    float      *ys;
    float      *zs;
    short      *normal_indices;
    const char *name;
    short      *used_normals;
};

struct ModelPoint
{
    float skin_s, skin_t;
    int   vert_idx;
};

class ModelData
{
  public:
    int total_frames_;
    int total_points_;
    int total_triangles_;
    int vertices_per_frame_;

    ModelFrame *frames_;
    ModelPoint *points_;

    std::vector<uint32_t> skin_id_list_;

  public:
    ModelData(int nframes, int npoints, int ntris, int nverts);
    ~ModelData();
};

ModelData *ModelLoad(epi::File *f, float &radius);

short ModelFindFrame(ModelData *md, const char *name);

void RenderModel(ModelData *md, const Image *skin_img, bool is_weapon, int frame1, int frame2, float lerp, float x,
                 float y, float z, MapObject *mo, RegionProperties *props, float scale, float aspect, float bias,
                 int rotation);

void RenderModel2D(ModelData *md, const Image *skin_img, int frame, float x, float y, float xscale, float yscale,
                   const MapObjectDefinition *info);

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
