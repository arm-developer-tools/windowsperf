#pragma once
// BSD 3-Clause License
//
// Copyright (c) 2023, Arm Limited
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

#include <windows.h>
#include <map>
#include <string>
#include <variant>
#include <vector>
#include "wperf-common/macros.h"


namespace drvconfig {

    extern std::map<std::wstring, struct property> data;

    enum access
    {
        DRVCONFIG_RO,
        DRVCONFIG_RW,
    };

    struct property {
        std::variant<LONG, std::wstring> value;
        enum access access;
        std::wstring unit;
    };

    /* Interface to configuration */
    void init();
    bool set(std::wstring config_str);
    bool set(std::wstring name, std::wstring value);
    void get_configs(std::vector<std::wstring>& config_strs);
    template<typename T>
    bool get(std::wstring name, T& value)
    {
        if (std::holds_alternative<T>(data[name].value)) {
            value = std::get<T>(data[name].value);
            return true;
        }
        return false;
    }
}
