#include <math.h>
#include <stddef.h>
#include <string.h>

#include <vector>

#include "epi.h"
#include "epi_endian.h"
#include "epi_str_compare.h"
#include "im_data.h"
#include "r_mdcommon.h"
#include "r_model.h"
#include "r_texgl.h"

/*============== MD2 FORMAT DEFINITIONS ====================*/

static constexpr const char *kMD2Identifier = "IDP2";
static constexpr uint8_t     kMD2Version    = 8;

struct RawMD2Header
{
    char ident[4];

    int32_t version;

    int32_t skin_width;
    int32_t skin_height;

    int32_t frame_size;

    int32_t num_skins;
    int32_t num_verts;
    int32_t num_st;
    int32_t num_tris;
    int32_t num_glcmds;
    int32_t num_frames;

    int32_t ofs_skins;
    int32_t ofs_st;
    int32_t ofs_tris;
    int32_t ofs_frames;
    int32_t ofs_glcmds;
    int32_t ofs_end;
};

struct RawMD2TextureCoordinate
{
    uint16_t s, t;
};

struct RawMD2Triangle
{
    uint16_t index_xyz[3];
    uint16_t index_st[3];
};

struct RawMD2Vertex
{
    uint8_t x, y, z;
    uint8_t light_normal;
};

struct RawMD2Frame
{
    uint32_t scale[3];
    uint32_t translate[3];

    char name[16];
};

/*============== MD3 FORMAT DEFINITIONS ====================*/

static constexpr const char *kMD3Identifier = "IDP3";
static constexpr uint8_t     kMD3Version    = 15;

struct RawMD3Header
{
    char    ident[4];
    int32_t version;

    char     name[64];
    uint32_t flags;

    int32_t num_frames;
    int32_t num_tags;
    int32_t num_meshes;
    int32_t num_skins;

    int32_t ofs_frames;
    int32_t ofs_tags;
    int32_t ofs_meshes;
    int32_t ofs_end;
};

struct RawMD3Mesh
{
    char ident[4];
    char name[64];

    uint32_t flags;

    int32_t num_frames;
    int32_t num_shaders;
    int32_t num_verts;
    int32_t num_tris;

    int32_t ofs_tris;
    int32_t ofs_shaders;
    int32_t ofs_texcoords;
    int32_t ofs_verts;
    int32_t ofs_next_mesh;
};

struct RawMD3TextureCoordinate
{
    uint32_t s, t;
};

struct RawMD3Triangle
{
    uint32_t index_xyz[3];
};

struct RawMD3Vertex
{
    int16_t x, y, z;

    uint8_t pitch, yaw;
};

struct RawMD3Frame
{
    uint32_t mins[3];
    uint32_t maxs[3];
    uint32_t origin[3];
    uint32_t radius;

    char name[16];
};

/*============== MDL FORMAT DEFINITIONS ====================*/

static constexpr const char *kMDLIdentifier = "IDPO";
static constexpr uint8_t     kMDLVersion    = 6;

struct RawMDLHeader
{
    char ident[4];

    int32_t version;

    uint32_t scale_x;
    uint32_t scale_y;
    uint32_t scale_z;

    uint32_t trans_x;
    uint32_t trans_y;
    uint32_t trans_z;

    uint32_t boundingradius;

    uint32_t eyepos_x;
    uint32_t eyepos_y;
    uint32_t eyepos_z;

    int32_t num_skins;

    int32_t skin_width;
    int32_t skin_height;

    int32_t num_verts;
    int32_t num_tris;
    int32_t num_frames;

    int32_t  synctype;
    int32_t  flags;
    uint32_t size;
};

struct RawMDLTextureCoordinate
{
    int32_t onseam;
    int32_t s;
    int32_t t;
};

struct RawMDLTriangle
{
    int32_t facesfront;
    int32_t vertex[3];
};

struct RawMDLVertex
{
    uint8_t x, y, z;
    uint8_t light_normal;
};

