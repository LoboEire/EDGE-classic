#include <stddef.h>

#include <unordered_map>

#include "ddf_types.h"
#include "dm_state.h"
#include "epi.h"
#include "epi_simd.h"
#include "g_game.h"
#include "i_defs_gl.h"
#include "n_network.h"
#include "p_blockmap.h"
#include "p_tick.h"
#include "r_backend.h"
#include "r_colormap.h"
#include "r_effects.h"
#include "r_gldefs.h"
#include "r_image.h"
#include "r_mdcommon.h"
#include "r_mirror.h"
#include "r_misc.h"
#include "r_modes.h"
#include "r_model.h"
#include "r_shader.h"
#include "r_state.h"
#include "r_units.h"
#include "sokol_images.h"
#include "sokol_local.h"
#include "sokol_pipeline.h"

extern std::unordered_map<GLuint, GLint> texture_clamp_t;

extern float ApproximateDistance(float dx, float dy, float dz);

extern ConsoleVariable fliplevels;
extern ConsoleVariable draw_culling;
extern ConsoleVariable cull_fog_color;
extern bool            need_to_draw_sky;

static HMM_Vec3  render_position;
static RGBAColor render_rgba;
static HMM_Vec2  render_texture_coordinates;

static float *pos_cache_x   = nullptr;
static float *pos_cache_y   = nullptr;
static float *pos_cache_z   = nullptr;
static int    pos_cache_cap = 0;

static void EnsurePosCache(int nverts)
{
    if (nverts <= pos_cache_cap)
        return;
    delete[] pos_cache_x;
    delete[] pos_cache_y;
    delete[] pos_cache_z;
    pos_cache_x   = new float[nverts];
    pos_cache_y   = new float[nverts];
    pos_cache_z   = new float[nverts];
    pos_cache_cap = nverts;
}

class CoordinateData
{
  public:
    MapObject *map_object_;

    ModelData *model_;

    float x_, y_, z_;

    bool is_weapon;
    bool is_fuzzy_;

    float xy_scale_;
    float z_scale_;

    float    fuzz_multiplier_;
    HMM_Vec2 fuzz_add_;

    HMM_Vec2 mouselook_x_matrix_;
    HMM_Vec2 mouselook_z_matrix_;

    HMM_Vec2 rotation_x_matrix_;
    HMM_Vec2 rotation_y_matrix_;

    ColorMixer normal_colors_[kTotalMDFormatNormals];

    const short *active_normals_;
    short       *used_normals_;

    bool is_additive_;
};

