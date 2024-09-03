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

#include <iostream>
#include <string>
#include <sstream>
#include <stringapiset.h>
#include <vector>
#include <cwctype>
#include <unordered_map>
#include <numeric>
#include "utils.h"

/// <summary>
/// Tokenizes wstring to a vector of wstring tokens delimeted by a specific character.
/// </summary>
/// <param name="str">Source string to get the tokens</param>
/// <param name="delim">The delimiting character</param>
/// <param name="tokens">The vector that is going to receive the tokens</param>
void TokenizeWideStringOfStrings(const std::wstring& str, const wchar_t& delim, std::vector<std::wstring>& tokens)
{
    using size_type = std::basic_string<wchar_t>::size_type;
    size_type pos = 0, last_pos = 0;
    pos = str.find(delim);
    while (pos != std::basic_string<wchar_t>::npos)
    {
        if (pos != last_pos)
        {
            std::wstring token = str.substr(last_pos, pos - last_pos);
            tokens.push_back(token);
        }
        last_pos = pos + 1;
        pos = str.find(delim, last_pos);
    }
    if (last_pos < str.size())
    {
        tokens.push_back(str.substr(last_pos, str.size() - last_pos + 1));
    }
}

void TokenizeWideStringOfStringsDelim(const std::wstring& str, const wchar_t& delim, std::vector<std::wstring>& tokens)
{
    using size_type = std::basic_string<wchar_t>::size_type;
    size_type pos = 0, last_pos = 0;
    pos = str.find(delim);
    while (pos != std::basic_string<wchar_t>::npos)
    {
        std::wstring token = str.substr(last_pos, pos - last_pos);
        token += delim;
        tokens.push_back(token);

        last_pos = pos + 1;
        pos = str.find(delim, last_pos);
    }
    if (last_pos < str.size())
    {
        tokens.push_back(str.substr(last_pos, str.size() - last_pos + 1));
    }
}


std::string MultiByteFromWideString(const wchar_t* wstr)
{
    if (!wstr)
        return std::string();
    int required_size = WideCharToMultiByte(CP_ACP, 0, wstr, -1, NULL, 0, NULL, NULL);
    std::shared_ptr<char[]> str_raw(new char[required_size]);
    WideCharToMultiByte(CP_ACP, 0, wstr, -1, str_raw.get(), required_size, NULL, NULL);
    return std::string(str_raw.get());
}

std::wstring TrimWideString(const std::wstring& wstr)
{
    if (wstr.size() == 0)
        return wstr;

    std::wstring::size_type i = 0, j = wstr.size()-1;
    
    while (i < wstr.size() && std::iswspace(wstr[i])) i++;
    while (j > 0 && std::iswspace(wstr[j])) j--;

    return wstr.substr(i, (j - i) + 1);
}

/// <summary>
/// Converts an array of chars into a wstring.
/// </summary>
/// <param name="str">Source char array to be converted</param>
std::wstring WideStringFromMultiByte(const char* str)
{
    if (!str)
        return std::wstring();
    int required_size = MultiByteToWideChar(CP_UTF8, 0, str, -1, NULL, 0);
    std::shared_ptr<wchar_t[]> wstr_raw(new wchar_t[required_size]);
    MultiByteToWideChar(CP_UTF8, 0, str, -1, wstr_raw.get(), required_size);
    return std::wstring(wstr_raw.get());
}

/// <summary>
/// Converts double to WSTRING. Function is using fixed format
/// and default precision set to 2.
/// </summary>
/// <param name="Value">Value to convert</param>
/// <param name="Precision">Precision</param>
std::wstring DoubleToWideString(double Value, int Precision) {
    std::wstringstream ss;
    ss << std::fixed << std::setprecision(Precision) << Value;
    return std::wstring(ss.str());
}

/// <summary>
/// Converts double to WSTRING. Function is using fixed format
/// and default precision set to 2.
/// </summary>
/// <param name="Value">Value to convert</param>
/// <param name="Precision">Precision</param>
/// <param name="Width">Total string width</param>
std::wstring DoubleToWideStringExt(double Value, int Precision, int Width) {
    std::wstringstream ss;
    ss << std::fixed << std::setw(Width) << std::setprecision(Precision) << Value;
    return std::wstring(ss.str());
}

std::wstring ReplaceFileExtension(std::wstring filename, std::wstring ext)
{
    size_t index = filename.find_last_of(L".");
    if (index != std::string::npos)
        filename = filename.substr(0, index) + L"." + ext;
    return filename;
}

/// <summary>
/// Case insensitive WSTRINGs comparision.
/// </summary>
/// <param name="str1">WSTRING to compare</param>
/// <param name="str2">WSTRING to compare</param>
bool CaseInsensitiveWStringComparision(const std::wstring& str1, const std::wstring& str2)
{
    return std::equal(str1.begin(), str1.end(),
        str2.begin(), str2.end(),
        [](wchar_t a, wchar_t b) { return towlower(a) == towlower(b); });
}

