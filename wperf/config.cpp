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

#include <algorithm>
#include "config.h"


namespace drvconfig {

    std::map<std::wstring, struct property> data;

    void init()
    {
        data.clear();

        // Read-write configuration values
        data[std::wstring(L"count.period")] = { PMU_CTL_START_PERIOD, DRVCONFIG_RW, std::wstring(L"ms") };

        // Read-only configuration values
        data[std::wstring(L"count.period_max")] = { PMU_CTL_START_PERIOD, DRVCONFIG_RO, std::wstring(L"ms") };
        data[std::wstring(L"count.period_min")] = { PMU_CTL_START_PERIOD_MIN, DRVCONFIG_RO, std::wstring(L"ms") };
    }

    bool set(std::wstring name, std::wstring value)
    {
        for (auto& [key, config] : data)
            if (key == name)
            {
                if (config.access == DRVCONFIG_RO)
                    return false;

                if (config.access == DRVCONFIG_RW)
                {
                    if (std::holds_alternative<LONG>(config.value))
                        config.value = std::stol(value);
                    else if (std::holds_alternative<std::wstring>(config.value))
                        config.value = value;
                    else
                        return false;
                }

                return true;
            }

        return false;
    }

    // Set config from wide-string L"NAME=VALUE"
    bool set(std::wstring config_str)
    {
        auto it = std::find(config_str.begin(), config_str.end(), L'=');

        if (it != config_str.end() &&
            std::distance(it, config_str.end()) > 1)
        {
            std::wstring name = std::wstring(config_str.begin(), it);
            std::wstring value = std::wstring(it + 1, config_str.end());

            return set(name, value);
        }

        return false;
    }

    void get_configs(std::vector<std::wstring>& config_strs)
    {
        config_strs.clear();
        for (const auto& [key, value] : data)
            config_strs.push_back(key);

        std::sort(config_strs.begin(), config_strs.end());
    }
}