static void BuildPosCache(const CoordinateData &cd, const ModelFrame *frame1, const ModelFrame *frame2, int nverts,
                          float lerp, float bias, bool reflective)
{
    EnsurePosCache(nverts);

    float xy_sc = cd.xy_scale_;
    float z_sc  = cd.z_scale_;
    float ox    = cd.x_;
    float oy    = cd.y_;
    float oz    = cd.z_;
    float mlX_X = cd.mouselook_x_matrix_.X;
    float mlX_Y = cd.mouselook_x_matrix_.Y;
    float mlZ_X = cd.mouselook_z_matrix_.X;
    float mlZ_Y = cd.mouselook_z_matrix_.Y;
    float rX_X  = cd.rotation_x_matrix_.X;
    float rX_Y  = cd.rotation_x_matrix_.Y;
    float rY_X  = cd.rotation_y_matrix_.X;
    float rY_Y  = cd.rotation_y_matrix_.Y;
    float refl  = reflective ? -1.0f : 1.0f;

    epi::SimdF32x4 v_lerp  = epi::SplatF32x4(lerp);
    epi::SimdF32x4 v_bias  = epi::SplatF32x4(bias);
    epi::SimdF32x4 v_xy_sc = epi::SplatF32x4(xy_sc);
    epi::SimdF32x4 v_z_sc  = epi::SplatF32x4(z_sc);
    epi::SimdF32x4 v_refl  = epi::SplatF32x4(refl);
    epi::SimdF32x4 v_mlX_X = epi::SplatF32x4(mlX_X);
    epi::SimdF32x4 v_mlX_Y = epi::SplatF32x4(mlX_Y);
    epi::SimdF32x4 v_mlZ_X = epi::SplatF32x4(mlZ_X);
    epi::SimdF32x4 v_mlZ_Y = epi::SplatF32x4(mlZ_Y);
    epi::SimdF32x4 v_rX_X  = epi::SplatF32x4(rX_X);
    epi::SimdF32x4 v_rX_Y  = epi::SplatF32x4(rX_Y);
    epi::SimdF32x4 v_rY_X  = epi::SplatF32x4(rY_X);
    epi::SimdF32x4 v_rY_Y  = epi::SplatF32x4(rY_Y);
    epi::SimdF32x4 v_ox    = epi::SplatF32x4(ox);
    epi::SimdF32x4 v_oy    = epi::SplatF32x4(oy);
    epi::SimdF32x4 v_oz    = epi::SplatF32x4(oz);

    int v = 0;
    for (; v + 3 < nverts; v += 4)
    {
        epi::SimdF32x4 f1x = epi::LoadF32x4(frame1->xs + v);
        epi::SimdF32x4 f2x = epi::LoadF32x4(frame2->xs + v);
        epi::SimdF32x4 f1y = epi::LoadF32x4(frame1->ys + v);
        epi::SimdF32x4 f2y = epi::LoadF32x4(frame2->ys + v);
        epi::SimdF32x4 f1z = epi::LoadF32x4(frame1->zs + v);
        epi::SimdF32x4 f2z = epi::LoadF32x4(frame2->zs + v);

        epi::SimdF32x4 lx = epi::AddF32x4(f1x, epi::MulF32x4(v_lerp, epi::SubF32x4(f2x, f1x)));
        epi::SimdF32x4 ly = epi::MulF32x4(epi::AddF32x4(f1y, epi::MulF32x4(v_lerp, epi::SubF32x4(f2y, f1y))), v_refl);
        epi::SimdF32x4 lz = epi::AddF32x4(epi::AddF32x4(f1z, epi::MulF32x4(v_lerp, epi::SubF32x4(f2z, f1z))), v_bias);

        lx = epi::MulF32x4(lx, v_xy_sc);
        ly = epi::MulF32x4(ly, v_xy_sc);
        lz = epi::MulF32x4(lz, v_z_sc);

        epi::SimdF32x4 tx = epi::AddF32x4(epi::MulF32x4(lx, v_mlX_X), epi::MulF32x4(lz, v_mlX_Y));
        epi::SimdF32x4 tz = epi::AddF32x4(epi::MulF32x4(lx, v_mlZ_X), epi::MulF32x4(lz, v_mlZ_Y));

        epi::SimdF32x4 px = epi::AddF32x4(v_ox, epi::AddF32x4(epi::MulF32x4(tx, v_rX_X), epi::MulF32x4(ly, v_rX_Y)));
        epi::SimdF32x4 py = epi::AddF32x4(v_oy, epi::AddF32x4(epi::MulF32x4(tx, v_rY_X), epi::MulF32x4(ly, v_rY_Y)));
        epi::SimdF32x4 pz = epi::AddF32x4(v_oz, tz);

        epi::StoreF32x4(pos_cache_x + v, px);
        epi::StoreF32x4(pos_cache_y + v, py);
        epi::StoreF32x4(pos_cache_z + v, pz);
    }

    for (; v < nverts; v++)
    {
        float lx = frame1->xs[v] + lerp * (frame2->xs[v] - frame1->xs[v]);
        float ly = (frame1->ys[v] + lerp * (frame2->ys[v] - frame1->ys[v])) * refl;
        float lz = frame1->zs[v] + lerp * (frame2->zs[v] - frame1->zs[v]) + bias;

        lx *= xy_sc;
        ly *= xy_sc;
        lz *= z_sc;

        float tx = lx * mlX_X + lz * mlX_Y;
        float tz = lx * mlZ_X + lz * mlZ_Y;

        pos_cache_x[v] = ox + tx * rX_X + ly * rX_Y;
        pos_cache_y[v] = oy + tx * rY_X + ly * rY_Y;
        pos_cache_z[v] = oz + tz;
    }
}

