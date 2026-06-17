//----------------------------------------------------------------------------
//  EDGE Lighting Shaders
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

#include "r_shader.h"

#include <unordered_map>

#include "ddf_main.h"
#include "epi.h"
#include "epi_simd.h"
#include "epi_str_hash.h"
#include "i_defs_gl.h"
#include "im_data.h"
#include "p_mobj.h"
#include "r_defs.h"
#include "r_gldefs.h"
#include "r_image.h"
#include "r_mirror.h"
#include "r_misc.h"
#include "r_state.h"
#include "r_texgl.h"
#include "r_units.h"

//----------------------------------------------------------------------------
//  LIGHT IMAGES
//----------------------------------------------------------------------------

static constexpr uint8_t kLightImageCurveSize = 32;

class LightImage
{
  public:
    std::string name_;

    const Image *image_;

    RGBAColor curve_[kLightImageCurveSize];

  public:
    LightImage(std::string_view name, const Image *img) : name_(name), image_(img)
    {
    }

    ~LightImage()
    {
    }

    inline GLuint TextureId() const
    {
        return ImageCache(image_, false);
    }

    void MakeStandardCurve() // TEMP CRUD
    {
        for (int i = 0; i < kLightImageCurveSize - 1; i++)
        {
            float d = i / (float)(kLightImageCurveSize - 1);

            float sq = exp(-5.44 * d * d);

            int r = (int)(255 * sq);
            int g = (int)(255 * sq);
            int b = (int)(255 * sq);

            curve_[i] = epi::MakeRGBA(r, g, b);
        }

        curve_[kLightImageCurveSize - 1] = kRGBABlack;
    }

    inline float CurveIntensity(float d)
    {
        float pos = d * (float)(kLightImageCurveSize - 1);

        if (pos >= (float)(kLightImageCurveSize - 1))
            return 0.0f;

        if (pos <= 0.0f)
            return 1.0f;

        int p1      = (int)pos;
        float frac  = pos - (float)p1;

        float v1 = epi::GetRGBARed(curve_[p1])     * (1.0f / 255.0f);
        float v2 = epi::GetRGBARed(curve_[p1 + 1]) * (1.0f / 255.0f);

        return v1 + frac * (v2 - v1);
    }

    RGBAColor CurvePoint(float d, RGBAColor tint)
    {
        // d is distance away from centre, between 0.0 and 1.0

        d *= (float)kLightImageCurveSize;

        if (d >= kLightImageCurveSize - 1.01)
            return curve_[kLightImageCurveSize - 1];

        // linearly interpolate between curve points

        int p1 = (int)floor(d);
        int dd = (int)(256 * (d - p1));

        int r1 = epi::GetRGBARed(curve_[p1]);
        int g1 = epi::GetRGBAGreen(curve_[p1]);
        int b1 = epi::GetRGBABlue(curve_[p1]);

        int r2 = epi::GetRGBARed(curve_[p1 + 1]);
        int g2 = epi::GetRGBAGreen(curve_[p1 + 1]);
        int b2 = epi::GetRGBABlue(curve_[p1 + 1]);

        r1 = (r1 * (256 - dd) + r2 * dd) >> 8;
        g1 = (g1 * (256 - dd) + g2 * dd) >> 8;
        b1 = (b1 * (256 - dd) + b2 * dd) >> 8;

        r1 = r1 * epi::GetRGBARed(tint) / 255;
        g1 = g1 * epi::GetRGBAGreen(tint) / 255;
        b1 = b1 * epi::GetRGBABlue(tint) / 255;

        return epi::MakeRGBA(r1, g1, b1);
    }
};

static std::unordered_map<epi::StringHash, LightImage *> known_light_images;