struct RawMDLSimpleFrame
{
    RawMDLVertex  bboxmin;
    RawMDLVertex  bboxmax;
    char          name[16];
    RawMDLVertex *verts;
};

struct RawMDLFrame
{
    int32_t           type;
    RawMDLSimpleFrame frame;
};

/*============== ModelData ====================*/

ModelData::ModelData(int nframes, int npoints, int ntris, int nverts)
    : total_frames_(nframes), total_points_(npoints), total_triangles_(ntris), vertices_per_frame_(nverts)
{
    frames_ = new ModelFrame[total_frames_];
    points_ = new ModelPoint[total_points_];
}

ModelData::~ModelData()
{
    for (int i = 0; i < total_frames_; i++)
    {
        delete[] frames_[i].xs;
        delete[] frames_[i].ys;
        delete[] frames_[i].zs;
        delete[] frames_[i].normal_indices;
        delete[] (char *)frames_[i].name;
        delete[] frames_[i].used_normals;
    }
    delete[] frames_;
    delete[] points_;
}

/*============== LOADING HELPERS ====================*/

static const char *CopyFrameName(RawMD2Frame *frm)
{
    char *str = new char[20];
    memcpy(str, frm->name, 16);
    str[16] = 0;
    return str;
}

static const char *CopyFrameName(RawMD3Frame *frm)
{
    char *str = new char[20];
    memcpy(str, frm->name, 16);
    str[16] = 0;
    return str;
}

static const char *CopyFrameName(RawMDLSimpleFrame *frm)
{
    char *str = new char[20];
    memcpy(str, frm->name, 16);
    str[16] = 0;
    return str;
}

static short *CreateNormalList(uint8_t *which_normals)
{
    int count = 0;

    for (int i = 0; i < kTotalMDFormatNormals; i++)
        if (which_normals[i])
            count++;

    short *n_list = new short[count + 1];

    count = 0;

    for (int i = 0; i < kTotalMDFormatNormals; i++)
        if (which_normals[i])
            n_list[count++] = i;

    n_list[count] = -1;

    return n_list;
}

/*============== MD2 LOADING ====================*/