/// <summary>
/// Converst WSTRING to its lowercase counterpart
/// </summary>
/// <param name="str">WSTRING to lowercase</param>
std::wstring WStringToLower(const std::wstring& str)
{
    std::wstring result;
    result.resize(str.size());
    std::transform(str.begin(), str.end(), result.begin(), towlower);
    return result;
}

/// <summary>
/// Join WSTRINGs frm input vector
/// </summary>
/// <param name="str">Joined with SEP WSTRING</param>
std::wstring WStringJoin(const std::vector<std::wstring>& input, std::wstring sep)
{
    std::wstring first = input.size() ? input[0] : std::wstring();

    if (input.size() <= 1)
        return first;

    return std::accumulate(input.begin() + 1, input.end(), first,
        [&sep](const std::wstring& a, const std::wstring& b) -> std::wstring {
            return a + sep + b;
        });
}

/// <summary>
/// Return TRUE if STR starts with PREFIX
/// </summary>
/// <param name="str">String to compare</param>
/// <param name="prefix">Prefix to compare</param>
bool WStringStartsWith(const std::wstring& str, const std::wstring& prefix)
{
    return (str.size() > prefix.size() &&
        std::equal(prefix.begin(), prefix.end(), str.begin()));
}

/// <summary>
/// Return TRUE if STR ends with SUFFIX
/// </summary>
/// <param name="str">String to compare</param>
/// <param name="prefix">Prefix to compare</param>
bool WStringEndsWith(const std::wstring& str, const std::wstring& suffix)
{
    return (str.size() > suffix.size() &&
        std::equal(suffix.rbegin(), suffix.rend(), str.rbegin()));
}

/// <summary>
/// Return TRUE if STR starts with PREFIX (case insensitive comparision)
/// </summary>
/// <param name="str">String to compare</param>
/// <param name="prefix">Prefix to compare</param>
bool CaseInsensitiveWStringStartsWith(const std::wstring& str, const std::wstring& prefix)
{
    return WStringStartsWith(WStringToLower(str), WStringToLower(prefix));
}

/// <summary>
/// Return TRUE if STR starts with PREFIX (case insensitive comparision)
/// </summary>
/// <param name="str">String to compare</param>
/// <param name="prefix">Prefix to compare</param>
bool CaseInsensitiveWStringEndsWith(const std::wstring& str, const std::wstring& suffix)
{
    return WStringEndsWith(WStringToLower(str), WStringToLower(suffix));
}

/// <summary>
/// Replaces OLD_TOKEN with NEW_TOKEN
/// </summary>
/// <param name="input">Input string with OLD_TOKEN to replace</param>
/// <param name="old_token">Token to replace</param>
/// <param name="new_token">Replaces OLD_TOKEN in INPUT</param>
/// <returns>TRUE if OLD_TOKEN was replaced with NEW_TOKEN returns TRUE</returns>
bool ReplaceTokenInString(std::string& input, const std::string old_token, const std::string new_token)
{
    bool result = true;
    auto pos = input.find(old_token);

    if (pos != std::string::npos)
        input.replace(pos, old_token.size(), new_token);
    else
        result = false;

    return result;
}

/// <summary>
/// Convert time from one of [milliseconds, seconds, minutes, hours, days] to seconds
/// </summary> 
/// <param name="number">Input number to be converted</param>
/// <param name="unit">Unit of input number</param>
/// <param name="unitConversionMap">Map of wstring unit multiplier</param>
double ConvertNumberWithUnit(double number, std::wstring unit, const std::unordered_map<std::wstring, double>& unitConversionMap)
{
    //takes a number and a unit, multiplies the number by a value to obtain number in desired units
    auto it = unitConversionMap.find(unit);
    double multiplier = it->second;
    double result = number * multiplier;

    return result;
}

/// <summary>
/// Replace all OLD_TOKEN occurances with NEW_TOKEN
/// </summary>
/// <param name="str">Input to be converted</param>
/// <param name="old_token">What to replace</param>
/// <param name="new_token">What to replace with</param>
void ReplaceAllTokensInWString(std::wstring& str, const std::wstring& old_token, const std::wstring& new_token)
{
    size_t pos = 0;

    while ((pos = str.find(old_token, pos)) != std::wstring::npos)
    {
        str.replace(pos, old_token.size(), new_token);
        pos += new_token.size();
    }
}

/// <summary>
/// Merge directory name with file name and form full file path
/// Note: only WSTRING path support for Windows OS.
/// </summary>
/// <param name="dir_str">Prefix directory</param>
/// <param name="filename_str">filename to merge</param>
/// <returns>Merged path to the file</returns>
std::wstring GetFullFilePath(std::wstring dir_str, std::wstring filename_str)
{
    std::filesystem::path dir(dir_str);
    std::filesystem::path file(filename_str);
    std::filesystem::path full_path = dir / file;
    return full_path;
}