static LightImage *GetLightImage(const MapObjectDefinition *info)
{
    // Intentional Const Overrides
    DynamicLightDefinition *D_info = (DynamicLightDefinition *)&info->dlight_;

    if (!D_info->cache_data_)
    {
        const std::string &shape = D_info->shape_;

        EPI_ASSERT(!shape.empty());

        if (known_light_images.count(shape))
        {
            D_info->cache_data_ = known_light_images[shape];
        }
        else
        {
            const Image *image = ImageLookup(shape.c_str(), kImageNamespaceGraphic, kImageLookupNull);

            if (!image)
                FatalError("Missing dynamic light graphic: %s\n", shape.c_str());

            LightImage *lim = new LightImage(shape, image);

            lim->MakeStandardCurve();

            known_light_images.try_emplace(shape, lim);

            D_info->cache_data_ = lim;
        }
    }

    return (LightImage *)D_info->cache_data_;
}

//----------------------------------------------------------------------------
//  DYNAMIC LIGHTS
//----------------------------------------------------------------------------

class dynlight_shader_c : public AbstractShader
{
  private:
    MapObject *mo;

    LightImage *lim;

    float radius;
    bool  normal_is_horiz_;
    float norm_nx_;
    float norm_ny_;
    float norm_nz_;
    float r_xy_scale_;

  public:
    dynlight_shader_c(MapObject *object, float r)
        : mo(object), radius(r), normal_is_horiz_(false), norm_nx_(0.0f), norm_ny_(0.0f), norm_nz_(1.0f),
          r_xy_scale_(1.0f)
    {
        // Note: this is shared, we must not delete it
        lim = GetLightImage(mo->info_);
    }

    ~dynlight_shader_c()
    { /* nothing to do */
    }

  private:
    inline void PrepareNormal(const HMM_Vec3 *normal)
    {
        float nx = normal->X;
        float ny = normal->Y;
        float nz = normal->Z;

        if (fabs(nz) > 50 * (fabs(nx) + fabs(ny)))
        {
            normal_is_horiz_ = true;
        }
        else
        {
            normal_is_horiz_  = false;
            float n_len_inv   = epi::RsqrtF32(nx * nx + ny * ny + nz * nz);
            norm_nx_          = nx * n_len_inv;
            norm_ny_          = ny * n_len_inv;
            norm_nz_          = nz * n_len_inv;
            r_xy_scale_       = epi::RsqrtF32(1.0f - norm_nz_ * norm_nz_);
        }
    }

    inline float TexCoord(HMM_Vec2 *texc, float r, const HMM_Vec3 *lit_pos)
    {
        float mx = mo->x;
        float my = mo->y;
        float mz = MapObjectMidZ(mo);

        render_mirror_set.Coordinate(mx, my);
        render_mirror_set.Height(mz);

        float dx = lit_pos->X - mx;
        float dy = lit_pos->Y - my;
        float dz = lit_pos->Z - mz;

        if (normal_is_horiz_)
        {
            texc->X = (1 + dx / r) / 2.0;
            texc->Y = (1 + dy / r) / 2.0;

            return fabs(dz) / r;
        }
        else
        {
            float dxy   = norm_nx_ * dy - norm_ny_ * dx;
            float adj_r = r * r_xy_scale_;

            texc->Y = (1 + dz / adj_r) / 2.0;
            texc->X = (1 + dxy / adj_r) / 2.0;

            return fabs(norm_nx_ * dx + norm_ny_ * dy + norm_nz_ * dz) / adj_r;
        }
    }

    inline float WhatRadius()
    {
        return radius * render_mirror_set.XYScale();
    }

    inline RGBAColor WhatColor()
    {
        return mo->dynamic_light_.color;
    }

    inline DynamicLightType WhatType()
    {
        return mo->info_->dlight_.type_;
    }

  public:
    void Sample(ColorMixer *col, float x, float y, float z)
    {
        float mx = mo->x;
        float my = mo->y;
        float mz = MapObjectMidZ(mo);

        render_mirror_set.Coordinate(mx, my);
        render_mirror_set.Height(mz);

        float dx = x - mx;
        float dy = y - my;
        float dz = z - mz;

        float dist = sqrtf(dx * dx + dy * dy + dz * dz);

        if (WhatType() == kDynamicLightTypeNone)
            return;

        RGBAColor new_col = lim->CurvePoint(dist / WhatRadius(), WhatColor());

        float L = (mo->info_->force_fullbright_ ? 255.0f : mo->state_->bright) / 255.0;

        if (new_col != kRGBABlack && L > 1 / 256.0)
        {
            if (WhatType() == kDynamicLightTypeAdd)
                col->add_GIVE(new_col, L);
            else
                col->modulate_GIVE(new_col, L);
        }
    }