static ModelData *MD2Load(epi::File *f, float &radius)
{
    radius = 1;

    RawMD2Header header;

    f->Read(&header, sizeof(RawMD2Header));

    int version = AlignedLittleEndianS32(header.version);

    LogDebug("MODEL IDENT: [%c%c%c%c] VERSION: %d", header.ident[0], header.ident[1], header.ident[2], header.ident[3],
             version);

    if (epi::StringPrefixCompare(header.ident, kMD2Identifier) != 0)
        FatalError("MD2LoadModel: file is not an MD2 model!");

    if (version != kMD2Version)
        FatalError("MD2LoadModel: strange version!");

    int num_frames      = AlignedLittleEndianS32(header.num_frames);
    int num_verts       = AlignedLittleEndianS32(header.num_verts);
    int total_triangles = AlignedLittleEndianS32(header.num_tris);
    int num_sts         = AlignedLittleEndianS32(header.num_st);
    int skin_width      = AlignedLittleEndianS32(header.skin_width);
    int skin_height     = AlignedLittleEndianS32(header.skin_height);
    int total_points    = total_triangles * 3;

    RawMD2Triangle *md2_triangles = new RawMD2Triangle[total_triangles];

    f->Seek(AlignedLittleEndianS32(header.ofs_tris), epi::File::kSeekpointStart);
    f->Read(md2_triangles, total_triangles * sizeof(RawMD2Triangle));

    for (int tri = 0; tri < total_triangles; tri++)
    {
        md2_triangles[tri].index_xyz[0] = AlignedLittleEndianU16(md2_triangles[tri].index_xyz[0]);
        md2_triangles[tri].index_xyz[1] = AlignedLittleEndianU16(md2_triangles[tri].index_xyz[1]);
        md2_triangles[tri].index_xyz[2] = AlignedLittleEndianU16(md2_triangles[tri].index_xyz[2]);
        md2_triangles[tri].index_st[0]  = AlignedLittleEndianU16(md2_triangles[tri].index_st[0]);
        md2_triangles[tri].index_st[1]  = AlignedLittleEndianU16(md2_triangles[tri].index_st[1]);
        md2_triangles[tri].index_st[2]  = AlignedLittleEndianU16(md2_triangles[tri].index_st[2]);
    }

    RawMD2TextureCoordinate *md2_sts = new RawMD2TextureCoordinate[num_sts];

    f->Seek(AlignedLittleEndianS32(header.ofs_st), epi::File::kSeekpointStart);
    f->Read(md2_sts, num_sts * sizeof(RawMD2TextureCoordinate));

    for (int st = 0; st < num_sts; st++)
    {
        md2_sts[st].s = AlignedLittleEndianU16(md2_sts[st].s);
        md2_sts[st].t = AlignedLittleEndianU16(md2_sts[st].t);
    }

    LogDebug("  frames:%d  points:%d  triangles: %d\n", num_frames, total_points, total_triangles);

    ModelData *md = new ModelData(num_frames, total_points, total_triangles, num_verts);

    LogDebug("  vertices_per_frame_:%d\n", md->vertices_per_frame_);

    ModelPoint *point = md->points_;

    for (int i = 0; i < total_triangles; i++)
    {
        EPI_ASSERT(point < md->points_ + md->total_points_);

        for (int j = 0; j < 3; j++, point++)
        {
            RawMD2Triangle t = md2_triangles[i];
            point->skin_s    = (float)md2_sts[t.index_st[j]].s / skin_width;
            point->skin_t    = 1.0f - ((float)md2_sts[t.index_st[j]].t / skin_height);
            point->vert_idx  = t.index_xyz[j];
            EPI_ASSERT(point->vert_idx >= 0);
            EPI_ASSERT(point->vert_idx < md->vertices_per_frame_);
        }
    }

    EPI_ASSERT(point == md->points_ + md->total_points_);

    delete[] md2_triangles;
    delete[] md2_sts;

    uint8_t       which_normals[kTotalMDFormatNormals];
    RawMD2Vertex *raw_verts = new RawMD2Vertex[num_verts];

    f->Seek(AlignedLittleEndianS32(header.ofs_frames), epi::File::kSeekpointStart);

    for (int i = 0; i < num_frames; i++)
    {
        RawMD2Frame raw_frame;

        f->Read(&raw_frame, sizeof(raw_frame));

        for (int j = 0; j < 3; j++)
        {
            raw_frame.scale[j]     = AlignedLittleEndianU32(raw_frame.scale[j]);
            raw_frame.translate[j] = AlignedLittleEndianU32(raw_frame.translate[j]);
        }

        float *f_ptr = (float *)raw_frame.scale;

        float scale[3], translate[3];

        scale[0] = f_ptr[0];
        scale[1] = f_ptr[1];
        scale[2] = f_ptr[2];

        translate[0] = f_ptr[3];
        translate[1] = f_ptr[4];
        translate[2] = f_ptr[5];

        md->frames_[i].name = CopyFrameName(&raw_frame);

        f->Read(raw_verts, num_verts * sizeof(RawMD2Vertex));

        md->frames_[i].xs             = new float[num_verts];
        md->frames_[i].ys             = new float[num_verts];
        md->frames_[i].zs             = new float[num_verts];
        md->frames_[i].normal_indices = new short[num_verts];

        EPI_CLEAR_MEMORY(which_normals, uint8_t, kTotalMDFormatNormals);

        for (int v = 0; v < num_verts; v++)
        {
            RawMD2Vertex *raw_V = raw_verts + v;

            md->frames_[i].xs[v] = (int)raw_V->x * scale[0] + translate[0];
            md->frames_[i].ys[v] = (int)raw_V->y * scale[1] + translate[1];
            md->frames_[i].zs[v] = (int)raw_V->z * scale[2] + translate[2];

            short ni = raw_V->light_normal;

            EPI_ASSERT(ni >= 0);

            if (ni >= kTotalMDFormatNormals)
            {
                LogDebug("Vert %d of Frame %d has an invalid normal index: %d\n", v, i, (int)ni);
                ni = ni % kTotalMDFormatNormals;
            }

            md->frames_[i].normal_indices[v] = ni;
            which_normals[ni]                 = 1;

            HMM_Vec3 vr = {{md->frames_[i].xs[v], md->frames_[i].ys[v], md->frames_[i].zs[v]}};
            float    r  = HMM_Len(vr);

            if (r > radius)
                radius = r;
        }

        md->frames_[i].used_normals = CreateNormalList(which_normals);
    }

    delete[] raw_verts;

    return md;
}

