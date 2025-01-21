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

#include <windows.h>
#include <vector>
#include <map>
#include <string>
#include "wperf-common/macros.h"
#include "wperf-common/iorequest.h"
#include "utils.h"


class spe_device
{
public:
    spe_device();
    ~spe_device();

    void init();

    // Consts

    // All availabkle filters for SPE `arm_spe_0//`
    static const std::vector<std::wstring> m_filter_names;

    // Filters also have aliases, this structure helps to translate alias to filter name
    static const std::map<std::wstring, std::wstring> m_filter_names_aliases;

    // Filter names have also short descriptions
    static const std::map<std::wstring, std::wstring> spe_device::m_filter_names_description;

    // Helper functions

    static std::wstring get_spe_version_name(UINT64 id_aa64dfr0_el1_value);
    static bool is_spe_supported(UINT64 id_aa64dfr0_el1_value);
    static void get_samples(const std::vector<UINT8>& spe_buffer, std::vector<FrameChain>& raw_samples, std::map<UINT64, std::wstring>& spe_events);

    static bool is_filter_name(std::wstring fname) {
        if (is_filter_name_alias(fname))
            fname = m_filter_names_aliases.at(fname);
        return std::find(m_filter_names.begin(), m_filter_names.end(), fname) != m_filter_names.end();
    }

    static bool is_filter_name_alias(std::wstring fname) {
        return m_filter_names_aliases.count(fname);
    }

    static uint64_t max_filter_val(std::wstring fname)
    {
        if (get_filter_name(fname) == L"min_latency")
            return SPE_CTL_FLAG_VAL_MASK;  // PMSLATFR_EL1, Sampling Latency Filter Register, MINLAT, bits [15:0]
        else if (get_filter_name(fname) == L"period")
            return SPE_CTL_INTERVAL_VAL_MASK;   // PMSIRR_EL1, Sampling Interval Reload Register, INTERVAL, bits [31:8]
        return 1;
    }

    static std::wstring get_filter_name(std::wstring fname);
};