    void Corner(ColorMixer *col, float nx, float ny, float nz, MapObject *mod_pos, bool is_weapon)
    {
        float mx = mo->x;
        float my = mo->y;
        float mz = MapObjectMidZ(mo);

        if (is_weapon)
        {
            mx += view_cosine * 24;
            my += view_sine * 24;
        }

        render_mirror_set.Coordinate(mx, my);
        render_mirror_set.Height(mz);

        float dx = mod_pos->x;
        float dy = mod_pos->y;
        float dz = MapObjectMidZ(mod_pos);

        render_mirror_set.Coordinate(dx, dy);
        render_mirror_set.Height(dz);

        dx -= mx;
        dy -= my;
        dz -= mz;

        float dist_sq  = dx * dx + dy * dy + dz * dz;
        float dist_inv = epi::RsqrtF32(dist_sq);
        float dist     = dist_sq * dist_inv;

        dx *= dist_inv;
        dy *= dist_inv;
        dz *= dist_inv;

        dist = HMM_MAX(1.0f, dist - mod_pos->radius_ * render_mirror_set.XYScale());

        float L = 0.6 - 0.7 * (dx * nx + dy * ny + dz * nz);

        L *= (mo->info_->force_fullbright_ ? 255.0f : mo->state_->bright) / 255.0;

        if (WhatType() == kDynamicLightTypeNone)
            return;

        RGBAColor new_col = lim->CurvePoint(dist / WhatRadius(), WhatColor());

        if (new_col != kRGBABlack && L > 1 / 256.0)
        {
            if (WhatType() == kDynamicLightTypeAdd)
                col->add_GIVE(new_col, L);
            else
                col->modulate_GIVE(new_col, L);
        }
    }