short ModelFindFrame(ModelData *md, const char *name)
{
    EPI_ASSERT(strlen(name) > 0);

    for (int f = 0; f < md->total_frames_; f++)
    {
        ModelFrame *frame = &md->frames_[f];

        if (DDFCompareName(name, frame->name) == 0)
            return f;
    }

    return -1;
}

/*============== MD3 LOADING ====================*/

static uint8_t md3_normal_to_md2[128][128];
static bool    md3_normal_map_built = false;

static uint8_t MD2FindNormal(float x, float y, float z)
{
    int quadrant = 0;

    if (x < 0)
    {
        x = -x;
        quadrant |= 4;
    }
    if (y < 0)
    {
        y = -y;
        quadrant |= 2;
    }
    if (z < 0)
    {
        z = -z;
        quadrant |= 1;
    }

    int   best_g   = 0;
    float best_dot = -1;

    for (int i = 0; i < 27; i++)
    {
        int n = md_normal_groups[i][0];

        float nx = md_normals[n].X;
        float ny = md_normals[n].Y;
        float nz = md_normals[n].Z;

        float dot = (x * nx + y * ny + z * nz);

        if (dot > best_dot)
        {
            best_g   = i;
            best_dot = dot;
        }
    }

    return md_normal_groups[best_g][quadrant];
}

static void MD3CreateNormalMap(void)
{
    float sintab[160];

    for (int i = 0; i < 160; i++)
        sintab[i] = sin(i * HMM_PI / 64.0);

    for (int pitch = 0; pitch < 128; pitch++)
    {
        uint8_t *dest = &md3_normal_to_md2[pitch][0];

        for (int yaw = 0; yaw < 128; yaw++)
        {
            float z = sintab[pitch + 32];
            float w = sintab[pitch];

            float x = w * sintab[yaw + 32];
            float y = w * sintab[yaw];

            *dest++ = MD2FindNormal(x, y, z);
        }
    }

    md3_normal_map_built = true;
}

