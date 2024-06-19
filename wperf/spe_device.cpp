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

#include "spe_device.h"

spe_device::spe_device() {}

spe_device::~spe_device() {}

void spe_device::init()
{

}

std::wstring spe_device::get_spe_version_name(UINT64 id_aa64dfr0_el1_value)
{
    UINT8 aa64_pms_ver = ID_AA64DFR0_EL1_PMSVer(id_aa64dfr0_el1_value);

    // Print SPE feature version
    std::wstring spe_str = L"unknown SPE configuration!";
    switch (aa64_pms_ver)
    {
    case 0b000: spe_str = L"not implemented"; break;
    case 0b001: spe_str = L"FEAT_SPE"; break;
    case 0b010: spe_str = L"FEAT_SPEv1p1"; break;
    case 0b011: spe_str = L"FEAT_SPEv1p2"; break;
    case 0b100: spe_str = L"FEAT_SPEv1p3"; break;
    }
    return spe_str;
}

bool spe_device::is_spe_supported(UINT64 id_aa64dfr0_el1_value)
{
    UINT8 aa64_pms_ver = ID_AA64DFR0_EL1_PMSVer(id_aa64dfr0_el1_value);
    return aa64_pms_ver >= 0b001;
}