    void WorldMix(GLuint shape, int num_vert, GLuint tex, float alpha, int *pass_var, BlendingMode blending,
                  bool masked, void *data, ShaderCoordinateFunction func,
                  ShaderCoordinateBatchFunction batch_func = nullptr)
    {
        if (WhatType() == kDynamicLightTypeNone)
            return;

        bool is_additive = (WhatType() == kDynamicLightTypeAdd);

        RGBAColor col = WhatColor();

        float L = (mo->info_->force_fullbright_ ? 255.0f : mo->state_->bright) / 255.0;

        float R = L * epi::GetRGBARed(col);
        float G = L * epi::GetRGBAGreen(col);
        float B = L * epi::GetRGBABlue(col);

        int out_vert;
        if (shape == GL_POLYGON)
            out_vert = (num_vert - 2) * 3;
        else if (shape == GL_QUAD_STRIP)
            out_vert = (num_vert / 2 - 1) * 6;
        else
            out_vert = num_vert;

        RendererVertex temp[kMaximumPolygonVertices];

        uint8_t alpha_byte = (uint8_t)(alpha * 255.0f);

        int v_idx = 0;

        epi::SimdF32x4 r_base = epi::SplatF32x4(R);
        epi::SimdF32x4 g_base = epi::SplatF32x4(G);
        epi::SimdF32x4 b_base = epi::SplatF32x4(B);
        epi::SimdI32x4 a_v    = epi::SplatI32x4((int32_t)alpha_byte);

        if (num_vert > 0)
        {
            HMM_Vec3  peek_normal, peek_lit_pos, peek_pos;
            HMM_Vec2  peek_texc;
            RGBAColor peek_rgba;
            (*func)(data, 0, &peek_pos, &peek_rgba, &peek_texc, &peek_normal, &peek_lit_pos);
            PrepareNormal(&peek_normal);
        }

        if (batch_func)
        {
            HMM_Vec3  batch_pos[4];
            RGBAColor batch_rgb[4];
            HMM_Vec2  batch_texc[4];
            HMM_Vec3  batch_normal[4];
            HMM_Vec3  batch_lit_pos[4];

            for (; v_idx + 3 < num_vert; v_idx += 4)
            {
                (*batch_func)(data, v_idx, batch_pos, batch_rgb, batch_texc, batch_normal, batch_lit_pos);

                float ity_arr[4];
                for (int k = 0; k < 4; k++)
                {
                    RendererVertex *dest         = temp + v_idx + k;
                    dest->position               = batch_pos[k];
                    dest->texture_coordinates[0] = batch_texc[k];
                    float dist                   = TexCoord(&dest->texture_coordinates[1], WhatRadius(), &batch_lit_pos[k]);
                    ity_arr[k]                   = lim->CurveIntensity(dist);
                }

                epi::SimdF32x4 ity_v = epi::SetF32x4(ity_arr[0], ity_arr[1], ity_arr[2], ity_arr[3]);

                epi::SimdI32x4 r_i = epi::CvtF32x4ToI32x4(epi::MulF32x4(r_base, ity_v));
                epi::SimdI32x4 g_i = epi::CvtF32x4ToI32x4(epi::MulF32x4(g_base, ity_v));
                epi::SimdI32x4 b_i = epi::CvtF32x4ToI32x4(epi::MulF32x4(b_base, ity_v));

                epi::SimdI32x4 rgba_v =
                    epi::OrI32x4(epi::OrI32x4(epi::ShiftLeftI32x4<24>(r_i), epi::ShiftLeftI32x4<16>(g_i)),
                                 epi::OrI32x4(epi::ShiftLeftI32x4<8>(b_i), a_v));

                int32_t simd_tmp[4];
                epi::StoreI32x4(simd_tmp, rgba_v);
                (temp + v_idx + 0)->rgba = (RGBAColor)simd_tmp[0];
                (temp + v_idx + 1)->rgba = (RGBAColor)simd_tmp[1];
                (temp + v_idx + 2)->rgba = (RGBAColor)simd_tmp[2];
                (temp + v_idx + 3)->rgba = (RGBAColor)simd_tmp[3];
            }
        }
        else
        {
            for (; v_idx + 3 < num_vert; v_idx += 4)
            {
                float ity_arr[4];

                for (int k = 0; k < 4; k++)
                {
                    RendererVertex *dest = temp + v_idx + k;
                    HMM_Vec3        lit_pos, normal;
                    (*func)(data, v_idx + k, &dest->position, &dest->rgba, &dest->texture_coordinates[0], &normal,
                            &lit_pos);
                    EPI_UNUSED(normal);
                    float dist = TexCoord(&dest->texture_coordinates[1], WhatRadius(), &lit_pos);
                    ity_arr[k] = lim->CurveIntensity(dist);
                }

                epi::SimdF32x4 ity_v = epi::SetF32x4(ity_arr[0], ity_arr[1], ity_arr[2], ity_arr[3]);

                epi::SimdI32x4 r_i = epi::CvtF32x4ToI32x4(epi::MulF32x4(r_base, ity_v));
                epi::SimdI32x4 g_i = epi::CvtF32x4ToI32x4(epi::MulF32x4(g_base, ity_v));
                epi::SimdI32x4 b_i = epi::CvtF32x4ToI32x4(epi::MulF32x4(b_base, ity_v));

                epi::SimdI32x4 rgba_v =
                    epi::OrI32x4(epi::OrI32x4(epi::ShiftLeftI32x4<24>(r_i), epi::ShiftLeftI32x4<16>(g_i)),
                                 epi::OrI32x4(epi::ShiftLeftI32x4<8>(b_i), a_v));

                int32_t simd_tmp[4];
                epi::StoreI32x4(simd_tmp, rgba_v);
                (temp + v_idx + 0)->rgba = (RGBAColor)simd_tmp[0];
                (temp + v_idx + 1)->rgba = (RGBAColor)simd_tmp[1];
                (temp + v_idx + 2)->rgba = (RGBAColor)simd_tmp[2];
                (temp + v_idx + 3)->rgba = (RGBAColor)simd_tmp[3];
            }
        }

        for (; v_idx < num_vert; v_idx++)
        {
            RendererVertex *dest = temp + v_idx;
            HMM_Vec3        lit_pos, normal;
            (*func)(data, v_idx, &dest->position, &dest->rgba, &dest->texture_coordinates[0], &normal, &lit_pos);
            EPI_UNUSED(normal);
            float dist = TexCoord(&dest->texture_coordinates[1], WhatRadius(), &lit_pos);
            float ity  = lim->CurveIntensity(dist);
            dest->rgba = epi::MakeRGBA((uint8_t)(R * ity), (uint8_t)(G * ity), (uint8_t)(B * ity), alpha_byte);
        }

        RendererVertex *glvert =
            BeginRenderUnit(out_vert,
                            (is_additive && masked) ? (GLuint)kTextureEnvironmentSkipRGB
                            : is_additive           ? (GLuint)kTextureEnvironmentDisable
                                                    : GL_MODULATE,
                            (is_additive && !masked) ? 0 : tex, GL_MODULATE, lim->TextureId(), *pass_var, blending,
                            *pass_var > 0 ? kRGBANoValue : mo->subsector_->sector->properties.fog_color,
                            mo->subsector_->sector->properties.fog_density);

        if (shape == GL_POLYGON)
        {
            for (int i = 1; i < num_vert - 1; i++)
            {
                *glvert++ = temp[0];
                *glvert++ = temp[i];
                *glvert++ = temp[i + 1];
            }
        }
        else if (shape == GL_QUAD_STRIP)
        {
            for (int i = 0; i < num_vert - 2; i += 2)
            {
                *glvert++ = temp[i];
                *glvert++ = temp[i + 1];
                *glvert++ = temp[i + 3];
                *glvert++ = temp[i];
                *glvert++ = temp[i + 3];
                *glvert++ = temp[i + 2];
            }
        }
        else
        {
            for (int i = 0; i < num_vert; i++)
                *glvert++ = temp[i];
        }

        EndRenderUnit(out_vert);

        (*pass_var) += 1;
    }

