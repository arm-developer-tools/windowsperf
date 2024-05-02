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

#pragma once

#include <algorithm>
#include <iomanip>
#include <string>
#include <type_traits>
#include <vector>
#include <unordered_map>

std::string MultiByteFromWideString(const wchar_t* wstr);
std::wstring TrimWideString(const std::wstring& wstr);
std::wstring WideStringFromMultiByte(const char* str);
std::wstring DoubleToWideString(double Value, int Precision = 2);
std::wstring DoubleToWideStringExt(double Value, int Precision, int Width);
std::wstring ReplaceFileExtension(std::wstring filename, std::wstring ext);
std::wstring WStringToLower(const std::wstring& str);
bool WStringStartsWith(const std::wstring& str, const std::wstring& prefix);
bool CaseInsensitiveWStringStartsWith(const std::wstring& str, const std::wstring& prefix);
bool CaseInsensitiveWStringComparision(const std::wstring& str1, const std::wstring& str2);
bool ReplaceTokenInString(std::string& input, const std::string old_token, const std::string new_token);
void TokenizeWideStringOfStrings(const std::wstring& str, const wchar_t& delim, std::vector<std::wstring>& tokens);
double ConvertNumberWithUnit(double number, std::wstring unit, const std::unordered_map<std::wstring, double>& unitConversionMap);

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
/// Converts integer VALUE to hex WSTRING, e.g. 100 -> "0x0064" where
/// WIDTH is total digit count (excluding 0x).
/// </summary>
/// <param name="Value">Value to convert to hex string</param>
/// <param name="Width">Total digits to fill with</param>
template<typename T>
std::wstring IntToHexWideString(T Value, size_t Width = 4) {
    static_assert(std::is_integral<T>::value, "Integral type required in Value<T>");
    std::wstringstream ss;
    if (std::is_same<T, wchar_t>::value)
        ss << L"0x" << std::setfill(L'0') << std::setw(Width) << std::hex << (uint32_t)Value;
    else
        ss << L"0x" << std::setfill(L'0') << std::setw(Width) << std::hex << Value;
    return std::wstring(ss.str());
}

/// <summary>
/// Converts integer VALUE to hex WSTRING, e.g. 100 -> "0064" where
/// WIDTH is total digit count (excluding 0x).
/// </summary>
/// <param name="Value">Value to convert to hex string</param>
/// <param name="Width">Total digits to fill with</param>
template<typename T>
std::wstring IntToHexWideStringNoPrefix(T Value, size_t Width = 4) {
    static_assert(std::is_integral<T>::value, "Integral type required in Value<T>");
    std::wstringstream ss;
    if (std::is_same<T, wchar_t>::value)
        ss << std::setfill(L'0') << std::setw(Width) << std::hex << (uint32_t)Value;
    else
        ss << std::setfill(L'0') << std::setw(Width) << std::hex << Value;
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

/// <summary>
/// Converts integer VALUE to string separated with commas ','
/// Example:
///     1234567 -> 1,234,567
///     -1234567 -> -1,234,567
/// </summary>
/// <param name="Value">Value to convert to comma separated integer STRING</param>
template<typename T>
std::wstring IntToDecWithCommas(T Value) {
    static_assert(std::is_integral<T>::value, "Integral type required in Output<T> of " __FUNCTION__);

    std::wstring result;
    std::wstring str = std::to_wstring(Value);

    int count = 0;
    for (auto i = str.rbegin(); i != str.rend(); i++, count++)
    {
        if (count && (count % 3 == 0) && *i != L'-')
            result.push_back(L',');
        result.push_back(*i);
    }

    std::reverse(result.begin(), result.end());
    return result;
}
