// AlmostEquals is derived from the Google C++ Testing Framework (Google Test):
//
// Copyright 2005, Google Inc.
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
//     * Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above
// copyright notice, this list of conditions and the following disclaimer
// in the documentation and/or other materials provided with the
// distribution.
//     * Neither the name of Google Inc. nor the names of its
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR
// PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
// CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
// ULP comparison algorithm reference:
// https://randomascii.wordpress.com/2012/02/25/comparing-floating-point-numbers-2012-edition/

#pragma once

#include <stdint.h>
#include <string.h>

namespace epi
{

constexpr uint32_t kAlmostEqualsMaxUlps = 4;

inline bool AlmostEquals(float first, float second)
{
    uint32_t bits_first;
    uint32_t bits_second;
    memcpy(&bits_first, &first, sizeof(uint32_t));
    memcpy(&bits_second, &second, sizeof(uint32_t));

    constexpr uint32_t kExponentMask = 0x7F800000u;
    constexpr uint32_t kAbsMask      = 0x7FFFFFFFu;

    if ((bits_first  & kAbsMask) > kExponentMask) return false;
    if ((bits_second & kAbsMask) > kExponentMask) return false;

    constexpr uint32_t kSignBit = 0x80000000u;

    uint32_t biased_first  = (bits_first  & kSignBit) ? (~bits_first  + 1) : (kSignBit | bits_first);
    uint32_t biased_second = (bits_second & kSignBit) ? (~bits_second + 1) : (kSignBit | bits_second);
    uint32_t distance      = (biased_first >= biased_second) ? (biased_first  - biased_second)
                                                             : (biased_second - biased_first);

    return distance <= kAlmostEqualsMaxUlps;
}

inline bool AlmostEquals(double first, double second)
{
    uint64_t bits_first;
    uint64_t bits_second;
    memcpy(&bits_first, &first, sizeof(uint64_t));
    memcpy(&bits_second, &second, sizeof(uint64_t));

    constexpr uint64_t kExponentMask = 0x7FF0000000000000ull;
    constexpr uint64_t kAbsMask      = 0x7FFFFFFFFFFFFFFFull;

    if ((bits_first  & kAbsMask) > kExponentMask) return false;
    if ((bits_second & kAbsMask) > kExponentMask) return false;

    constexpr uint64_t kSignBit = 0x8000000000000000ull;

    uint64_t biased_first  = (bits_first  & kSignBit) ? (~bits_first  + 1) : (kSignBit | bits_first);
    uint64_t biased_second = (bits_second & kSignBit) ? (~bits_second + 1) : (kSignBit | bits_second);
    uint64_t distance      = (biased_first >= biased_second) ? (biased_first  - biased_second)
                                                             : (biased_second - biased_first);

    return distance <= kAlmostEqualsMaxUlps;
}

class RNG
{
public:
    void     Init(uint64_t seed)   { i_ = kWeyl * seed; }
    void     Set(uint64_t pos)     { i_ = kWeyl * pos; }
    void     Seek(int64_t offset)  { i_ += kWeyl * (uint64_t)offset; }
    uint64_t Tell() const          { return i_ * kWeylI; }
    uint64_t Peek() const          { return Mix(i_); }

    static uint64_t At(uint64_t n) { return Mix(kWeyl * n); }

    uint64_t Next()
    {
        uint64_t i = i_;
        uint64_t r = Mix(i);
        i_         = i + kWeyl;
        return r;
    }
    uint64_t Prev()
    {
        uint64_t i = i_;
        uint64_t r = Mix(i);
        i_         = i - kWeyl;
        return r;
    }

private:
    static uint64_t Mix(uint64_t x)
    {
        x ^= (x >> 31);
        x *= UINT64_C(0x7fb5d329728ea185);
        x ^= (x >> 27);
        x *= UINT64_C(0x81dadef4bc2dd44d);
        x ^= (x >> 33);
        return x;
    }

    static constexpr uint64_t kWeyl  = UINT64_C(0x61c8864680b583eb);
    static constexpr uint64_t kWeylI = UINT64_C(0x0e217c1e66c88cc3);

    uint64_t i_ = 0;
};

} // namespace epi

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