    void SetRadius(float r)
    {
        radius = r;
    }
};

AbstractShader *MakeDLightShader(MapObject *mo, float r)
{
    return new dynlight_shader_c(mo, r);
}

//----------------------------------------------------------------------------
//  SECTOR GLOWS
//----------------------------------------------------------------------------

class plane_glow_c : public AbstractShader
{
  private:
    MapObject *mo;

    LightImage *lim;

    float radius;

  public:
    plane_glow_c(MapObject *_glower, float r) : mo(_glower), radius(r)
    {
        lim = GetLightImage(mo->info_);
    }

    ~plane_glow_c()
    { /* nothing to do */
    }

  private:
    inline float Dist(const Sector *sec, float z)
    {
        if (mo->info_->glow_type_ == kSectorGlowTypeFloor)
            return fabs(sec->floor_height - z);
        else
            return fabs(sec->ceiling_height - z); // kSectorGlowTypeCeiling
    }

    inline void TexCoord(HMM_Vec2 *texc, float r, const Sector *sec, const HMM_Vec3 *lit_pos, const HMM_Vec3 *normal)
    {
        EPI_UNUSED(normal);
        texc->X = 0.5;
        texc->Y = 0.5 + Dist(sec, lit_pos->Z) / r / 2.0;
    }

    inline float WhatRadius()
    {
        return radius * render_mirror_set.XYScale();
    }

    inline RGBAColor WhatColor()
    {
        return mo->dynamic_light_.color;
    }

    inline DynamicLightType WhatType()
    {
        return mo->info_->dlight_.type_;
    }

  public:
    void Sample(ColorMixer *col, float x, float y, float z)
    {
        EPI_UNUSED(x);
        EPI_UNUSED(y);
        const Sector *sec = mo->subsector_->sector;

        float dist = Dist(sec, z);

        if (WhatType() == kDynamicLightTypeNone)
            return;

        RGBAColor new_col = lim->CurvePoint(dist / WhatRadius(), WhatColor());

        float L = (mo->info_->force_fullbright_ ? 255.0f : mo->state_->bright) / 255.0;

        if (new_col != kRGBABlack && L > 1 / 256.0)
        {
            if (WhatType() == kDynamicLightTypeAdd)
                col->add_GIVE(new_col, L);
            else
                col->modulate_GIVE(new_col, L);
        }
    }

