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

//
// WindowsPerf package version string
//
// Given a version number MAJOR.MINOR.PATCH, increment the:
//
//     MAJOR version when you make incompatible API changes.
//     MINOR version when you add functionality in a backwards compatible manner.
//     PATCH version when you make backwards compatible bug fixes.
//

#define MAJOR 3
#define MINOR 7
#define PATCH 1

//
// WindowsPerf package version helper macros
//

// If you want to stringize the result of expansion of a macro argument, you have to use two levels of macros.
// See: https://gcc.gnu.org/onlinedocs/cpp/Stringizing.html
#define WPERF_XSTRING(s)  WPERF_STRING(s)
#define WPERF_STRING(s)   #s
//
// Supporting macros used to create static WindowsPerf Feature string based on macros defined during compilation
//
// Use ENABLE_FEAT_STR macro to get Feature String
// Use ENABLE_FEAT_SIZE macro to get Feature String size in bytes (including trailing '\0')
//
#define ENABLE_TRACING_FEAT                   "+trace"
#define ENABLE_ETW_TRACING_FEAT               "+etw-drv"
#define ENABLE_ETW_TRACING_APP_FEAT           "+etw-app"
#define ENABLE_SPE_FEAT                       "+spe"

// Generates WindowsPerf Features string, e.g. "+trace""+etw-drv""+etw-app"""
#define FEAT_XSTRING(s)                       FEAT_STRING(s)
#define FEAT_STRING(s)                        s

#define ENABLE_FEAT_JOIN(_1, _2, _3, _4, _5)  FEAT_XSTRING(_1) ## FEAT_XSTRING(_2) ## FEAT_XSTRING(_3) ## FEAT_XSTRING(_4) ## FEAT_XSTRING(_5)
#define ENABLE_FEAT_STR                       ENABLE_FEAT_JOIN(L"", ENABLE_TRACING_FEAT, ENABLE_ETW_TRACING_FEAT, ENABLE_ETW_TRACING_APP_FEAT, ENABLE_SPE_FEAT)
#define ENABLE_FEAT_SIZE                      sizeof ENABLE_FEAT_STR

/* Clear _FEAT defines to remove them from feature string*/
#ifndef ENABLE_TRACING
#undef ENABLE_TRACING_FEAT
#define ENABLE_TRACING_FEAT ""
#endif

#ifndef ENABLE_ETW_TRACING
#undef ENABLE_ETW_TRACING_FEAT
#define ENABLE_ETW_TRACING_FEAT	""
#endif

#ifndef ENABLE_ETW_TRACING_APP
#undef ENABLE_ETW_TRACING_APP_FEAT
#define ENABLE_ETW_TRACING_APP_FEAT ""
#endif

#ifndef ENABLE_SPE
#undef ENABLE_SPE_FEAT
#define ENABLE_SPE_FEAT	""
#endif

//
// Macros used to automatically update `wperf-driver/Resource.rc` macros such as:
// VER_PRODUCTVERSION or VER_PRODUCTVERSION_STR.
//

// VER_PRODUCTVERSION proxy, e.g. 3,4,0,0
#define WPERF_VER_PRODUCTVERSION(MA,MI,PA)          MA,MI,PA,0              // Comma separated version NUMBER LIST, with trailing "0"

// VER_PRODUCTVERSION_STR proxy, e.g. 3.4.0.0
#define WPERF_VER_PRODUCTVERSION_STR(MA,MI,PA)      MA "." MI "." PA ".0"   // Dot separated version STRING, with trailing "0"
