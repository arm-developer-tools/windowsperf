// BSD 3-Clause License
//
// Copyright (c) 2022, Arm Limited
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

#pragma once

#include <algorithm>
#include <iomanip>
#include <string>
#include <vector>

std::string MultiByteFromWideString(const wchar_t* wstr);
std::wstring IntToHexWideString(int Value, size_t Width = 4);
std::wstring DoubleToWideString(double Value, int Precision = 2);
std::wstring DoubleToWideStringExt(double Value, int Precision, int Width);

/// <summary>
/// Converts integer VALUE to decimal WSTRING, e.g. 123 -> "123"
/// </summary>
/// <param name="Value">Value to convert to hex string</param>
/// <param name="Width">Total digits to fill with</param>
template<typename T>
std::wstring IntToDecWideString(T Value, size_t Width) {
    static_assert(std::is_integral<T>::value, "Integral type required in Value<T>");
    std::wstringstream ss;
    ss << std::setw(Width) << Value;
    return std::wstring(ss.str());
}

/// <summary>
/// Function tokenizes string and returns vector in INT values.
/// Example string input:
///
///    L"0,2,3,5"
///
/// </summary>
/// <param name="Input">Input WSTRING</param>
/// <param name="Delimiter">Delimeter used to tokenize INPUT</param>
/// <param name="Output">Vector with tokenized values (is cleared by function)</param>
/// <returns>Count of elements tokenized</returns>
template<typename T>
bool TokenizeWideStringOfInts(_In_ std::wstring Input, _In_  const wchar_t Delimiter, _Out_ std::vector<T>& Output) {
    static_assert(std::is_integral<T>::value, "Integral type required in Output<T>");

    std::wstring token;
    std::wistringstream ss(Input);

    Output.clear();
    while (std::getline(ss, token, Delimiter)) {
        if (std::all_of(token.begin(), token.end(), ::isdigit))
            Output.push_back((T)_wtoi(token.c_str()));
        else
            return false;
    }

    return true;
}