static void InitNormalColors(CoordinateData *data)
{
    short *n_list = data->used_normals_;

    for (; *n_list >= 0; n_list++)
        data->normal_colors_[*n_list].Clear();
}

static void ShadeNormals(AbstractShader *shader, CoordinateData *data, bool skip_calc)
{
    short *n_list = data->used_normals_;

    for (; *n_list >= 0; n_list++)
    {
        short n  = *n_list;
        float nx = 0;
        float ny = 0;
        float nz = 0;

        if (!skip_calc)
        {
            float nx1 = md_normals[n].X;
            float ny1 = md_normals[n].Y;
            float nz1 = md_normals[n].Z;

            float nx2 = nx1 * data->mouselook_x_matrix_.X + nz1 * data->mouselook_x_matrix_.Y;
            float nz2 = nx1 * data->mouselook_z_matrix_.X + nz1 * data->mouselook_z_matrix_.Y;
            float ny2 = ny1;

            nx = nx2 * data->rotation_x_matrix_.X + ny2 * data->rotation_x_matrix_.Y;
            ny = nx2 * data->rotation_y_matrix_.X + ny2 * data->rotation_y_matrix_.Y;
            nz = nz2;
        }

        shader->Corner(data->normal_colors_ + n, nx, ny, nz, data->map_object_, data->is_weapon);
    }
}

static void DynamicLightCallback(MapObject *mo, void *dataptr)
{
    CoordinateData *data = (CoordinateData *)dataptr;

    if (mo == data->map_object_)
        return;

    EPI_ASSERT(mo->dynamic_light_.shader);

    ShadeNormals(mo->dynamic_light_.shader, data, false);
}

static int MulticolMaxRGB(CoordinateData *data, bool additive)
{
    int result = 0;

    short *n_list = data->used_normals_;

    for (; *n_list >= 0; n_list++)
    {
        ColorMixer *col = &data->normal_colors_[*n_list];

        int mx = additive ? col->add_MAX() : col->mod_MAX();

        result = HMM_MAX(result, mx);
    }

    return result;
}

static void UpdateMulticols(CoordinateData *data)
{
    short *n_list = data->used_normals_;

    for (; *n_list >= 0; n_list++)
    {
        ColorMixer *col = &data->normal_colors_[*n_list];

        col->modulate_red_ -= 256;
        col->modulate_green_ -= 256;
        col->modulate_blue_ -= 256;
    }
}

static inline void ModelCoordFunc(CoordinateData *data, int point_idx)
{
    const ModelData  *md    = data->model_;
    const ModelPoint *point = &md->points_[point_idx];

    render_position = {{pos_cache_x[point->vert_idx], pos_cache_y[point->vert_idx], pos_cache_z[point->vert_idx]}};

    if (data->is_fuzzy_)
    {
        render_texture_coordinates.X = point->skin_s * data->fuzz_multiplier_ + data->fuzz_add_.X;
        render_texture_coordinates.Y = point->skin_t * data->fuzz_multiplier_ + data->fuzz_add_.Y;

        render_rgba = kRGBABlack;
        return;
    }

    render_texture_coordinates = {{point->skin_s, point->skin_t}};

    ColorMixer *col = &data->normal_colors_[data->active_normals_[point->vert_idx]];

    if (!data->is_additive_)
    {
        render_rgba = epi::MakeRGBAClamped(col->modulate_red_ * render_view_red_multiplier,
                                           col->modulate_green_ * render_view_green_multiplier,
                                           col->modulate_blue_ * render_view_blue_multiplier);
    }
    else
    {
        render_rgba = epi::MakeRGBAClamped(col->add_red_ * render_view_red_multiplier,
                                           col->add_green_ * render_view_green_multiplier,
                                           col->add_blue_ * render_view_blue_multiplier);
    }
}