    void Corner(ColorMixer *col, float nx, float ny, float nz, MapObject *mod_pos, bool is_weapon)
    {
        EPI_UNUSED(nx);
        EPI_UNUSED(ny);
        const Sector *sec = mo->subsector_->sector;

        float dz = (mo->info_->glow_type_ == kSectorGlowTypeFloor) ? +1 : -1;
        float dist;

        if (is_weapon)
        {
            float weapon_z = mod_pos->z + mod_pos->height_ * mod_pos->info_->shotheight_;

            if (mo->info_->glow_type_ == kSectorGlowTypeFloor)
                dist = weapon_z - sec->floor_height;
            else
                dist = sec->ceiling_height - weapon_z;
        }
        else if (mo->info_->glow_type_ == kSectorGlowTypeFloor)
            dist = mod_pos->z - sec->floor_height;
        else
            dist = sec->ceiling_height - (mod_pos->z + mod_pos->height_);

        dist = HMM_MAX(1.0, fabs(dist));

        float L = 0.6 - 0.7 * (dz * nz);

        L *= (mo->info_->force_fullbright_ ? 255.0f : mo->state_->bright) / 255.0;

        if (WhatType() == kDynamicLightTypeNone)
            return;

        RGBAColor new_col = lim->CurvePoint(dist / WhatRadius(), WhatColor());

        if (new_col != kRGBABlack && L > 1 / 256.0)
        {
            if (WhatType() == kDynamicLightTypeAdd)
                col->add_GIVE(new_col, L);
            else
                col->modulate_GIVE(new_col, L);
        }
    }

    void WorldMix(GLuint shape, int num_vert, GLuint tex, float alpha, int *pass_var, BlendingMode blending,
                  bool masked, void *data, ShaderCoordinateFunction func,
                  ShaderCoordinateBatchFunction batch_func = nullptr)
    {
        EPI_UNUSED(batch_func);
        const Sector *sec = mo->subsector_->sector;

        if (WhatType() == kDynamicLightTypeNone)
            return;

        bool is_additive = (WhatType() == kDynamicLightTypeAdd);

        RGBAColor col = WhatColor();

        float L = (mo->info_->force_fullbright_ ? 255.0f : mo->state_->bright) / 255.0;

        float R = L * epi::GetRGBARed(col);
        float G = L * epi::GetRGBAGreen(col);
        float B = L * epi::GetRGBABlue(col);

        int out_vert;
        if (shape == GL_POLYGON)
            out_vert = (num_vert - 2) * 3;
        else if (shape == GL_QUAD_STRIP)
            out_vert = (num_vert / 2 - 1) * 6;
        else
            out_vert = num_vert;

        RendererVertex temp[kMaximumPolygonVertices];
        for (int v_idx = 0; v_idx < num_vert; v_idx++)
        {
            RendererVertex *dest = temp + v_idx;

            HMM_Vec3 lit_pos;
            HMM_Vec3 normal;

            (*func)(data, v_idx, &dest->position, &dest->rgba, &dest->texture_coordinates[0], &normal, &lit_pos);

            TexCoord(&dest->texture_coordinates[1], WhatRadius(), sec, &lit_pos, &normal);

            dest->rgba = epi::MakeRGBA((uint8_t)R, (uint8_t)G, (uint8_t)B, (uint8_t)(alpha * 255.0f));
        }

        RendererVertex *glvert =
            BeginRenderUnit(out_vert,
                            (is_additive && masked) ? (GLuint)kTextureEnvironmentSkipRGB
                            : is_additive           ? (GLuint)kTextureEnvironmentDisable
                                                    : GL_MODULATE,
                            (is_additive && !masked) ? 0 : tex, GL_MODULATE, lim->TextureId(), *pass_var, blending,
                            *pass_var > 0 ? kRGBANoValue : mo->subsector_->sector->properties.fog_color,
                            mo->subsector_->sector->properties.fog_density);

        if (shape == GL_POLYGON)
        {
            for (int i = 1; i < num_vert - 1; i++)
            {
                *glvert++ = temp[0];
                *glvert++ = temp[i];
                *glvert++ = temp[i + 1];
            }
        }
        else if (shape == GL_QUAD_STRIP)
        {
            for (int i = 0; i < num_vert - 2; i += 2)
            {
                *glvert++ = temp[i];
                *glvert++ = temp[i + 1];
                *glvert++ = temp[i + 3];
                *glvert++ = temp[i];
                *glvert++ = temp[i + 3];
                *glvert++ = temp[i + 2];
            }
        }
        else
        {
            for (int v_idx = 0; v_idx < num_vert; v_idx++)
                *glvert++ = temp[v_idx];
        }

        EndRenderUnit(out_vert);

        (*pass_var) += 1;
    }

