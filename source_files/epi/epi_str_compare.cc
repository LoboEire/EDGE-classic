//----------------------------------------------------------------------------
//  EPI String Comparison
//----------------------------------------------------------------------------
//
//  Copyright (c) 2022-2024 The EDGE Team.
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

#include "epi_str_compare.h"

#include <string.h>

#include "epi.h"
#include "epi_str_util.h"

namespace epi
{

int StringCompare(std::string_view A, std::string_view B)
{
    size_t min_len = A.size() < B.size() ? A.size() : B.size();
    int    result  = memcmp(A.data(), B.data(), min_len);
    if (result != 0)
        return result;
    if (A.size() < B.size())
        return -1;
    if (A.size() > B.size())
        return 1;
    return 0;
}

//----------------------------------------------------------------------------

int StringCompareMax(std::string_view A, std::string_view B, size_t n)
{
    EPI_ASSERT(n != 0);
    size_t a_len   = A.size() < n ? A.size() : n;
    size_t b_len   = B.size() < n ? B.size() : n;
    size_t min_len = a_len < b_len ? a_len : b_len;
    int    result  = memcmp(A.data(), B.data(), min_len);
    if (result != 0)
        return result;
    if (a_len < b_len)
        return -1;
    if (a_len > b_len)
        return 1;
    return 0;
}

//----------------------------------------------------------------------------

int StringCaseCompareASCII(std::string_view A, std::string_view B)
{
    size_t A_pos = 0;
    size_t B_pos = 0;
    size_t A_end = A.size();
    size_t B_end = B.size();

    for (;; A_pos++, B_pos++)
    {
        unsigned char AC = A_pos < A_end ? (unsigned char)ToLowerASCII((unsigned char)A[A_pos]) : 0;
        unsigned char BC = B_pos < B_end ? (unsigned char)ToLowerASCII((unsigned char)B[B_pos]) : 0;

        if (AC != BC)
            return AC - BC;

        if (A_pos == A_end)
            return 0;
    }
}

//----------------------------------------------------------------------------

int StringCaseCompareMaxASCII(std::string_view A, std::string_view B, size_t n)
{
    EPI_ASSERT(n != 0);
    size_t A_pos = 0;
    size_t B_pos = 0;
    size_t A_end = A.size();
    size_t B_end = B.size();

    for (;; A_pos++, B_pos++)
    {
        if (n == 0)
            return 0;

        unsigned char AC = A_pos < A_end ? (unsigned char)ToLowerASCII((unsigned char)A[A_pos]) : 0;
        unsigned char BC = B_pos < B_end ? (unsigned char)ToLowerASCII((unsigned char)B[B_pos]) : 0;

        if (AC != BC)
            return AC - BC;

        if (A_pos == A_end)
            return 0;

        n--;
    }
}

//----------------------------------------------------------------------------

int StringPrefixCompare(std::string_view A, std::string_view B)
{
    if (A.size() < B.size())
    {
        int result = memcmp(A.data(), B.data(), A.size());
        return result != 0 ? result : -1;
    }
    return memcmp(A.data(), B.data(), B.size());
}

//----------------------------------------------------------------------------

int StringPrefixCaseCompareASCII(std::string_view A, std::string_view B)
{
    size_t A_pos = 0;
    size_t B_pos = 0;
    size_t A_end = A.size();
    size_t B_end = B.size();

    for (;; A_pos++, B_pos++)
    {
        unsigned char AC = A_pos < A_end ? (unsigned char)ToLowerASCII((unsigned char)A[A_pos]) : 0;
        unsigned char BC = B_pos < B_end ? (unsigned char)ToLowerASCII((unsigned char)B[B_pos]) : 0;

        if (B_pos == B_end)
            return 0;

        if (AC != BC)
            return AC - BC;
    }
}

} // namespace epi

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
