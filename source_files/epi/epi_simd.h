#pragma once

#include <math.h>
#include <stdint.h>
#include <string.h>

#if defined(__SSE2__) || (defined(_MSC_VER) && (defined(_M_AMD64) || defined(_M_X64) || (defined(_M_IX86_FP) && _M_IX86_FP >= 2)))
#define EPI_SIMD_SSE2
#include <emmintrin.h>
#endif

#if defined(__ARM_NEON) || (defined(_MSC_VER) && (defined(_M_ARM) || defined(_M_ARM64)))
#define EPI_SIMD_NEON
#include <arm_neon.h>
#endif

#if defined(__wasm_simd128__) && !defined(__SSE2__)
#define EPI_SIMD_WASM
#include <wasm_simd128.h>
#endif

namespace epi
{

#if defined(EPI_SIMD_SSE2)

typedef __m128  SimdF32x4;
typedef __m128i SimdI32x4;
typedef __m128i SimdI16x8;
typedef __m128i SimdU8x16;
typedef __m128d SimdF64x2;

#elif defined(EPI_SIMD_NEON)

typedef float32x4_t SimdF32x4;
typedef int32x4_t   SimdI32x4;
typedef int16x8_t   SimdI16x8;
typedef uint8x16_t  SimdU8x16;
#if defined(__aarch64__)
typedef float64x2_t SimdF64x2;
#else
struct SimdF64x2 { double v[2]; };
#endif

#elif defined(EPI_SIMD_WASM)

typedef v128_t SimdF32x4;
typedef v128_t SimdI32x4;
typedef v128_t SimdI16x8;
typedef v128_t SimdU8x16;
typedef v128_t SimdF64x2;

#else

struct SimdF32x4 { float   v[4]; };
struct SimdI32x4 { int32_t v[4]; };
struct SimdI16x8 { int16_t v[8]; };
struct SimdU8x16 { uint8_t v[16]; };
struct SimdF64x2 { double  v[2]; };

#endif

inline SimdF32x4 SplatF32x4(float x)
{
#if defined(EPI_SIMD_SSE2)
    return _mm_set1_ps(x);
#elif defined(EPI_SIMD_NEON)
    return vdupq_n_f32(x);
#elif defined(EPI_SIMD_WASM)
    return wasm_f32x4_splat(x);
#else
    SimdF32x4 r;
    r.v[0] = r.v[1] = r.v[2] = r.v[3] = x;
    return r;
#endif
}

inline SimdF32x4 SetF32x4(float x0, float x1, float x2, float x3)
{
#if defined(EPI_SIMD_SSE2)
    return _mm_set_ps(x3, x2, x1, x0);
#elif defined(EPI_SIMD_NEON)
    float tmp[4] = {x0, x1, x2, x3};
    return vld1q_f32(tmp);
#elif defined(EPI_SIMD_WASM)
    return wasm_f32x4_make(x0, x1, x2, x3);
#else
    SimdF32x4 r;
    r.v[0] = x0; r.v[1] = x1; r.v[2] = x2; r.v[3] = x3;
    return r;
#endif
}

inline SimdF32x4 LoadF32x4(const float *p)
{
#if defined(EPI_SIMD_SSE2)
    return _mm_loadu_ps(p);
#elif defined(EPI_SIMD_NEON)
    return vld1q_f32(p);
#elif defined(EPI_SIMD_WASM)
    return wasm_v128_load(p);
#else
    SimdF32x4 r;
    r.v[0] = p[0]; r.v[1] = p[1]; r.v[2] = p[2]; r.v[3] = p[3];
    return r;
#endif
}

inline void StoreF32x4(float *p, SimdF32x4 v)
{
#if defined(EPI_SIMD_SSE2)
    _mm_storeu_ps(p, v);
#elif defined(EPI_SIMD_NEON)
    vst1q_f32(p, v);
#elif defined(EPI_SIMD_WASM)
    wasm_v128_store(p, v);
#else
    p[0] = v.v[0]; p[1] = v.v[1]; p[2] = v.v[2]; p[3] = v.v[3];
#endif
}

inline void StoreU8x16(uint8_t *p, SimdU8x16 v)
{
#if defined(EPI_SIMD_SSE2)
    _mm_storeu_si128((__m128i *)p, v);
#elif defined(EPI_SIMD_NEON)
    vst1q_u8(p, v);
#elif defined(EPI_SIMD_WASM)
    wasm_v128_store(p, v);
#else
    for (int i = 0; i < 16; i++)
        p[i] = v.v[i];
#endif
}

inline float GetLane0F32x4(SimdF32x4 v)
{
#if defined(EPI_SIMD_SSE2)
    return _mm_cvtss_f32(v);
#elif defined(EPI_SIMD_NEON)
    return vgetq_lane_f32(v, 0);
#elif defined(EPI_SIMD_WASM)
    return wasm_f32x4_extract_lane(v, 0);
#else
    return v.v[0];
#endif
}

inline SimdF32x4 AddF32x4(SimdF32x4 a, SimdF32x4 b)
{
#if defined(EPI_SIMD_SSE2)
    return _mm_add_ps(a, b);
#elif defined(EPI_SIMD_NEON)
    return vaddq_f32(a, b);
#elif defined(EPI_SIMD_WASM)
    return wasm_f32x4_add(a, b);
#else
    SimdF32x4 r;
    for (int i = 0; i < 4; i++)
        r.v[i] = a.v[i] + b.v[i];
    return r;
#endif
}

inline SimdF32x4 SubF32x4(SimdF32x4 a, SimdF32x4 b)
{
#if defined(EPI_SIMD_SSE2)
    return _mm_sub_ps(a, b);
#elif defined(EPI_SIMD_NEON)
    return vsubq_f32(a, b);
#elif defined(EPI_SIMD_WASM)
    return wasm_f32x4_sub(a, b);
#else
    SimdF32x4 r;
    for (int i = 0; i < 4; i++)
        r.v[i] = a.v[i] - b.v[i];
    return r;
#endif
}

inline SimdF32x4 MulF32x4(SimdF32x4 a, SimdF32x4 b)
{
#if defined(EPI_SIMD_SSE2)
    return _mm_mul_ps(a, b);
#elif defined(EPI_SIMD_NEON)
    return vmulq_f32(a, b);
#elif defined(EPI_SIMD_WASM)
    return wasm_f32x4_mul(a, b);
#else
    SimdF32x4 r;
    for (int i = 0; i < 4; i++)
        r.v[i] = a.v[i] * b.v[i];
    return r;
#endif
}

inline SimdI32x4 CvtF32x4ToI32x4(SimdF32x4 a)
{
#if defined(EPI_SIMD_SSE2)
    return _mm_cvttps_epi32(a);
#elif defined(EPI_SIMD_NEON)
    return vcvtq_s32_f32(a);
#elif defined(EPI_SIMD_WASM)
    return wasm_i32x4_trunc_sat_f32x4(a);
#else
    SimdI32x4 r;
    for (int i = 0; i < 4; i++)
        r.v[i] = (int32_t)a.v[i];
    return r;
#endif
}

inline SimdI32x4 SplatI32x4(int32_t x)
{
#if defined(EPI_SIMD_SSE2)
    return _mm_set1_epi32(x);
#elif defined(EPI_SIMD_NEON)
    return vdupq_n_s32(x);
#elif defined(EPI_SIMD_WASM)
    return wasm_i32x4_splat(x);
#else
    SimdI32x4 r;
    r.v[0] = r.v[1] = r.v[2] = r.v[3] = x;
    return r;
#endif
}

template <int N>
inline SimdI32x4 ShiftLeftI32x4(SimdI32x4 v)
{
#if defined(EPI_SIMD_SSE2)
    return _mm_slli_epi32(v, N);
#elif defined(EPI_SIMD_NEON)
    return vshlq_n_s32(v, N);
#elif defined(EPI_SIMD_WASM)
    return wasm_i32x4_shl(v, N);
#else
    SimdI32x4 r;
    for (int i = 0; i < 4; i++)
        r.v[i] = v.v[i] << N;
    return r;
#endif
}

inline SimdI32x4 OrI32x4(SimdI32x4 a, SimdI32x4 b)
{
#if defined(EPI_SIMD_SSE2)
    return _mm_or_si128(a, b);
#elif defined(EPI_SIMD_NEON)
    return vorrq_s32(a, b);
#elif defined(EPI_SIMD_WASM)
    return wasm_v128_or(a, b);
#else
    SimdI32x4 r;
    for (int i = 0; i < 4; i++)
        r.v[i] = a.v[i] | b.v[i];
    return r;
#endif
}

inline void StoreI32x4(int32_t *p, SimdI32x4 v)
{
#if defined(EPI_SIMD_SSE2)
    _mm_storeu_si128((__m128i *)p, v);
#elif defined(EPI_SIMD_NEON)
    vst1q_s32(p, v);
#elif defined(EPI_SIMD_WASM)
    wasm_v128_store(p, v);
#else
    p[0] = v.v[0]; p[1] = v.v[1]; p[2] = v.v[2]; p[3] = v.v[3];
#endif
}

inline SimdI16x8 PackI32x4ToI16x8(SimdI32x4 a, SimdI32x4 b)
{
#if defined(EPI_SIMD_SSE2)
    return _mm_packs_epi32(a, b);
#elif defined(EPI_SIMD_NEON)
    return vcombine_s16(vqmovn_s32(a), vqmovn_s32(b));
#elif defined(EPI_SIMD_WASM)
    return wasm_i16x8_narrow_i32x4(a, b);
#else
    SimdI16x8 r;
    for (int i = 0; i < 4; i++)
    {
        int32_t va = a.v[i], vb = b.v[i];
        r.v[i]     = (int16_t)(va < -32768 ? -32768 : va > 32767 ? 32767 : va);
        r.v[i + 4] = (int16_t)(vb < -32768 ? -32768 : vb > 32767 ? 32767 : vb);
    }
    return r;
#endif
}

inline SimdU8x16 PackI16x8ToU8x16(SimdI16x8 a, SimdI16x8 b)
{
#if defined(EPI_SIMD_SSE2)
    return _mm_packus_epi16(a, b);
#elif defined(EPI_SIMD_NEON)
    return vcombine_u8(vqmovun_s16(a), vqmovun_s16(b));
#elif defined(EPI_SIMD_WASM)
    return wasm_u8x16_narrow_i16x8(a, b);
#else
    SimdU8x16 r;
    for (int i = 0; i < 8; i++)
    {
        int16_t va = a.v[i], vb = b.v[i];
        r.v[i]     = (uint8_t)(va < 0 ? 0 : va > 255 ? 255 : va);
        r.v[i + 8] = (uint8_t)(vb < 0 ? 0 : vb > 255 ? 255 : vb);
    }
    return r;
#endif
}

inline SimdF32x4 MinF32x4(SimdF32x4 a, SimdF32x4 b)
{
#if defined(EPI_SIMD_SSE2)
    return _mm_min_ps(a, b);
#elif defined(EPI_SIMD_NEON)
    return vminq_f32(a, b);
#elif defined(EPI_SIMD_WASM)
    return wasm_f32x4_min(a, b);
#else
    SimdF32x4 r;
    for (int i = 0; i < 4; i++)
        r.v[i] = a.v[i] < b.v[i] ? a.v[i] : b.v[i];
    return r;
#endif
}

inline SimdF32x4 MaxF32x4(SimdF32x4 a, SimdF32x4 b)
{
#if defined(EPI_SIMD_SSE2)
    return _mm_max_ps(a, b);
#elif defined(EPI_SIMD_NEON)
    return vmaxq_f32(a, b);
#elif defined(EPI_SIMD_WASM)
    return wasm_f32x4_max(a, b);
#else
    SimdF32x4 r;
    for (int i = 0; i < 4; i++)
        r.v[i] = a.v[i] > b.v[i] ? a.v[i] : b.v[i];
    return r;
#endif
}

inline SimdF64x2 SplatF64x2(double x)
{
#if defined(EPI_SIMD_SSE2)
    return _mm_set1_pd(x);
#elif defined(EPI_SIMD_NEON) && defined(__aarch64__)
    return vdupq_n_f64(x);
#elif defined(EPI_SIMD_WASM)
    return wasm_f64x2_splat(x);
#else
    SimdF64x2 r;
    r.v[0] = r.v[1] = x;
    return r;
#endif
}

inline SimdF64x2 SetF64x2(double x0, double x1)
{
#if defined(EPI_SIMD_SSE2)
    return _mm_set_pd(x1, x0);
#elif defined(EPI_SIMD_NEON) && defined(__aarch64__)
    double tmp[2] = {x0, x1};
    return vld1q_f64(tmp);
#elif defined(EPI_SIMD_WASM)
    return wasm_f64x2_make(x0, x1);
#else
    SimdF64x2 r;
    r.v[0] = x0;
    r.v[1] = x1;
    return r;
#endif
}

inline SimdF64x2 AddF64x2(SimdF64x2 a, SimdF64x2 b)
{
#if defined(EPI_SIMD_SSE2)
    return _mm_add_pd(a, b);
#elif defined(EPI_SIMD_NEON) && defined(__aarch64__)
    return vaddq_f64(a, b);
#elif defined(EPI_SIMD_WASM)
    return wasm_f64x2_add(a, b);
#else
    SimdF64x2 r;
    r.v[0] = a.v[0] + b.v[0];
    r.v[1] = a.v[1] + b.v[1];
    return r;
#endif
}

inline SimdF64x2 SubF64x2(SimdF64x2 a, SimdF64x2 b)
{
#if defined(EPI_SIMD_SSE2)
    return _mm_sub_pd(a, b);
#elif defined(EPI_SIMD_NEON) && defined(__aarch64__)
    return vsubq_f64(a, b);
#elif defined(EPI_SIMD_WASM)
    return wasm_f64x2_sub(a, b);
#else
    SimdF64x2 r;
    r.v[0] = a.v[0] - b.v[0];
    r.v[1] = a.v[1] - b.v[1];
    return r;
#endif
}

inline SimdF64x2 MulF64x2(SimdF64x2 a, SimdF64x2 b)
{
#if defined(EPI_SIMD_SSE2)
    return _mm_mul_pd(a, b);
#elif defined(EPI_SIMD_NEON) && defined(__aarch64__)
    return vmulq_f64(a, b);
#elif defined(EPI_SIMD_WASM)
    return wasm_f64x2_mul(a, b);
#else
    SimdF64x2 r;
    r.v[0] = a.v[0] * b.v[0];
    r.v[1] = a.v[1] * b.v[1];
    return r;
#endif
}

inline SimdF64x2 DivF64x2(SimdF64x2 a, SimdF64x2 b)
{
#if defined(EPI_SIMD_SSE2)
    return _mm_div_pd(a, b);
#elif defined(EPI_SIMD_NEON) && defined(__aarch64__)
    return vdivq_f64(a, b);
#elif defined(EPI_SIMD_WASM)
    return wasm_f64x2_div(a, b);
#else
    SimdF64x2 r;
    r.v[0] = a.v[0] / b.v[0];
    r.v[1] = a.v[1] / b.v[1];
    return r;
#endif
}

inline double GetLane0F64x2(SimdF64x2 v)
{
#if defined(EPI_SIMD_SSE2)
    return _mm_cvtsd_f64(v);
#elif defined(EPI_SIMD_NEON) && defined(__aarch64__)
    return vgetq_lane_f64(v, 0);
#elif defined(EPI_SIMD_WASM)
    return wasm_f64x2_extract_lane(v, 0);
#else
    return v.v[0];
#endif
}

inline double GetLane1F64x2(SimdF64x2 v)
{
#if defined(EPI_SIMD_SSE2)
    return _mm_cvtsd_f64(_mm_unpackhi_pd(v, v));
#elif defined(EPI_SIMD_NEON) && defined(__aarch64__)
    return vgetq_lane_f64(v, 1);
#elif defined(EPI_SIMD_WASM)
    return wasm_f64x2_extract_lane(v, 1);
#else
    return v.v[1];
#endif
}

inline SimdF64x2 PerpDistPairF64x2(double psx, double psy, double pex, double pey,
                                    double pdx, double pdy, double p_perp, double p_length)
{
    SimdF64x2 xs     = SetF64x2(psx, pex);
    SimdF64x2 ys     = SetF64x2(psy, pey);
    SimdF64x2 v_pdy  = SplatF64x2(pdy);
    SimdF64x2 v_pdx  = SplatF64x2(pdx);
    SimdF64x2 v_perp = SplatF64x2(p_perp);
    SimdF64x2 v_len  = SplatF64x2(p_length);
    return DivF64x2(AddF64x2(SubF64x2(MulF64x2(xs, v_pdy), MulF64x2(ys, v_pdx)), v_perp), v_len);
}

inline float RsqrtF32(float x)
{
#if defined(EPI_SIMD_SSE2)
    __m128 v   = _mm_set_ss(x);
    __m128 r   = _mm_rsqrt_ss(v);
    __m128 xrr = _mm_mul_ss(_mm_mul_ss(v, r), r);
    __m128 nr  = _mm_sub_ss(_mm_set_ss(3.0f), xrr);
    r          = _mm_mul_ss(_mm_mul_ss(r, nr), _mm_set_ss(0.5f));
    return _mm_cvtss_f32(r);
#elif defined(EPI_SIMD_NEON)
    float32x2_t v = vdup_n_f32(x);
    float32x2_t r = vrsqrte_f32(v);
    r             = vmul_f32(r, vrsqrts_f32(vmul_f32(v, r), r));
    return vget_lane_f32(r, 0);
#else
    return 1.0f / sqrtf(x);
#endif
}

inline SimdF32x4 RsqrtF32x4(SimdF32x4 v)
{
#if defined(EPI_SIMD_SSE2)
    __m128 r   = _mm_rsqrt_ps(v);
    __m128 xrr = _mm_mul_ps(_mm_mul_ps(v, r), r);
    __m128 nr  = _mm_sub_ps(_mm_set1_ps(3.0f), xrr);
    return _mm_mul_ps(_mm_mul_ps(r, nr), _mm_set1_ps(0.5f));
#elif defined(EPI_SIMD_NEON)
    float32x4_t r = vrsqrteq_f32(v);
    return vmulq_f32(r, vrsqrtsq_f32(vmulq_f32(v, r), r));
#elif defined(EPI_SIMD_WASM)
    return wasm_f32x4_div(wasm_f32x4_splat(1.0f), wasm_f32x4_sqrt(v));
#else
    SimdF32x4 r;
    for (int i = 0; i < 4; i++)
        r.v[i] = 1.0f / sqrtf(v.v[i]);
    return r;
#endif
}

inline void EnableFastFloats()
{
#if defined(EPI_SIMD_SSE2)
    _mm_setcsr(_mm_getcsr() | 0x8040);
#elif defined(EPI_SIMD_NEON)
#if defined(__aarch64__)
    uint64_t fpcr;
    __asm__ volatile("mrs %0, fpcr" : "=r"(fpcr));
    fpcr |= (1ULL << 24);
    __asm__ volatile("msr fpcr, %0" :: "r"(fpcr));
#elif defined(__arm__)
    uint32_t fpscr;
    __asm__ volatile("vmrs %0, fpscr" : "=r"(fpscr));
    fpscr |= (1U << 24);
    __asm__ volatile("vmsr fpscr, %0" :: "r"(fpscr));
#endif
#endif
}

inline void StoreLo64(uint8_t *p, SimdI32x4 v)
{
#if defined(EPI_SIMD_SSE2)
    _mm_storel_epi64((__m128i *)p, v);
#elif defined(EPI_SIMD_NEON)
    vst1_u32((uint32_t *)p, vreinterpret_u32_s32(vget_low_s32(v)));
#elif defined(EPI_SIMD_WASM)
    int64_t tmp = wasm_i64x2_extract_lane(v, 0);
    memcpy(p, &tmp, 8);
#else
    memcpy(p, v.v, 8);
#endif
}

inline void StoreHi64(uint8_t *p, SimdI32x4 v)
{
#if defined(EPI_SIMD_SSE2)
    _mm_storel_epi64((__m128i *)p, _mm_srli_si128(v, 8));
#elif defined(EPI_SIMD_NEON)
    vst1_u32((uint32_t *)p, vreinterpret_u32_s32(vget_high_s32(v)));
#elif defined(EPI_SIMD_WASM)
    int64_t tmp = wasm_i64x2_extract_lane(v, 1);
    memcpy(p, &tmp, 8);
#else
    memcpy(p, v.v + 2, 8);
#endif
}

} // namespace epi