    void SetRadius(float r)
    {
        radius = r;
    }
};

AbstractShader *MakePlaneGlow(MapObject *mo, float r)
{
    return new plane_glow_c(mo, r);
}

//----------------------------------------------------------------------------
//  WALL GLOWS
//----------------------------------------------------------------------------

class wall_glow_c : public AbstractShader
{
  private:
    Line      *ld;
    MapObject *mo;

    float norm_x, norm_y; // normal

    LightImage *lim;

    float radius;

    inline float Dist(float x, float y)
    {
        return (ld->vertex_1->X - x) * norm_x + (ld->vertex_1->Y - y) * norm_y;
    }

    inline void TexCoord(HMM_Vec2 *texc, float r, const Sector *sec, const HMM_Vec3 *lit_pos, const HMM_Vec3 *normal)
    {
        EPI_UNUSED(sec);
        EPI_UNUSED(normal);
        texc->X = 0.5;
        texc->Y = 0.5 + Dist(lit_pos->X, lit_pos->Y) / r / 2.0;
    }

    inline float WhatRadius()
    {
        return radius * render_mirror_set.XYScale();
    }

    inline RGBAColor WhatColor()
    {
        return mo->dynamic_light_.color;
    }

    inline DynamicLightType WhatType()
    {
        return mo->info_->dlight_.type_;
    }

  public:
    wall_glow_c(MapObject *_glower, float r) : mo(_glower), radius(r)
    {
        EPI_ASSERT(mo->dynamic_light_.glow_wall);
        ld     = mo->dynamic_light_.glow_wall;
        norm_x = (ld->vertex_1->Y - ld->vertex_2->Y) / ld->length;
        norm_y = (ld->vertex_2->X - ld->vertex_1->X) / ld->length;
        // Note: this is shared, we must not delete it
        lim = GetLightImage(mo->info_);
    }

    ~wall_glow_c()
    { /* nothing to do */
    }

    void Sample(ColorMixer *col, float x, float y, float z)
    {
        EPI_UNUSED(z);
        float dist = Dist(x, y);

        float L = std::log1p(dist);

        L *= (mo->info_->force_fullbright_ ? 255.0f : mo->state_->bright) / 255.0;

        if (WhatType() == kDynamicLightTypeNone)
            return;

        RGBAColor new_col = lim->CurvePoint(dist / WhatRadius(), WhatColor());

        if (new_col != kRGBABlack && L > 1 / 256.0)
        {
            if (WhatType() == kDynamicLightTypeAdd)
                col->add_GIVE(new_col, L);
            else
                col->modulate_GIVE(new_col, L);
        }
    }

    void Corner(ColorMixer *col, float nx, float ny, float nz, MapObject *mod_pos, bool is_weapon = false)
    {
        EPI_UNUSED(nx);
        EPI_UNUSED(ny);
        EPI_UNUSED(nz);
        EPI_UNUSED(is_weapon);
        float dist = Dist(mod_pos->x, mod_pos->y);

        float L = std::log1p(dist);

        L *= (mo->info_->force_fullbright_ ? 255.0f : mo->state_->bright) / 255.0;

        if (WhatType() == kDynamicLightTypeNone)
            return;

        RGBAColor new_col = lim->CurvePoint(dist / WhatRadius(), WhatColor());

        if (new_col != kRGBABlack && L > 1 / 256.0)
        {
            if (WhatType() == kDynamicLightTypeAdd)
                col->add_GIVE(new_col, L);
            else
                col->modulate_GIVE(new_col, L);
        }
    }

