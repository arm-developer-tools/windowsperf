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
#include <filesystem>
#include <iomanip>
#include <string>
#include <type_traits>
#include <vector>
#include <unordered_map>
#include <regex>

std::string MultiByteFromWideString(const wchar_t* wstr);
std::wstring TrimWideString(const std::wstring& wstr);
std::wstring WideStringFromMultiByte(const char* str);
std::wstring DoubleToWideString(double Value, int Precision = 2);
std::wstring DoubleToWideStringExt(double Value, int Precision, int Width);
std::wstring ReplaceFileExtension(std::wstring filename, std::wstring ext);
std::wstring WStringToLower(const std::wstring& str);
std::wstring WStringJoin(const std::vector<std::wstring>& input, std::wstring sep);
bool WStringStartsWith(const std::wstring& str, const std::wstring& prefix);
bool WStringEndsWith(const std::wstring& str, const std::wstring& suffix);
bool CaseInsensitiveWStringStartsWith(const std::wstring& str, const std::wstring& prefix);
bool CaseInsensitiveWStringEndsWith(const std::wstring& str, const std::wstring& suffix);
bool CaseInsensitiveWStringComparision(const std::wstring& str1, const std::wstring& str2);
bool ReplaceTokenInString(std::string& input, const std::string old_token, const std::string new_token);
void TokenizeWideStringOfStrings(const std::wstring& str, const wchar_t& delim, std::vector<std::wstring>& tokens);
void TokenizeWideStringOfStringsDelim(const std::wstring& str, const wchar_t& delim, std::vector<std::wstring>& tokens);
double ConvertNumberWithUnit(double number, std::wstring unit, const std::unordered_map<std::wstring, double>& unitConversionMap);
void ReplaceAllTokensInWString(std::wstring& str, const std::wstring& old_token, const std::wstring& new_token);
std::wstring GetFullFilePath(std::wstring dir_str, std::wstring filename_str);

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

    std::wstring regex_string = L"^[0-9]+\\-[0-9]+$";
    std::wregex r(regex_string);
    std::wsmatch match;

    Output.clear();
    while (std::getline(ss, token, Delimiter)) {
        if (std::all_of(token.begin(), token.end(), ::isdigit))
            Output.push_back((T)_wtoi(token.c_str()));
        else if (std::regex_search(token, match, r)) {              //enables the function to accept different input formats, here: ranges e.g 1-3 = 1,2,3
            TokenizeWideStringofIntRange(token, L'-', Output);
        }
        else
            return false;
    }

    return true;
}

/// <summary>
/// Function tokenizes string which contains a range, returning the validity of the string (and the output vector)
/// Example Input:
/// 
///     L"0-4"
/// 
/// </summary>
/// <typeparam name="T"> INT</typeparam>
/// <param name="Range">Input Range as a wide string</param>
/// <param name="Separator">Wide Character to seperate range, e.g. L'-' </param>
/// <param name="Output">Output Vector</param>
/// <returns></returns>
template<typename T>
_Success_(return) bool TokenizeWideStringofIntRange(_In_ std::wstring Range, _In_ wchar_t Separator, _Out_ std::vector<T>& Output) {

    auto dash_pos = Range.find(Separator);

    if (dash_pos != std::wstring::npos) {
        std::wstring startWString = Range.substr(0, dash_pos);
        std::wstring endWString = Range.substr(dash_pos + 1);

        if (std::all_of(startWString.begin(), startWString.end(), ::isdigit) && std::all_of(endWString.begin(), endWString.end(), ::isdigit)) {
            
            T startNum = (T)_wtoi(startWString.c_str());
            T endNum = (T)_wtoi(endWString.c_str());

            T lowerBound = startNum < endNum ? startNum : endNum;
            T upperBound = startNum < endNum ? endNum : startNum;

            for (T i = lowerBound; i <= upperBound; i++) {
                Output.push_back((i));
            }
        }
        else
            return false;
    }
    else
        return false;

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

/// <summary>
/// Replaces all occurances of OLD_TOKEN with NEW_TOKEN
/// </summary>
/// <param name="input">Input string with OLD_TOKEN to replace</param>
/// <param name="old_token">Token to replace</param>
/// <param name="new_token">Replaces all OLD_TOKEN in INPUT</param>
/// <returns>How many tokens were replaces</returns>
template<typename T>
T ReplaceAllTokensInString(T input, const T old_token, const T new_token) {
    static_assert(std::is_same<decltype(input), std::string>::value
        || std::is_same<decltype(input), std::wstring>::value, "(W)String type required in input<T>");
    static_assert(std::is_same<decltype(old_token), const std::string>::value
        || std::is_same<decltype(old_token), const std::wstring>::value, "(W)String type required in old_token<T>");
    static_assert(std::is_same<decltype(new_token), const std::string>::value
        || std::is_same<decltype(new_token), const std::wstring>::value, "(W)String type required in new_token<T>");


    size_t pos = 0;

    while ((pos = input.find(old_token, pos)) != T::npos)
    {
        input.replace(pos, old_token.size(), new_token);
        pos += new_token.size();
    }

    return input;
}
