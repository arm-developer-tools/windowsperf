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

#include <DriverSpecs.h>
_Analysis_mode_(_Analysis_code_type_user_code_)

#define INITGUID

#include <windows.h>
#include <strsafe.h>
#include <set>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <numeric>
#include "wperf.h"
#include "debug.h"
#include "wperf-common\public.h"
#include "wperf-common\macros.h"
#include "wperf-common\iorequest.h"
#include "utils.h"
#include "output.h"
#include "exception.h"
#include "pe_file.h"
#include "process_api.h"
#include "events.h"

//
// Port start
//

#include <algorithm>
#include <exception>
#include <fstream>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <vector>
#include <map>
#include <deque>
#include <ctime>

#include <tchar.h>
#include <stdio.h>
#include <psapi.h>
#include <cfgmgr32.h>
#include <devpkey.h>

#include "pmu_device.h"
#include "user_request.h"


WCHAR G_DevicePath[MAX_DEVPATH_LENGTH];

#pragma comment (lib, "cfgmgr32.lib")
#pragma comment (lib, "User32.lib")
#pragma comment (lib, "ole32.lib")
#pragma comment (lib, "oleaut32.lib")

static bool no_ctrl_c = true;

static BOOL WINAPI ctrl_handler(DWORD dwCtrlType)
{
    switch (dwCtrlType)
    {
    case CTRL_C_EVENT:
        no_ctrl_c = false;
        m_out.GetOutputStream() << L"Ctrl-C received, quit counting...";
        return TRUE;
    case CTRL_BREAK_EVENT:
    default:
        m_out.GetErrorOutputStream() << L"unsupported dwCtrlType " << dwCtrlType << std::endl;
        return FALSE;
    }
}

