#pragma once
// BSD 3-Clause License
//
// Copyright (c) 2024, Arm Limited
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// 1. Redistributions of source code must retain the above copyright notice, this
//    list of conditions and the following disclaimer.
//
// 2. Redistributions in binary form must reproduce the above copyright notice,
//    this list of conditions and the following disclaimer in the documentation
//    and/or other materials provided with the distribution.
//
// 3. Neither the name of the copyright holder nor the names of its
//    contributors may be used to endorse or promote products derived from
//    this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
// FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
// DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
// SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
// CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
// OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#define WIDEREAL(_V_) L##_V_
#define WIDE(_V_) WIDEREAL(_V_)

#define LITERALCONSTANTS_GET(_V) static_cast<CharType *>(LiteralConstants<CharType>::get(_V, WIDE(_V)))

/*
LiteralConstants define handful constants to use throughout the code, since the Char type is a template you can 
have both wide strings as well as normal strings if needed without the additional hassle to declare both versions and 
in case you need just a single type the other constants won't get compiled.

The LITERALCONSTANTS_GET macro is just a quick macro to use the get static member function, it can be used outside LiteralConstants as long
as CharType is defined in the context.
*/
template <typename CharType=char>
struct LiteralConstants
{
    inline constexpr static CharType* get(char* v, wchar_t* wv)
    {
        if constexpr(std::is_same<CharType, char>::value)
        {
            return v;
        } else {
            return wv;
        }
    }

    inline constexpr static CharType* m_comma = LITERALCONSTANTS_GET(",");
    inline constexpr static CharType* m_quotes = LITERALCONSTANTS_GET("\"");
    inline constexpr static CharType* m_colon = LITERALCONSTANTS_GET(":");
    inline constexpr static CharType* m_cbracket_open = LITERALCONSTANTS_GET("{");
    inline constexpr static CharType* m_cbracket_close = LITERALCONSTANTS_GET("}");
    inline constexpr static CharType* m_bracket_open = LITERALCONSTANTS_GET("[");
    inline constexpr static CharType* m_bracket_close = LITERALCONSTANTS_GET("]");
    inline constexpr static CharType* m_space = LITERALCONSTANTS_GET(" ");
    inline constexpr static CharType* m_equal = LITERALCONSTANTS_GET("=");
};