static ModelData *MD3Load(epi::File *f, float &radius)
{
    radius = 1;

    float *ff;

    if (!md3_normal_map_built)
        MD3CreateNormalMap();

    RawMD3Header header;

    f->Read(&header, sizeof(RawMD3Header));

    int version = AlignedLittleEndianS32(header.version);

    LogDebug("MODEL IDENT: [%c%c%c%c] VERSION: %d", header.ident[0], header.ident[1], header.ident[2], header.ident[3],
             version);

    if (strncmp(header.ident, kMD3Identifier, 4) != 0)
        FatalError("MD3LoadModel: file is not an MD3 model!");

    if (version != kMD3Version)
        FatalError("MD3LoadModel: strange version!");

    if (AlignedLittleEndianS32(header.num_meshes) > 1)
        LogWarning("Ignoring extra meshes in MD3 model.\n");

    int mesh_base = AlignedLittleEndianS32(header.ofs_meshes);

    f->Seek(mesh_base, epi::File::kSeekpointStart);

    RawMD3Mesh mesh;

    f->Read(&mesh, sizeof(RawMD3Mesh));

    int num_frames      = AlignedLittleEndianS32(mesh.num_frames);
    int num_verts       = AlignedLittleEndianS32(mesh.num_verts);
    int total_triangles = AlignedLittleEndianS32(mesh.num_tris);

    LogDebug("  frames:%d  verts:%d  triangles: %d\n", num_frames, num_verts, total_triangles);

    ModelData *md = new ModelData(num_frames, total_triangles * 3, total_triangles, num_verts);

    ModelPoint *temp_TEXC = new ModelPoint[num_verts];

    f->Seek(mesh_base + AlignedLittleEndianS32(mesh.ofs_texcoords), epi::File::kSeekpointStart);

    for (int i = 0; i < num_verts; i++)
    {
        RawMD3TextureCoordinate texc;

        f->Read(&texc, sizeof(RawMD3TextureCoordinate));

        texc.s = AlignedLittleEndianU32(texc.s);
        texc.t = AlignedLittleEndianU32(texc.t);

        ff                    = (float *)&texc.s;
        temp_TEXC[i].skin_s   = *ff;
        ff                    = (float *)&texc.t;
        temp_TEXC[i].skin_t   = 1.0f - *ff;
        temp_TEXC[i].vert_idx = i;
    }

    f->Seek(mesh_base + AlignedLittleEndianS32(mesh.ofs_tris), epi::File::kSeekpointStart);

    for (int i = 0; i < total_triangles; i++)
    {
        RawMD3Triangle tri;

        f->Read(&tri, sizeof(RawMD3Triangle));

        int a = AlignedLittleEndianU32(tri.index_xyz[0]);
        int b = AlignedLittleEndianU32(tri.index_xyz[1]);
        int c = AlignedLittleEndianU32(tri.index_xyz[2]);

        EPI_ASSERT(a < num_verts);
        EPI_ASSERT(b < num_verts);
        EPI_ASSERT(c < num_verts);

        ModelPoint *point = md->points_ + i * 3;

        point[0] = temp_TEXC[a];
        point[1] = temp_TEXC[b];
        point[2] = temp_TEXC[c];
    }

    delete[] temp_TEXC;

    f->Seek(mesh_base + AlignedLittleEndianS32(mesh.ofs_verts), epi::File::kSeekpointStart);

    uint8_t which_normals[kTotalMDFormatNormals];

    for (int i = 0; i < num_frames; i++)
    {
        md->frames_[i].xs             = new float[num_verts];
        md->frames_[i].ys             = new float[num_verts];
        md->frames_[i].zs             = new float[num_verts];
        md->frames_[i].normal_indices = new short[num_verts];

        EPI_CLEAR_MEMORY(which_normals, uint8_t, kTotalMDFormatNormals);

        for (int v = 0; v < num_verts; v++)
        {
            RawMD3Vertex vert;

            f->Read(&vert, sizeof(RawMD3Vertex));

            md->frames_[i].xs[v] = AlignedLittleEndianS16(vert.x) / 64.0f;
            md->frames_[i].ys[v] = AlignedLittleEndianS16(vert.y) / 64.0f;
            md->frames_[i].zs[v] = AlignedLittleEndianS16(vert.z) / 64.0f;

            short ni                         = md3_normal_to_md2[vert.pitch >> 1][vert.yaw >> 1];
            md->frames_[i].normal_indices[v] = ni;
            which_normals[ni]                = 1;

            HMM_Vec3 vr = {{md->frames_[i].xs[v], md->frames_[i].ys[v], md->frames_[i].zs[v]}};
            float    r  = HMM_Len(vr);

            if (r > radius)
                radius = r;
        }

        md->frames_[i].used_normals = CreateNormalList(which_normals);
    }

    f->Seek(AlignedLittleEndianS32(header.ofs_frames), epi::File::kSeekpointStart);

    for (int i = 0; i < num_frames; i++)
    {
        RawMD3Frame frame;

        f->Read(&frame, sizeof(RawMD3Frame));

        md->frames_[i].name = CopyFrameName(&frame);

        LogDebug("Frame %d = '%s'\n", i + 1, md->frames_[i].name);
    }

    return md;
}

