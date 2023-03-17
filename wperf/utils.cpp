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

#include <iostream>
#include <string>
#include <sstream>
#include <stringapiset.h>
#include <vector>
#include "utils.h"

std::string MultiByteFromWideString(const wchar_t* wstr)
{
    if (!wstr)
        return std::string();
    int required_size = WideCharToMultiByte(CP_ACP, 0, wstr, -1, NULL, 0, NULL, NULL);
    std::shared_ptr<char[]> str_raw(new char[required_size]);
    WideCharToMultiByte(CP_ACP, 0, wstr, -1, str_raw.get(), required_size, NULL, NULL);
    return std::string(str_raw.get());
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