    void WorldMix(GLuint shape, int num_vert, GLuint tex, float alpha, int *pass_var, BlendingMode blending,
                  bool masked, void *data, ShaderCoordinateFunction func,
                  ShaderCoordinateBatchFunction batch_func = nullptr)
    {
        EPI_UNUSED(batch_func);
        const Sector *sec = mo->subsector_->sector;

        if (WhatType() == kDynamicLightTypeNone)
            return;

        bool is_additive = (WhatType() == kDynamicLightTypeAdd);

        RGBAColor col = WhatColor();

        float L = (mo->info_->force_fullbright_ ? 255.0f : mo->state_->bright) / 255.0;

        float R = L * epi::GetRGBARed(col);
        float G = L * epi::GetRGBAGreen(col);
        float B = L * epi::GetRGBABlue(col);

        int out_vert;
        if (shape == GL_POLYGON)
            out_vert = (num_vert - 2) * 3;
        else if (shape == GL_QUAD_STRIP)
            out_vert = (num_vert / 2 - 1) * 6;
        else
            out_vert = num_vert;

        RendererVertex temp[kMaximumPolygonVertices];
        for (int v_idx = 0; v_idx < num_vert; v_idx++)
        {
            RendererVertex *dest = temp + v_idx;

            HMM_Vec3 lit_pos;
            HMM_Vec3 normal;

            (*func)(data, v_idx, &dest->position, &dest->rgba, &dest->texture_coordinates[0], &normal, &lit_pos);

            TexCoord(&dest->texture_coordinates[1], WhatRadius(), sec, &lit_pos, &normal);

            dest->rgba =
                epi::MakeRGBA((uint8_t)(R * render_view_red_multiplier), (uint8_t)(G * render_view_green_multiplier),
                              (uint8_t)(B * render_view_blue_multiplier), (uint8_t)(alpha * 255.0f));
        }

        RendererVertex *glvert =
            BeginRenderUnit(out_vert,
                            (is_additive && masked) ? (GLuint)kTextureEnvironmentSkipRGB
                            : is_additive           ? (GLuint)kTextureEnvironmentDisable
                                                    : GL_MODULATE,
                            (is_additive && !masked) ? 0 : tex, GL_MODULATE, lim->TextureId(), *pass_var, blending,
                            *pass_var > 0 ? kRGBANoValue : mo->subsector_->sector->properties.fog_color,
                            mo->subsector_->sector->properties.fog_density);

        if (shape == GL_POLYGON)
        {
            for (int i = 1; i < num_vert - 1; i++)
            {
                *glvert++ = temp[0];
                *glvert++ = temp[i];
                *glvert++ = temp[i + 1];
            }
        }
        else if (shape == GL_QUAD_STRIP)
        {
            for (int i = 0; i < num_vert - 2; i += 2)
            {
                *glvert++ = temp[i];
                *glvert++ = temp[i + 1];
                *glvert++ = temp[i + 3];
                *glvert++ = temp[i];
                *glvert++ = temp[i + 3];
                *glvert++ = temp[i + 2];
            }
        }
        else
        {
            for (int v_idx = 0; v_idx < num_vert; v_idx++)
                *glvert++ = temp[v_idx];
        }

        EndRenderUnit(out_vert);

        (*pass_var) += 1;
    }

    void SetRadius(float r)
    {
        radius = r;
    }
};

AbstractShader *MakeWallGlow(MapObject *mo, float r)
{
    return new wall_glow_c(mo, r);
}

void DeleteAllLightImages()
{
    for (std::unordered_map<epi::StringHash, LightImage *>::iterator iter     = known_light_images.begin(),
                                                                     iter_end = known_light_images.end();
         iter != iter_end; ++iter)
    {
        delete iter->second;
    }
    known_light_images.clear();
}

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