//
// Port End
//
int __cdecl
wmain(
    _In_ int argc,
    _In_reads_(argc) wchar_t* argv[]
    )
{
    HANDLE hDevice = INVALID_HANDLE_VALUE;
    auto exit_code = EXIT_SUCCESS;

    if (argc < 2)
    {
        user_request::print_help();
        return EXIT_SUCCESS;
    }

    if ( !GetDevicePath(
            (LPGUID) &GUID_DEVINTERFACE_WINDOWSPERF,
            G_DevicePath,
            sizeof(G_DevicePath)/sizeof(G_DevicePath[0])) )
    {
        WindowsPerfDbgPrint("Error: Failed to find device path. GetLastError=%d\n", GetLastError());
        return EXIT_FAILURE;
    }

    hDevice = CreateFile(G_DevicePath,
                         GENERIC_READ|GENERIC_WRITE,
                         FILE_SHARE_READ | FILE_SHARE_WRITE,
                         NULL,
                         OPEN_EXISTING,
                         0,
                         NULL );

    if (hDevice == INVALID_HANDLE_VALUE) {
        WindowsPerfDbgPrint("Error: Failed to open device. GetLastError=%d\n",GetLastError());
        return EXIT_FAILURE;
    }

    //
    // Port Begin
    //
    user_request request;
    pmu_device pmu_device;
    wstr_vec raw_args;

    try{
        pmu_device.init(hDevice);
    } catch(std::exception&) {
        exit_code = EXIT_FAILURE;
        goto clean_exit;
    }

    for (int i = 1; i < argc; i++)
        raw_args.push_back(argv[i]);

    try{
        struct pmu_device_cfg pmu_cfg;
        pmu_device.get_pmu_device_cfg(pmu_cfg);
        request.init(raw_args, pmu_cfg, pmu_device.builtin_metrics);
        pmu_device.do_verbose = request.do_verbose;
    } catch(std::exception&)
    {
        exit_code = EXIT_FAILURE;
        goto clean_exit;
    }

    if (request.do_help)
    {
        user_request::print_help();
        goto clean_exit;
    }

    uint32_t enable_bits = 0;
    for (const auto& a : request.ioctl_events)
    {
        if (a.first == EVT_CORE)
        {
            enable_bits |= CTL_FLAG_CORE;
        }
        else if (a.first == EVT_DSU)
        {
            enable_bits |= CTL_FLAG_DSU;
        }
        else if (a.first == EVT_DMC_CLK || a.first == EVT_DMC_CLKDIV2)
        {
            enable_bits |= CTL_FLAG_DMC;
        }
        else
        {
            m_out.GetErrorOutputStream() << L"Unrecognized EVT_CLASS when mapping enable_bits: " << a.first << "\n";
            exit_code = EXIT_FAILURE;
            goto clean_exit;
        }
    }

    version_info driver_ver;
    pmu_device.version_query(driver_ver);

    if (request.do_version)
    {
        std::vector<std::wstring> col_component, col_version;
        col_component.push_back(L"wperf");
        col_version.push_back(std::to_wstring(MAJOR) + L"." +
                              std::to_wstring(MINOR) + L"." +
                              std::to_wstring(PATCH));
        col_component.push_back(L"wperf-driver");
        col_version.push_back(std::to_wstring(driver_ver.major) + L"." +
                              std::to_wstring(driver_ver.minor) + L"." +
                              std::to_wstring(driver_ver.patch));
        TableOutputL table(m_outputType);
        table.PresetHeaders<VersionOutputTraitsL>();
        table.Insert(col_component, col_version);
        m_out.Print(table, true);
        goto clean_exit;
    }

    if (driver_ver.major != MAJOR || driver_ver.minor != MINOR
        || driver_ver.patch != PATCH)
    {
        m_out.GetErrorOutputStream() << L"Version mismatch between wperf-driver and wperf.\n";
        m_out.GetErrorOutputStream() << L"wperf-driver version: " << driver_ver.major << "."
                   << driver_ver.minor << "." << driver_ver.patch << "\n";
        m_out.GetErrorOutputStream() << L"wperf version: " << MAJOR << "." << MINOR << "."
                   << PATCH << "\n";
        exit_code = EXIT_FAILURE;
        goto clean_exit;
    }

    if (request.do_test)
    {
        pmu_device.do_test(enable_bits, request.ioctl_events);
        goto clean_exit;
    }

    try
    {
        if (request.do_list)
        {
            pmu_device.do_list(request.metrics);
            goto clean_exit;
        }

        pmu_device.post_init(request.cores_idx, request.dmc_idx, request.do_timeline, enable_bits);

        if (request.do_count)
        {
            if (!request.has_events())
            {
                m_out.GetErrorOutputStream() << "no event specified\n";
                return -1;
            }
            else if (request.do_verbose)
            {
                request.show_events();
            }

            if (SetConsoleCtrlHandler(&ctrl_handler, TRUE) == FALSE)
                throw fatal_exception("SetConsoleCtrlHandler failed");

            uint32_t stop_bits = CTL_FLAG_CORE;
            if (pmu_device.has_dsu)
                stop_bits |= CTL_FLAG_DSU;
            if (pmu_device.has_dmc)
                stop_bits |= CTL_FLAG_DMC;

            pmu_device.stop(stop_bits);

            pmu_device.timeline_params(request.ioctl_events, request.count_interval, request.do_kernel);

            for (uint32_t core_idx : request.cores_idx)
                pmu_device.events_assign(core_idx, request.ioctl_events, request.do_kernel);

            pmu_device.timeline_header(request.ioctl_events);

            int64_t counting_duration_iter = request.count_duration > 0 ?
                static_cast<int64_t>(request.count_duration * 10) : _I64_MAX;

            int64_t counting_interval_iter = request.count_interval > 0 ?
                static_cast<int64_t>(request.count_interval * 2) : 0;

            do
            {
                pmu_device.reset(enable_bits);

                SYSTEMTIME timestamp_a;
                GetSystemTime(&timestamp_a);

                pmu_device.start(enable_bits);

                m_out.GetOutputStream() << L"counting ... -";

                int progress_map_index = 0;
                wchar_t progress_map[] = { L'/', L'|', L'\\', L'-' };
                int64_t t_count1 = counting_duration_iter;

                while (t_count1 > 0 && no_ctrl_c)
                {
                    m_out.GetOutputStream() << L'\b' << progress_map[progress_map_index % 4];
                    t_count1--;
                    Sleep(100);
                    progress_map_index++;
                }
                m_out.GetOutputStream() << L'\b' << "done\n";

                pmu_device.stop(enable_bits);

                SYSTEMTIME timestamp_b;
                GetSystemTime(&timestamp_b);

                if (enable_bits & CTL_FLAG_CORE)
                {
                    pmu_device.core_events_read();
                    pmu_device.print_core_stat(request.ioctl_events[EVT_CORE]);
                }

                if (enable_bits & CTL_FLAG_DSU)
                {
                    pmu_device.dsu_events_read();
                    pmu_device.print_dsu_stat(request.ioctl_events[EVT_DSU], request.report_l3_cache_metric);
                }

                if (enable_bits & CTL_FLAG_DMC)
                {
                    pmu_device.dmc_events_read();
                    pmu_device.print_dmc_stat(request.ioctl_events[EVT_DMC_CLK], request.ioctl_events[EVT_DMC_CLKDIV2], request.report_ddr_bw_metric);
                }

                ULARGE_INTEGER li_a, li_b;
                FILETIME time_a, time_b;

                SystemTimeToFileTime(&timestamp_a, &time_a);
                SystemTimeToFileTime(&timestamp_b, &time_b);
                li_a.u.LowPart = time_a.dwLowDateTime;
                li_a.u.HighPart = time_a.dwHighDateTime;
                li_b.u.LowPart = time_b.dwLowDateTime;
                li_b.u.HighPart = time_b.dwHighDateTime;

                if (!request.do_timeline)
                {
                    double duration = (double)(li_b.QuadPart - li_a.QuadPart) / 10000000.0;
                    m_out.GetOutputStream() << std::endl;
                    m_out.GetOutputStream() << std::right << std::setw(20)
                        << duration << L" seconds time elapsed" << std::endl;
                    m_globalJSON.m_duration = duration;
                }
                else
                {
                    m_out.GetOutputStream() << L"sleeping ... -";
                    int64_t t_count2 = counting_interval_iter;
                    for (; t_count2 > 0 && no_ctrl_c; t_count2--)
                    {
                        m_out.GetOutputStream() << L'\b' << progress_map[t_count2 % 4];
                        Sleep(500);
                    }

                    m_out.GetOutputStream() << L'\b' << "done\n";
                }

                if (m_outputType == TableOutputL::JSON || m_outputType == TableOutputL::ALL)
                {
                    m_out.Print(m_globalJSON);
                }
            } while (request.do_timeline && no_ctrl_c);
        }
        else if (request.do_sample)
        {
            if (SetConsoleCtrlHandler(&ctrl_handler, TRUE) == FALSE)
                throw fatal_exception("SetConsoleCtrlHandler failed for sampling");

            if (request.sample_pe_file == L"")
                throw fatal_exception("PE file not specified");

            if (request.sample_pdb_file == L"")
                throw fatal_exception("PDB file not specified");

            std::vector<SectionDesc> sec_info;          // List of sections in executable
            std::vector<FuncSymDesc> sym_info;
            std::vector<std::wstring> sec_import;       // List of DLL imported by executable
            uint64_t static_entry_point, image_base;

            parse_pe_file(request.sample_pe_file, static_entry_point, image_base, sec_info, sec_import);
            parse_pdb_file(request.sample_pdb_file, sym_info, request.sample_display_short);

            uint32_t stop_bits = CTL_FLAG_CORE;

            pmu_device.stop(stop_bits);

            pmu_device.set_sample_src(request.ioctl_events_sample, request.do_kernel);

            UINT64 runtime_vaddr_delta = 0;

            std::map<std::wstring, PeFileMetaData> dll_metadata;        // [pe_name] -> PeFileMetaData
            std::map<std::wstring, ModuleMetaData> modules_metadata;    // [mod_name] -> ModuleMetaData

            HMODULE hMods[1024];
            DWORD cbNeeded;
            DWORD pid = FindProcess(request.sample_image_name);
            HANDLE process_handle = OpenProcess(PROCESS_VM_READ | PROCESS_QUERY_INFORMATION, 0, pid);

            if (EnumProcessModules(process_handle, hMods, sizeof(hMods), &cbNeeded))
            {
                for (auto i = 0; i < (cbNeeded / sizeof(HMODULE)); i++)
                {
                    TCHAR szModName[MAX_PATH];
                    TCHAR lpszBaseName[MAX_PATH];

                    std::wstring name;
                    if (GetModuleBaseNameW(process_handle, hMods[i], lpszBaseName, MAX_PATH))
                    {
                        name = lpszBaseName;
                        modules_metadata[name].mod_name = name;
                    }

                    // Get the full path to the module's file.
                    if (GetModuleFileNameEx(process_handle, hMods[i], szModName, sizeof(szModName) / sizeof(TCHAR)))
                    {
                        std::wstring mod_path = szModName;
                        modules_metadata[name].mod_path = mod_path;
                        modules_metadata[name].handle = hMods[i];
                    }
                }
            }

            if (request.do_verbose)
            {
                m_out.GetOutputStream() << L"================================" << std::endl;
                for (const auto& [key, value] : modules_metadata)
                    {
                        m_out.GetOutputStream() << std::setw(32) << key
                            << std::setw(32) << IntToHexWideString((ULONGLONG)value.handle, 20)
                            << L"          " << value.mod_path << std::endl;
                    }
            }

            for (auto& [key, value] : modules_metadata)
            {
                std::wstring pdb_path = gen_pdb_name(value.mod_path);
                std::ifstream ifile(pdb_path);
                if (ifile) {
                    PeFileMetaData pefile_metadata;
                    parse_pe_file(value.mod_path, pefile_metadata);
                    dll_metadata[value.mod_name] = pefile_metadata;

                    parse_pdb_file(pdb_path, value.sym_info, request.sample_display_short);
                    ifile.close();
                }
            }

            if (request.do_verbose)
            {
                m_out.GetOutputStream() << L"================================" << std::endl;
                for (const auto& [key, value] : dll_metadata)
                {
                    m_out.GetOutputStream() << std::setw(32) << key
                        << L"          " << value.pe_name << std::endl;

                    for (auto& sec : value.sec_info)
                    {
                        m_out.GetOutputStream() << std::setw(32) << sec.name
                            << std::setw(32) << IntToHexWideString(sec.offset, 20)
                            << std::setw(32) << IntToHexWideString(sec.virtual_size)
                            << std::endl;
                    }
                }
            }

            HMODULE module_handle = GetModule(process_handle, request.sample_image_name);
            MODULEINFO modinfo;
            bool ret = GetModuleInformation(process_handle, module_handle, &modinfo, sizeof(MODULEINFO));
            if (!ret)
            {
                m_out.GetOutputStream() << L"failed to query base address of '" << request.sample_image_name << L"'\n";
            }
            else
            {
                runtime_vaddr_delta = (UINT64)modinfo.EntryPoint - (image_base + static_entry_point);
                m_out.GetOutputStream() << L"base address of '" << request.sample_image_name
                    << L"': 0x" << std::hex << (UINT64)modinfo.EntryPoint
                    << L", runtime delta: 0x" << runtime_vaddr_delta << std::endl;
            }

            std::vector<FrameChain> raw_samples;
            {
                DWORD image_exit_code = 0;

                pmu_device.start_sample();
                
                m_out.GetOutputStream() << L"sampling ...";
                do
                {
                    Sleep(1000);
                    bool sample = pmu_device.get_sample(raw_samples);
                    if (sample)
                        m_out.GetOutputStream() << L".";
                    else
                        m_out.GetOutputStream() << L"e";

                    if (GetExitCodeProcess(process_handle, &image_exit_code))
                        if (image_exit_code != STILL_ACTIVE)
                            break;
                } while (no_ctrl_c);
                m_out.GetOutputStream() << " done!" << std::endl;

                pmu_device.stop_sample();

                if (request.do_verbose)
                    m_out.GetOutputStream() << "Sampling stopped, process pid=" << pid
                        << L" exited with code " << IntToHexWideString(image_exit_code) << std::endl;
            }

            CloseHandle(process_handle);

            std::vector<SampleDesc> resolved_samples;

            // Search for symbols in image (executable)
            uint64_t sec_base = 0;
            for (auto& b : sym_info)
            {
                for (const auto &c : sec_info)
                {
                    if (c.idx == (b.sec_idx - 1))
                    {
                        sec_base = image_base + c.offset + runtime_vaddr_delta;
                        break;
                    }
                }
            }

            for (const auto &a : raw_samples)
            {
                bool found = false;
                SampleDesc sd;

                // Search in symbol table for image (executable)
                for (const auto& b : sym_info)
                {
                    if (a.pc >= (b.offset + sec_base) && a.pc < (b.offset + sec_base + b.size))
                    {
                        sd.name = b.name;
                        found = true;
                        break;
                    }
                }

                // Nothing was found in base images, let's search inside modules loaded with
                // images (such as DLLs).
                // Note: at this point:
                //  `dll_metadata` contains names of all modules loaded with image (executable)
                //  `modules_metadata` contains e.g. symbols of image modules loaded which had
                //                     PDB files present and we were able to load them.
                if (!found)
                {
                    sec_base = 0;

                    for (const auto& [key, value] : dll_metadata)
                    {
                        if (modules_metadata.count(key))
                        {
                            ModuleMetaData& mmd = modules_metadata[key];

                            for (auto& b : mmd.sym_info)
                                for (const auto& c : value.sec_info)
                                    if (c.idx == (b.sec_idx - 1))
                                    {
                                        sec_base = (UINT64)mmd.handle + c.offset;
                                        break;
                                    }

                            for (const auto& b : mmd.sym_info)
                                if (a.pc >= (b.offset + sec_base) && a.pc < (b.offset + sec_base + b.size))
                                {
                                    sd.name = b.name + L":" + key;
                                    found = true;
                                    break;
                                }
                        }
                    }
                }

                if (!found)
                    sd.name = L"unknown";

                for (uint32_t counter_idx = 0; counter_idx < 32; counter_idx++)
                {
                    if (!(a.ov_flags & (1i64 << (UINT64)counter_idx)))
                        continue;

                    bool inserted = false;
                    uint32_t event_src;
                    if (counter_idx == 31)
                        event_src = CYCLE_EVT_IDX;
                    else
                        event_src = request.ioctl_events_sample[counter_idx].index;
                    for (auto& c : resolved_samples)
                    {
                        if (c.name == sd.name && c.event_src == event_src)
                        {
                            c.freq++;
                            bool pc_found = false;
                            for (int i = 0; i < c.pc.size(); i++)
                            {
                                if (c.pc[i].first == a.pc)
                                {
                                    c.pc[i].second += 1;
                                    pc_found = true;
                                    break;
                                }
                            }

                            if (!pc_found)
                                c.pc.push_back(std::make_pair(a.pc, 1));

                            inserted = true;
                            break;
                        }
                    }

                    if (!inserted)
                    {
                        sd.freq = 1;
                        sd.event_src = event_src;
                        sd.pc.push_back(std::make_pair(a.pc, 1));
                        resolved_samples.push_back(sd);
                    }
                }
            }

            std::sort(resolved_samples.begin(), resolved_samples.end(), sort_samples);

            uint32_t prev_evt_src = 0;
            if(resolved_samples.size() > 0)
                prev_evt_src = resolved_samples[0].event_src;

            std::vector<uint64_t> total_samples;
            uint64_t acc = 0;
            for (const auto& a : resolved_samples)
            {
                if (a.event_src != prev_evt_src)
                {
                    prev_evt_src = a.event_src;
                    total_samples.push_back(acc);
                    acc = 0;
                }

                acc += a.freq;
            }
            total_samples.push_back(acc);

            int32_t group_idx = -1;
            prev_evt_src = CYCLE_EVT_IDX - 1;
            uint64_t printed_sample_num = 0, printed_sample_freq = 0;
            std::vector<std::wstring> col_overhead, col_count, col_symbol;
            for (auto a : resolved_samples)
            {
                if (a.event_src != prev_evt_src)
                {
                    if (prev_evt_src != CYCLE_EVT_IDX - 1)
                    {
                        TableOutputL table(m_outputType);
                        table.PresetHeaders<SamplingOutputTraitsL>();
                        table.SetAlignment(0, ColumnAlignL::RIGHT);
                        table.SetAlignment(1, ColumnAlignL::RIGHT);
                        table.Insert(col_overhead, col_count, col_symbol);
                        table.InsertExtra(L"interval", std::to_wstring(request.sampling_inverval[prev_evt_src]));
                        table.InsertExtra(L"printed_sample_num", std::to_wstring(printed_sample_num));
                        m_out.Print(table);
                        table.m_event = GlobalStringType(get_event_name(static_cast<uint16_t>(prev_evt_src)));
                        m_globalSamplingJSON.m_samplingTables.push_back(table);
                        col_overhead.clear();
                        col_count.clear();
                        col_symbol.clear();
                    }
                    prev_evt_src = a.event_src;

                    if (printed_sample_num > 0 && printed_sample_num < request.sample_display_row)
                        m_out.GetOutputStream() 
                            << DoubleToWideStringExt(((double)printed_sample_freq * 100 / (double)total_samples[group_idx]), 2, 6) << L"%"
                            << IntToDecWideString(printed_sample_freq, 10)
                            << L"  top " << std::dec << printed_sample_num << L" in total" << std::endl;

                    m_out.GetOutputStream()
                        << L"======================== sample source: "
                        << get_event_name(static_cast<uint16_t>(a.event_src)) << L", top "
                        << std::dec << request.sample_display_row
                        << L" hot functions ========================" << std::endl;

                    printed_sample_num = 0;
                    printed_sample_freq = 0;
                    group_idx++;
                }

                if (printed_sample_num == request.sample_display_row)
                {
                    m_out.GetOutputStream() << DoubleToWideStringExt(((double)printed_sample_freq * 100 / (double)total_samples[group_idx]), 2, 6) << L"%"
                        << IntToDecWideString(printed_sample_freq, 10)
                        << L"  top " << std::dec << request.sample_display_row << L" in total" << std::endl;
                    printed_sample_num++;
                    continue;
                }

                if (printed_sample_num > request.sample_display_row)
                    continue;

                col_overhead.push_back(DoubleToWideString(((double)a.freq * 100 / (double)total_samples[group_idx])) + L"%");
                col_count.push_back(std::to_wstring(a.freq));
                col_symbol.push_back(a.name);

                if (request.do_verbose)
                {
                    std::sort(a.pc.begin(), a.pc.end(), sort_pcs);

                    for (int i = 0; i < 10 && i < a.pc.size(); i++)
                    {
                        m_out.GetOutputStream() << L"                   " << IntToHexWideString(a.pc[i].first, 20) << L" " << IntToDecWideString(a.pc[i].second, 8) << std::endl;
                    }
                }

                printed_sample_freq += a.freq;
                printed_sample_num++;
            }

            TableOutputL table(m_outputType);
            table.PresetHeaders<SamplingOutputTraitsL>();
            table.SetAlignment(0, ColumnAlignL::RIGHT);
            table.SetAlignment(1, ColumnAlignL::RIGHT);
            table.Insert(col_overhead, col_count, col_symbol);
            table.InsertExtra(L"interval", std::to_wstring(request.sampling_inverval[prev_evt_src]));
            table.InsertExtra(L"printed_sample_num", std::to_wstring(printed_sample_num));
            m_out.Print(table);
            table.m_event = GlobalStringType(get_event_name(static_cast<uint16_t>(prev_evt_src)));
            m_globalSamplingJSON.m_samplingTables.push_back(table);
            m_globalSamplingJSON.sample_display_row = request.sample_display_row;

            if (m_outputType == TableOutputL::JSON || m_outputType == TableOutputL::ALL)
            {
                m_out.Print(m_globalSamplingJSON);
            }

            if (printed_sample_num > 0 && printed_sample_num < request.sample_display_row)
                m_out.GetOutputStream() << DoubleToWideStringExt((double)printed_sample_freq * 100 / (double)total_samples[group_idx], 2, 6) << L"%"
                           << IntToDecWideString(printed_sample_freq, 10) << L"  top " << std::dec << printed_sample_num << L" in total" << std::endl;
        }
    }
	catch (fatal_exception& e)
	{
		std::cerr << e.what() << std::endl;
        exit_code = EXIT_FAILURE;
        goto clean_exit;
	}

    //
    // Port End
    //
clean_exit:
    if (hDevice != INVALID_HANDLE_VALUE) {
        CloseHandle(hDevice);
    }

    return exit_code;
}