void RenderModel(ModelData *md, const Image *skin_img, bool is_weapon, int frame1, int frame2, float lerp, float x,
                 float y, float z, MapObject *mo, RegionProperties *props, float scale, float aspect, float bias,
                 int rotation)
{
    if (frame1 < 0 || frame1 >= md->total_frames_)
    {
        LogDebug("Render model: bad frame %d\n", frame1);
        return;
    }
    if (frame2 < 0 || frame2 >= md->total_frames_)
    {
        LogDebug("Render model: bad frame %d\n", frame2);
        return;
    }

    CoordinateData data;

    data.is_fuzzy_ = (mo->flags_ & kMapObjectFlagFuzzy) ? true : false;

    float trans = mo->visibility_;

    if (is_weapon && data.is_fuzzy_ && mo->player_ && mo->player_->powers_[kPowerTypePartInvisTranslucent] > 0)
    {
        data.is_fuzzy_ = false;
        trans *= 0.3f;
    }

    if (trans <= 0)
        return;

    BlendingMode blending;

    if (skin_img != nullptr)
    {
        if (trans >= 0.99f && skin_img->opacity_ == kOpacitySolid)
            blending = kBlendingNone;
        else if (trans < 0.11f || skin_img->opacity_ == kOpacityComplex)
            blending = kBlendingMasked;
        else
            blending = kBlendingLess;

        if (trans < 0.99f || skin_img->opacity_ == kOpacityComplex)
            blending = (BlendingMode)(blending | kBlendingAlpha);
    }
    else
    {
        blending = kBlendingNone;
    }

    if (mo->hyper_flags_ & kHyperFlagNoZBufferUpdate)
        blending = (BlendingMode)(blending | kBlendingNoZBuffer);

    if (render_mirror_set.Reflective())
    {
        if (fliplevels.d_)
            blending = (BlendingMode)(blending | kBlendingCullBack);
        else
            blending = (BlendingMode)(blending | kBlendingCullFront);
    }
    else
    {
        if (fliplevels.d_)
            blending = (BlendingMode)(blending | kBlendingCullFront);
        else
            blending = (BlendingMode)(blending | kBlendingCullBack);
    }

    const ModelFrame *frame1_ptr = &md->frames_[frame1];
    const ModelFrame *frame2_ptr = &md->frames_[frame2];

    data.map_object_ = mo;
    data.model_      = md;

    data.x_ = x;
    data.y_ = y;
    data.z_ = z;

    data.is_weapon = is_weapon;

    data.xy_scale_ = scale * aspect * render_mirror_set.XYScale();
    data.z_scale_  = scale * render_mirror_set.ZScale();

    bool tilt = is_weapon || (mo->flags_ & kMapObjectFlagMissile) || (mo->hyper_flags_ & kHyperFlagForceModelTilt);

    if (!console_active && !paused && !menu_active && !rts_menu_active &&
        (is_weapon || (!time_stop_active && !erraticism_active)))
    {
        BAMAngle ang;
        if (is_weapon)
        {
            BAMAngleToMatrix(tilt ? ~epi::BAMInterpolate(mo->old_vertical_angle_, mo->vertical_angle_, fractional_tic)
                                  : 0,
                             &data.mouselook_x_matrix_, &data.mouselook_z_matrix_);
            ang = epi::BAMInterpolate(mo->old_angle_, mo->angle_, fractional_tic) + rotation;
        }
        else
        {
            BAMAngleToMatrix(tilt ? ~mo->vertical_angle_ : 0, &data.mouselook_x_matrix_, &data.mouselook_z_matrix_);
            ang = mo->angle_ + rotation;
        }
        render_mirror_set.Angle(ang);
        BAMAngleToMatrix(~ang, &data.rotation_x_matrix_, &data.rotation_y_matrix_);
    }
    else
    {
        BAMAngleToMatrix(tilt ? ~mo->vertical_angle_ : 0, &data.mouselook_x_matrix_, &data.mouselook_z_matrix_);
        BAMAngle ang = mo->angle_ + rotation;
        render_mirror_set.Angle(ang);
        BAMAngleToMatrix(~ang, &data.rotation_x_matrix_, &data.rotation_y_matrix_);
    }

    data.used_normals_   = (lerp < 0.5f) ? frame1_ptr->used_normals : frame2_ptr->used_normals;
    data.active_normals_ = (lerp < 0.5f) ? frame1_ptr->normal_indices : frame2_ptr->normal_indices;

    BuildPosCache(data, frame1_ptr, frame2_ptr, md->vertices_per_frame_, lerp, bias, render_mirror_set.Reflective());

    InitNormalColors(&data);

    GLuint skin_tex = 0;

    if (data.is_fuzzy_)
    {
        skin_tex = ImageCache(fuzz_image, false);

        data.fuzz_multiplier_ = 0.8;
        data.fuzz_add_        = {{0, 0}};

        if (!data.is_weapon && !view_is_zoomed)
        {
            float dist = ApproximateDistance(mo->x - view_x, mo->y - view_y, mo->z - view_z);
            data.fuzz_multiplier_ = 70.0 / HMM_Clamp(35, dist, 700);
        }

        FuzzAdjust(&data.fuzz_add_, mo);

        trans = 1.0f;

        blending = (BlendingMode)(blending | (kBlendingAlpha | kBlendingMasked));
        blending = (BlendingMode)(blending & ~kBlendingLess);
    }
    else if (skin_img != nullptr)
    {
        skin_tex = ImageCache(skin_img, false,
                              render_view_effect_colormap ? render_view_effect_colormap
                              : is_weapon                 ? nullptr
                                                          : mo->info_->palremap_);

        AbstractShader *shader =
            GetColormapShader(props, mo->info_->force_fullbright_ ? 255 : mo->state_->bright, mo->subsector_->sector);
        ShadeNormals(shader, &data, true);

        if (use_dynamic_lights && render_view_extra_light < 250)
        {
            float r = mo->radius_;

            DynamicLightIterator(mo->x - r, mo->y - r, mo->z, mo->x + r, mo->y + r, mo->z + mo->height_,
                                 DynamicLightCallback, &data);

            SectorGlowIterator(mo->subsector_->sector, mo->x - r, mo->y - r, mo->z, mo->x + r, mo->y + r,
                               mo->z + mo->height_, DynamicLightCallback, &data);
        }
    }
    else
    {
        int mdl_skin = 0;

        if (is_weapon)
            mdl_skin = mo->player_->weapons_[mo->player_->ready_weapon_].model_skin;
        else
            mdl_skin = mo->model_skin_;

        mdl_skin--;

        if (mdl_skin > -1)
            skin_tex = md->skin_id_list_[mdl_skin];
        else
            skin_tex = md->skin_id_list_[0];

        if (skin_tex == 0)
            FatalError("Model frame %s missing skins?\n", frame1_ptr->name);

        AbstractShader *shader =
            GetColormapShader(props, mo->info_->force_fullbright_ ? 255 : mo->state_->bright, mo->subsector_->sector);
        ShadeNormals(shader, &data, true);

        if (use_dynamic_lights && render_view_extra_light < 250)
        {
            float r = mo->radius_;

            DynamicLightIterator(mo->x - r, mo->y - r, mo->z, mo->x + r, mo->y + r, mo->z + mo->height_,
                                 DynamicLightCallback, &data);

            SectorGlowIterator(mo->subsector_->sector, mo->x - r, mo->y - r, mo->z, mo->x + r, mo->y + r,
                               mo->z + mo->height_, DynamicLightCallback, &data);
        }
    }

    int num_pass = data.is_fuzzy_ ? 1 : (detail_level > 0 ? 4 : 3);

    RGBAColor fc_to_use = mo->subsector_->sector->properties.fog_color;
    float     fd_to_use = mo->subsector_->sector->properties.fog_density;

    if (fc_to_use == kRGBANoValue)
    {
        if (EDGE_IMAGE_IS_SKY(mo->subsector_->sector->ceiling))
        {
            fc_to_use = current_map->outdoor_fog_color_;
            fd_to_use = 0.01f * current_map->outdoor_fog_density_;
        }
        else
        {
            fc_to_use = current_map->indoor_fog_color_;
            fd_to_use = 0.01f * current_map->indoor_fog_density_;
        }
    }

    if (!draw_culling.d_ && fc_to_use != kRGBANoValue && !AlmostEquals(fd_to_use, 0.0f))
    {
        render_state->ClearColor(fc_to_use);
        render_state->FogColor(fc_to_use);
        render_state->FogMode(GL_EXP);
        render_state->FogDensity(std::log1p(fd_to_use));
        render_state->Enable(GL_FOG);
    }
    else if (draw_culling.d_)
    {
        RGBAColor fog_color;

        if (need_to_draw_sky)
        {
            switch (cull_fog_color.d_)
            {
            case 0:
                fog_color = culling_fog_color;
                break;
            case 1:
                fog_color = kRGBASilver;
                break;
            case 2:
                fog_color = 0x404040FF;
                break;
            case 3:
                fog_color = kRGBABlack;
                break;
            default:
                fog_color = culling_fog_color;
                break;
            }
        }
        else
        {
            fog_color = kRGBABlack;
        }

        render_state->ClearColor(fog_color);
        render_state->FogMode(GL_LINEAR);
        render_state->FogColor(fog_color);
        render_state->FogStart(renderer_far_clip.f_ - 750.0f);
        render_state->FogEnd(renderer_far_clip.f_ - 250.0f);
        render_state->Enable(GL_FOG);
    }
    else
    {
        render_state->Disable(GL_FOG);
    }

    for (int pass = 0; pass < num_pass; pass++)
    {
        render_backend->Flush(1, md->total_triangles_ * 3);

        if (pass == 1)
        {
            blending = (BlendingMode)(blending & ~kBlendingAlpha);
            blending = (BlendingMode)(blending | kBlendingAdd);
            render_state->Disable(GL_FOG);
        }

        data.is_additive_ = (pass > 0 && pass == num_pass - 1);

        if (pass > 0 && pass < num_pass - 1)
        {
            UpdateMulticols(&data);
            if (MulticolMaxRGB(&data, false) <= 0)
                continue;
        }
        else if (data.is_additive_)
        {
            if (MulticolMaxRGB(&data, true) <= 0)
                continue;
        }

        render_state->PolygonOffset(0, -pass);

        if (blending & kBlendingLess)
        {
            render_state->Enable(GL_ALPHA_TEST);
            render_state->AlphaFunction(GL_GREATER, 0.066f);
        }
        else if (blending & kBlendingMasked)
        {
            render_state->Enable(GL_ALPHA_TEST);
            render_state->AlphaFunction(GL_GREATER, 0);
        }
        else
        {
            render_state->Disable(GL_ALPHA_TEST);
        }

        if (blending & kBlendingAdd)
        {
            render_state->Enable(GL_BLEND);
            render_state->BlendFunction(GL_SRC_ALPHA, GL_ONE);
        }
        else if (blending & kBlendingAlpha)
        {
            render_state->Enable(GL_BLEND);
            render_state->BlendFunction(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        }
        else
        {
            render_state->Disable(GL_BLEND);
        }

        if (blending & (kBlendingCullBack | kBlendingCullFront))
        {
            render_state->Enable(GL_CULL_FACE);
            render_state->CullFace((blending & kBlendingCullFront) ? GL_FRONT : GL_BACK);
        }
        else
        {
            render_state->Disable(GL_CULL_FACE);
        }

        render_state->DepthMask((blending & kBlendingNoZBuffer) ? false : true);

        render_state->ActiveTexture(GL_TEXTURE1);
        render_state->Disable(GL_TEXTURE_2D);
        render_state->ActiveTexture(GL_TEXTURE0);
        render_state->Enable(GL_TEXTURE_2D);
        render_state->BindTexture(skin_tex);

        if (data.is_additive_)
        {
            render_state->TextureEnvironmentMode(GL_COMBINE);
            render_state->TextureEnvironmentCombineRGB(GL_REPLACE);
            render_state->TextureEnvironmentSource0RGB(GL_PREVIOUS);
        }
        else
        {
            render_state->TextureEnvironmentMode(GL_MODULATE);
            render_state->TextureEnvironmentCombineRGB(GL_MODULATE);
            render_state->TextureEnvironmentSource0RGB(GL_TEXTURE);
        }

        GLint old_clamp = kDummyClamp;

        if (blending & kBlendingClampY)
        {
            auto existing = texture_clamp_t.find(skin_tex);
            if (existing != texture_clamp_t.end())
                old_clamp = existing->second;

            render_state->TextureWrapT(renderer_dumb_clamp.d_ ? GL_CLAMP : GL_CLAMP_TO_EDGE);
        }

        sgl_enable_texture();
        sg_image img;
        img.id = skin_tex;

        sg_sampler img_sampler;
        GetImageSampler(skin_tex, &img_sampler.id);

        sgl_texture(img, img_sampler);

        uint32_t pipeline_flags = 0;
        pipeline_flags |= kPipelineDepthWrite;

        render_state->SetPipeline(pipeline_flags);

        sgl_begin_triangles();

        for (int pt = 0; pt < md->total_points_; pt++)
        {
            ModelCoordFunc(&data, pt);

            epi::SetRGBAAlpha(render_rgba, trans);

            sgl_v3f_t2f_c4b(render_position[0], render_position[1], render_position[2],
                             render_texture_coordinates[0], render_texture_coordinates[1],
                             epi::GetRGBARed(render_rgba), epi::GetRGBAGreen(render_rgba),
                             epi::GetRGBABlue(render_rgba), epi::GetRGBAAlpha(render_rgba));
        }

        sgl_end();

        if (old_clamp != kDummyClamp)
            render_state->TextureWrapT(old_clamp);
    }
}

void RenderModel2D(ModelData *md, const Image *skin_img, int frame, float x, float y, float xscale, float yscale,
                   const MapObjectDefinition *info)
{
    if (frame < 0 || frame >= md->total_frames_)
        return;

    render_backend->Flush(1, md->total_triangles_ * 3);

    GLuint skin_tex;

    if (skin_img != nullptr)
        skin_tex = ImageCache(skin_img, false, info->palremap_);
    else
        skin_tex = md->skin_id_list_[0];

    if (skin_tex == 0)
        FatalError("Model frame %s missing skins?\n", md->frames_[frame].name);

    xscale = yscale * info->model_scale_ * info->model_aspect_;
    yscale = yscale * info->model_scale_;

    render_state->Enable(GL_TEXTURE_2D);
    render_state->BindTexture(skin_tex);

    render_state->Enable(GL_BLEND);
    render_state->Enable(GL_CULL_FACE);

    RGBAColor color = (info->flags_ & kMapObjectFlagFuzzy) ? epi::MakeRGBA(0, 0, 0, 128) : kRGBAWhite;

    sgl_enable_texture();
    sg_image img;
    img.id = skin_tex;

    sg_sampler img_sampler;
    GetImageSampler(skin_tex, &img_sampler.id);

    sgl_texture(img, img_sampler);

    uint32_t pipeline_flags = 0;
    pipeline_flags |= kPipelineDepthWrite;

    render_state->SetPipeline(pipeline_flags);

    const ModelFrame *frame_ptr = &md->frames_[frame];

    sgl_begin_triangles();

    for (int pt = 0; pt < md->total_points_; pt++)
    {
        const ModelPoint *point = &md->points_[pt];

        float dx = frame_ptr->xs[point->vert_idx] * xscale;
        float dy = frame_ptr->ys[point->vert_idx] * xscale;
        float dz = (frame_ptr->zs[point->vert_idx] + info->model_bias_) * yscale;

        sgl_v3f_t2f_c4b(x + dy, y + dz, dx / 256.0f, point->skin_s, point->skin_t, epi::GetRGBARed(color),
                         epi::GetRGBAGreen(color), epi::GetRGBABlue(color), epi::GetRGBAAlpha(color));
    }

    sgl_end();

    render_state->Disable(GL_BLEND);
    render_state->Disable(GL_TEXTURE_2D);
    render_state->Disable(GL_CULL_FACE);
}

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