/*============== MDL LOADING ====================*/

static ModelData *MDLLoad(epi::File *f, float &radius)
{
    radius = 1;

    RawMDLHeader header;

    f->Read(&header, sizeof(RawMDLHeader));

    int version = AlignedLittleEndianS32(header.version);

    LogDebug("MODEL IDENT: [%c%c%c%c] VERSION: %d", header.ident[0], header.ident[1], header.ident[2], header.ident[3],
             version);

    if (epi::StringPrefixCompare(header.ident, kMDLIdentifier) != 0)
        FatalError("MDL_LoadModel: lump is not an MDL model!");

    if (version != kMDLVersion)
        FatalError("MDL_LoadModel: strange version!");

    int num_frames = AlignedLittleEndianS32(header.num_frames);
    int num_tris   = AlignedLittleEndianS32(header.num_tris);
    int num_verts  = AlignedLittleEndianS32(header.num_verts);
    int swidth     = AlignedLittleEndianS32(header.skin_width);
    int sheight    = AlignedLittleEndianS32(header.skin_height);
    int num_points = num_tris * 3;

    ModelData *md = new ModelData(num_frames, num_points, num_tris, num_verts);

    for (int i = 0; i < AlignedLittleEndianS32(header.num_skins); i++)
    {
        int      group  = 0;
        uint8_t *pixels = new uint8_t[sheight * swidth];

        f->Read(&group, sizeof(int));
        if (AlignedLittleEndianS32(group))
            FatalError("MDL_LoadModel: Group skins unsupported!\n");

        f->Read(pixels, sheight * swidth * sizeof(uint8_t));

        ImageData *tmp_img = new ImageData(swidth, sheight, 3);

        for (int j = 0; j < swidth * sheight; ++j)
        {
            for (int k = 0; k < 3; ++k)
                tmp_img->pixels_[(j * 3) + k] = md_colormap[pixels[j]][k];
        }

        delete[] pixels;
        md->skin_id_list_.push_back(UploadTexture(tmp_img, kUploadMipMap | kUploadSmooth));
        delete tmp_img;
    }

    RawMDLTextureCoordinate *texcoords = new RawMDLTextureCoordinate[num_verts];
    f->Read(texcoords, num_verts * sizeof(RawMDLTextureCoordinate));

    RawMDLTriangle *tris = new RawMDLTriangle[num_tris];
    f->Read(tris, num_tris * sizeof(RawMDLTriangle));

    RawMDLFrame *frames = new RawMDLFrame[num_frames];

    for (int fr = 0; fr < num_frames; fr++)
    {
        frames[fr].frame.verts = new RawMDLVertex[num_verts];
        f->Read(&frames[fr].type, sizeof(int32_t));
        f->Read(&frames[fr].frame.bboxmin, sizeof(RawMDLVertex));
        f->Read(&frames[fr].frame.bboxmax, sizeof(RawMDLVertex));
        f->Read(frames[fr].frame.name, 16 * sizeof(char));
        f->Read(frames[fr].frame.verts, num_verts * sizeof(RawMDLVertex));
    }

    LogDebug("  frames:%d  points:%d  tris: %d\n", num_frames, num_points, num_tris);
    LogDebug("  vertices_per_frame_:%d\n", md->vertices_per_frame_);

    ModelPoint *point = md->points_;

    for (int i = 0; i < num_tris; i++)
    {
        EPI_ASSERT(point < md->points_ + md->total_points_);

        for (int j = 0; j < 3; j++, point++)
        {
            RawMDLTriangle raw_tri = tris[i];
            point->vert_idx        = AlignedLittleEndianS32(raw_tri.vertex[j]);
            float s                = (float)AlignedLittleEndianS16(texcoords[point->vert_idx].s);
            float t                = (float)AlignedLittleEndianS16(texcoords[point->vert_idx].t);
            if (!AlignedLittleEndianS32(raw_tri.facesfront) &&
                AlignedLittleEndianS32(texcoords[point->vert_idx].onseam))
                s += (float)swidth * 0.5f;
            point->skin_s = (s + 0.5f) / (float)swidth;
            point->skin_t = (t + 0.5f) / (float)sheight;
            EPI_ASSERT(point->vert_idx >= 0);
            EPI_ASSERT(point->vert_idx < md->vertices_per_frame_);
        }
    }

    EPI_ASSERT(point == md->points_ + md->total_points_);

    uint8_t which_normals[kTotalMDFormatNormals];

    uint32_t raw_scale[3];
    uint32_t raw_translate[3];

    raw_scale[0]     = AlignedLittleEndianU32(header.scale_x);
    raw_scale[1]     = AlignedLittleEndianU32(header.scale_y);
    raw_scale[2]     = AlignedLittleEndianU32(header.scale_z);
    raw_translate[0] = AlignedLittleEndianU32(header.trans_x);
    raw_translate[1] = AlignedLittleEndianU32(header.trans_y);
    raw_translate[2] = AlignedLittleEndianU32(header.trans_z);

    float *f_ptr = (float *)raw_scale;
    float  scale[3], translate[3];

    scale[0] = f_ptr[0];
    scale[1] = f_ptr[1];
    scale[2] = f_ptr[2];

    f_ptr        = (float *)raw_translate;
    translate[0] = f_ptr[0];
    translate[1] = f_ptr[1];
    translate[2] = f_ptr[2];

    for (int i = 0; i < num_frames; i++)
    {
        RawMDLFrame raw_frame = frames[i];

        md->frames_[i].name = CopyFrameName(&raw_frame.frame);

        RawMDLVertex *raw_verts = frames[i].frame.verts;

        md->frames_[i].xs             = new float[num_verts];
        md->frames_[i].ys             = new float[num_verts];
        md->frames_[i].zs             = new float[num_verts];
        md->frames_[i].normal_indices = new short[num_verts];

        EPI_CLEAR_MEMORY(which_normals, uint8_t, kTotalMDFormatNormals);

        for (int v = 0; v < num_verts; v++)
        {
            RawMDLVertex *raw_V = raw_verts + v;

            md->frames_[i].xs[v] = (int)raw_V->x * scale[0] + translate[0];
            md->frames_[i].ys[v] = (int)raw_V->y * scale[1] + translate[1];
            md->frames_[i].zs[v] = (int)raw_V->z * scale[2] + translate[2];

            short ni = raw_V->light_normal;

            EPI_ASSERT(ni >= 0);

            if (ni >= kTotalMDFormatNormals)
            {
                LogDebug("Vert %d of Frame %d has an invalid normal index: %d\n", v, i, (int)ni);
                ni = ni % kTotalMDFormatNormals;
            }

            md->frames_[i].normal_indices[v] = ni;
            which_normals[ni]                 = 1;

            HMM_Vec3 vr = {{md->frames_[i].xs[v], md->frames_[i].ys[v], md->frames_[i].zs[v]}};
            float    r  = HMM_Len(vr);

            if (r > radius)
                radius = r;
        }

        md->frames_[i].used_normals = CreateNormalList(which_normals);
    }

    delete[] texcoords;
    delete[] tris;
    for (int i = 0; i < num_frames; i++)
        delete[] frames[i].frame.verts;
    delete[] frames;

    return md;
}

ModelData *ModelLoad(epi::File *f, float &radius)
{
    char ident[4];

    f->Read(ident, 4);
    f->Seek(0, epi::File::kSeekpointStart);

    if (strncmp(ident, kMD3Identifier, 4) == 0)
        return MD3Load(f, radius);

    if (strncmp(ident, kMD2Identifier, 4) == 0)
        return MD2Load(f, radius);

    if (strncmp(ident, kMDLIdentifier, 4) == 0)
        return MDLLoad(f, radius);

    FatalError("ModelLoad: unrecognized model format [%c%c%c%c]\n", ident[0], ident[1], ident[2], ident[3]);
}

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