BOOL
GetDevicePath(
    _In_ LPGUID InterfaceGuid,
    _Out_writes_(BufLen) PWCHAR DevicePath,
    _In_ size_t BufLen
    )
{
    CONFIGRET cr = CR_SUCCESS;
    PWSTR deviceInterfaceList = NULL;
    ULONG deviceInterfaceListLength = 0;
    PWSTR nextInterface;
    HRESULT hr = E_FAIL;
    BOOL bRet = TRUE;

    cr = CM_Get_Device_Interface_List_Size(
                &deviceInterfaceListLength,
                InterfaceGuid,
                NULL,
                CM_GET_DEVICE_INTERFACE_LIST_PRESENT);
    if (cr != CR_SUCCESS) {
        WindowsPerfDbgPrint("Error: 0x%x retrieving device interface list size.\n", cr);
        goto clean0;
    }

    if (deviceInterfaceListLength <= 1) {
        bRet = FALSE;
        WindowsPerfDbgPrint("Error: No active device interfaces found.\n"
                            "       Is the sample driver loaded?");
        goto clean0;
    }

    deviceInterfaceList = (PWSTR)malloc(deviceInterfaceListLength * sizeof(WCHAR));
    if (deviceInterfaceList == NULL) {
        WindowsPerfDbgPrint("Error: Allocating memory for device interface list.\n");
        goto clean0;
    }
    ZeroMemory(deviceInterfaceList, deviceInterfaceListLength * sizeof(WCHAR));

    cr = CM_Get_Device_Interface_List(
                InterfaceGuid,
                NULL,
                deviceInterfaceList,
                deviceInterfaceListLength,
                CM_GET_DEVICE_INTERFACE_LIST_PRESENT);
    if (cr != CR_SUCCESS) {
        WindowsPerfDbgPrint("Error: 0x%x retrieving device interface list.\n", cr);
        goto clean0;
    }

    nextInterface = deviceInterfaceList + wcslen(deviceInterfaceList) + 1;
    if (*nextInterface != UNICODE_NULL) {
        WindowsPerfDbgPrint("Warning: More than one device interface instance found. \n"
                            "         Selecting first matching device.\n\n");
    }

    hr = StringCchCopy(DevicePath, BufLen, deviceInterfaceList);
    if (FAILED(hr)) {
        bRet = FALSE;
        WindowsPerfDbgPrint("Error: StringCchCopy failed with HRESULT 0x%x", hr);
        goto clean0;
    }

clean0:
    if (deviceInterfaceList != NULL) {
        free(deviceInterfaceList);
    }
    if (CR_SUCCESS != cr) {
        bRet = FALSE;
    }

    return bRet;
}
