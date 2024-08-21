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

#include <Windows.h>
#include <sysinfoapi.h>
#if defined(ENABLE_ETW_TRACING_APP)
#include "wperf-etw.h"
#endif

#include "wperf.h"
#include "wperf-common\public.h"
#include "wperf-common\macros.h"
#include "wperf-common\iorequest.h"
#include "utils.h"
#include "output.h"
#include "exception.h"
#include "pe_file.h"
#include "process_api.h"
#include "events.h"
#include "pmu_device.h"
#include "man.h"
#include "user_request.h"
#include "config.h"
#include "perfdata.h"
#include "disassembler.h"

static bool no_ctrl_c = true;

using MapKey = std::tuple<std::wstring, DWORD, TableOutput<DisassemblyOutputTraitsL, GlobalCharType>, std::wstring>;
auto MapComp = [](const MapKey& a, const MapKey& b)
{
    return std::tie(std::get<0>(a), std::get<1>(a)) < std::tie(std::get<0>(b), std::get<1>(b));
};

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

int __cdecl
wmain(
    _In_ const int argc,
    _In_reads_(argc) const wchar_t* argv[]
)
{
    auto exit_code = EXIT_SUCCESS;

    user_request request;
    pmu_device pmu_device;
    wstr_vec raw_args;

    LLVMDisassembler disassembler;
    bool spawned_process = false;
    HANDLE process_handle = NULL;
    PROCESS_INFORMATION pi;
    ZeroMemory(&pi, sizeof(pi));

    //* Handle CLI options before we initialize PMU device(s)
    for (int i = 1; i < argc; i++)
        raw_args.push_back(argv[i]);

    pmu_device.do_force_lock = user_request::is_force_lock(raw_args);

    if (raw_args.size() == 1 && user_request::is_help(raw_args))
    {
        user_request::print_help();
        goto clean_exit;
    }

    if (raw_args.empty())
    {
        user_request::print_help_header();
        user_request::print_help_prompt();
        goto clean_exit;
    }
    //* Handle CLI options before we initialize PMU device(s)

    try {
        pmu_device.init();
        pmu_device.core_init();
        pmu_device.dsu_init();
        pmu_device.dmc_init();
        pmu_device.spe_init();
    }
    catch (const locked_exception&)
    {
        m_out.GetErrorOutputStream() << L"warning: other WindowsPerf process acquired the wperf-driver." << std::endl;
        m_out.GetErrorOutputStream() << L"note: use --force-lock to force driver to give lock to current `wperf` process." << std::endl;
        m_out.GetErrorOutputStream() << L"Operation cancelled!" << std::endl;
        exit_code = EXIT_FAILURE;
        goto clean_exit;
    }
    catch (const lock_denied_exception&)
    {
        m_out.GetErrorOutputStream() << L"warning: other WindowsPerf process hijacked the wperf-driver, see --force-lock." << std::endl;
        m_out.GetErrorOutputStream() << L"Operation terminated, your data was lost!" << std::endl;
        exit_code = EXIT_FAILURE;
        goto clean_exit;
    }
    catch (const lock_insufficient_resources_exception&)
    {
        m_out.GetErrorOutputStream() << L"warning: not enough resources available to execute the request." << std::endl;
        m_out.GetErrorOutputStream() << L"Operation cancelled!" << std::endl;
        exit_code = EXIT_FAILURE;
        goto clean_exit;
    }
    catch (const lock_unknown_exception&)
    {
        m_out.GetErrorOutputStream() << L"warning: unknown lock error." << std::endl;
        m_out.GetErrorOutputStream() << L"Operation cancelled!" << std::endl;
        exit_code = EXIT_FAILURE;
        goto clean_exit;

    }
    catch (std::exception&) {
        exit_code = EXIT_FAILURE;
        goto clean_exit;
    }

#if defined(ENABLE_ETW_TRACING_APP)
    EventRegisterWindowsPerf_App();
#endif

    try
    {
        struct pmu_device_cfg pmu_cfg;
        pmu_device.get_pmu_device_cfg(pmu_cfg);
        request.init(raw_args, pmu_cfg,
            pmu_device.builtin_metrics,
            pmu_device.get_product_groups_metrics_names(),
            pmu_events::extra_events);
        pmu_device.do_verbose = request.do_verbose;
        pmu_device.timeline_output_file = request.timeline_output_file;
        pmu_device.m_sampling_with_spe = request.m_sampling_with_spe;
        pmu_device.m_sampling_flags = request.m_sampling_flags;
    }
    catch (std::exception&)
    {
        exit_code = EXIT_FAILURE;
        goto clean_exit;
    }

    if (request.do_detect)
    {
        pmu_device.do_detect();
        goto clean_exit;
    }

    if (request.do_help)
    {
        user_request::print_help();
        goto clean_exit;
    }
    if (request.do_man)
    {
        std::vector<std::wstring> col1, col2;

        wstr_vec man_query_args;
        TokenizeWideStringOfStrings(request.man_query_args, L',', man_query_args);

        //restricts the number of args into man to one
        if (!man_query_args.empty())
        {
            wstr_vec man_query_one_arg(1, man_query_args[0]);
            man_query_args = man_query_one_arg;
        }

        try 
        {
            man(pmu_device, man_query_args, L'/', pmu_device.get_product_name(), col1, col2);
        }
        catch(const fatal_exception&)
        {
            exit_code = EXIT_FAILURE;
            goto clean_exit;
        }

        TableOutput<ManOutputTraitsL, GlobalCharType> table(MAN);
        table.PresetHeaders();
        table.Insert(col1, col2);
        m_out.Print(table, true, TableType::MAN);

        goto clean_exit;

    }

    uint32_t enable_bits = 0;
    try
    {
        std::vector<enum evt_class> e_classes;
        for (const auto& [key, _] : request.ioctl_events)
            e_classes.push_back(key);

        enable_bits = pmu_device.enable_bits(e_classes);
    }
    catch (fatal_exception& e)
    {
        m_out.GetErrorOutputStream() << e.what() << std::endl;
        exit_code = EXIT_FAILURE;
        goto clean_exit;
    }

    if (request.do_version)
    {
        version_info driver_ver;
        pmu_device.do_version(driver_ver);

        if (driver_ver.major != MAJOR || driver_ver.minor != MINOR
            || driver_ver.patch != PATCH)
        {
            m_out.GetErrorOutputStream() << L"Version mismatch between wperf-driver and wperf.\n";
            m_out.GetErrorOutputStream() << L"wperf-driver version: " << driver_ver.major << "."
                << driver_ver.minor << "." << driver_ver.patch << "\n";
            m_out.GetErrorOutputStream() << L"wperf version: " << MAJOR << "." << MINOR << "."
                << PATCH << "\n";
            exit_code = EXIT_FAILURE;
        }

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
            HardwareInformation hardwareInformation{ 0 };
            GetHardwareInfo(hardwareInformation);

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

            uint32_t stop_bits = pmu_device.stop_bits();

            pmu_device.stop(stop_bits);

            pmu_device.timeline_params(request.ioctl_events, request.count_interval, request.do_kernel);

            for (uint32_t core_idx : request.cores_idx)
                pmu_device.events_assign(core_idx, request.ioctl_events, request.do_kernel);

            pmu_device.timeline_header(request.ioctl_events);

            int64_t counting_duration_iter = request.count_duration > 0 ?
                static_cast<int64_t>(request.count_duration * 10) : _I64_MAX;

            int64_t counting_interval_iter = request.count_interval > 0 ?
                static_cast<int64_t>(request.count_interval * 2) : 0;

            int counting_timeline_times = request.count_timeline;

            // === Spawn counting process ===
            bool do_count_process_spawn = request.sample_pe_file.size();
            DWORD pid;
            // === Spawn counting process ===
            if (do_count_process_spawn)
            {
                if (request.cores_idx.size() > 1)
                    throw fatal_exception("you can specify only one core for process spawn");

                SpawnProcess(request.sample_pe_file.c_str(), request.record_commandline.c_str(), &pi, request.record_spawn_delay);
                pid = GetProcessId(pi.hProcess);
                process_handle = pi.hProcess;

                if (!SetAffinity(hardwareInformation, pid, request.cores_idx[0]))
                {
                    TerminateProcess(pi.hProcess, 0);
                    CloseHandle(pi.hThread);
                    CloseHandle(process_handle);
                    throw fatal_exception("Error setting affinity");
                }

                if (request.do_verbose)
                    m_out.GetOutputStream() << request.sample_pe_file << " pid is " << pid << std::endl;

                process_handle = OpenProcess(PROCESS_VM_READ | PROCESS_QUERY_INFORMATION, 0, pid);
                spawned_process = true;
            }

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

                DWORD image_exit_code = 0;
                while (t_count1 > 0 && no_ctrl_c)
                {
                    m_out.GetOutputStream() << L'\b' << progress_map[progress_map_index % 4];
                    t_count1--;
                    Sleep(100);
                    progress_map_index++;

                    if (do_count_process_spawn && GetExitCodeProcess(process_handle, &image_exit_code))
                        if (image_exit_code != STILL_ACTIVE)
                            break;
                }
                m_out.GetOutputStream() << L'\b' << "done\n";

                pmu_device.stop(enable_bits);

                SYSTEMTIME timestamp_b;
                GetSystemTime(&timestamp_b);

                if (enable_bits & CTL_FLAG_CORE)
                {
                    pmu_device.core_events_read();
                    pmu_device.print_core_stat(request.ioctl_events[EVT_CORE]);
                    pmu_device.print_core_metrics(request.ioctl_events[EVT_CORE]);
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

                const double  duration = timestamps_to_duration(timestamp_a, timestamp_b);
                m_globalJSON.m_duration = duration;

                if (!request.do_timeline)
                {
                    m_out.GetOutputStream() << std::endl;
                    m_out.GetOutputStream() << std::right << std::setw(20)
                        << duration << L" seconds time elapsed" << std::endl;
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

                if (!request.do_timeline)
                    if (m_outputType == TableType::JSON || m_outputType == TableType::ALL)
                        m_out.Print(m_globalJSON);

                m_globalTimelineJSON.m_timelineWperfStat.push_back(m_globalJSON);
                m_globalTimelineJSON.m_count_duration = request.count_duration;
                m_globalTimelineJSON.m_count_interval = request.count_interval;
                m_globalTimelineJSON.m_count_timeline = request.count_timeline;

                if (request.do_timeline)
                    m_globalJSON = WPerfStatJSON<GlobalCharType>();

                if (counting_timeline_times > 0)
                {
                    --counting_timeline_times;
                    if (counting_timeline_times <= 0)
                        break;
                }

                if (do_count_process_spawn && image_exit_code != STILL_ACTIVE)
                    break;

            } while (request.do_timeline && no_ctrl_c);

            if (do_count_process_spawn)
            {
                TerminateProcess(pi.hProcess, 0);
                CloseHandle(pi.hThread);
                CloseHandle(process_handle);
                spawned_process = false;
            }

            if (request.do_timeline)
                m_out.Print(m_globalTimelineJSON);
        }
        else if (request.do_sample || request.do_record)
        {
            enable_bits = 0;
            if(request.m_sampling_with_spe)
            {
                /* If we are sampling with SPE we will also enable special PMU events to be counted during
                the sampling session. We added those events manually.*/
                try
                {
                    std::vector<enum evt_class> e_classes;
                    for (const auto& [key, _] : request.ioctl_events)
                        e_classes.push_back(key);

                    enable_bits = pmu_device.enable_bits(e_classes);
                }
                catch (fatal_exception& e)
                {
                    m_out.GetErrorOutputStream() << e.what() << std::endl;
                    exit_code = EXIT_FAILURE;
                    goto clean_exit;
                }

                uint32_t stop_bits = pmu_device.stop_bits();

                pmu_device.stop(stop_bits);

                for (uint32_t core_idx : request.cores_idx)
                    pmu_device.events_assign(core_idx, request.ioctl_events, request.do_kernel);
            }

            if (request.do_verbose)
            {
                m_out.GetOutputStream() << "sampling type: ";
                if (request.m_sampling_with_spe == true)
                    m_out.GetOutputStream() << "hardware sampling (" << pmu_device.get_spe_version_name() << L")";
                else
                    m_out.GetOutputStream() << "software sampling";
                m_out.GetOutputStream() << std::endl << std::endl;
            }

            if (request.do_disassembly)
            {
                if (!disassembler.CheckCommand())
                {
                    m_out.GetErrorOutputStream() << L"Error executing disassembler `" << disassembler.GetCommand() << L"`. Is it on PATH?" << std::endl;
                    m_out.GetErrorOutputStream() << L"note: wperf uses LLVM's objdump. You can install Visual Studio 'C++ Clang Compiler...' and 'MSBuild support for LLVM'" << std::endl;
                    throw fatal_exception("Failed to call disassembler!");
                }
            }
            HardwareInformation hardwareInformation{ 0 };
            GetHardwareInfo(hardwareInformation);
            
            PerfDataWriter perfDataWriter;
            if (request.do_export_perf_data)
                perfDataWriter.WriteCommandLine(argc, argv);

            if (SetConsoleCtrlHandler(&ctrl_handler, TRUE) == FALSE)
                throw fatal_exception("SetConsoleCtrlHandler failed for sampling");

            if (request.sample_pe_file == L"")
                throw fatal_exception("PE file not specified");

            if (request.sample_pdb_file == L"")
                throw fatal_exception("PDB file not specified");

            if (request.cores_idx.size() > 1)
                throw fatal_exception("you can specify only one core for sampling");

            m_globalSamplingJSON.m_pe_file = request.sample_pe_file;
            m_globalSamplingJSON.m_pdb_file = request.sample_pdb_file;

            std::vector<SectionDesc> sec_info;          // List of sections in executable
            std::vector<FuncSymDesc> sym_info;
            std::vector<std::wstring> sec_import;       // List of DLL imported by executable
            uint64_t static_entry_point, image_base;

            parse_pe_file(request.sample_pe_file, static_entry_point, image_base, sec_info, sec_import);
            parse_pdb_file(request.sample_pdb_file, sym_info, request.sample_display_short);

            uint32_t stop_bits = CTL_FLAG_CORE;

            if (!request.m_sampling_with_spe)
            {
                pmu_device.stop(stop_bits);
                pmu_device.set_sample_src(request.ioctl_events_sample, request.do_kernel);
            }

            if (request.do_export_perf_data)
            {
                for (auto& events_sample : request.ioctl_events_sample)
                {
                    perfDataWriter.RegisterSampleEvent(events_sample.index);
                }
            }

            UINT64 runtime_vaddr_delta = 0;

            std::map<std::wstring, PeFileMetaData> dll_metadata;        // [pe_name] -> PeFileMetaData
            std::map<std::wstring, ModuleMetaData> modules_metadata;    // [mod_name] -> ModuleMetaData

            const size_t hMods_size = sizeof(HMODULE) * MAX_MODULES;
            auto hMods = std::make_unique<HMODULE[]>(MAX_MODULES);     // HMODULE hMods[1024];
            DWORD cbNeeded;
            DWORD pid;
            TCHAR imageFileName[MAX_PATH];

            //If the user asked to record we should spawn the process ourselves.
            if (request.do_record)
            {
                SpawnProcess(request.sample_pe_file.c_str(), request.record_commandline.c_str(), &pi, request.record_spawn_delay);
                pid = GetProcessId(pi.hProcess);
                process_handle = pi.hProcess;

                if (!SetAffinity(hardwareInformation, pid, request.cores_idx[0]))
                {
                    TerminateProcess(pi.hProcess, 0);
                    CloseHandle(pi.hThread);
                    CloseHandle(process_handle);
                    throw fatal_exception("Error setting affinity");
                }

                if (request.do_verbose)
                {
                    m_out.GetOutputStream() << request.sample_pe_file << " pid is " << pid << std::endl;
                }

                DWORD len = GetModuleFileNameEx(pi.hProcess, NULL, imageFileName, MAX_PATH);
                if (len == 0)
                {
                    m_out.GetErrorOutputStream() << "Error getting module name " << GetLastError() << "." << std::endl;
                    throw fatal_exception("Unable to read module name.");
                }
                spawned_process = true;
            }
            else {
                pid = FindProcess(request.sample_image_name);
            }
            process_handle = OpenProcess(PROCESS_VM_READ | PROCESS_QUERY_INFORMATION, 0, pid);

            if (request.do_export_perf_data)
            {
                perfDataWriter.RegisterEvent(PerfDataWriter::COMM, pid, request.sample_image_name);
            }

            if (EnumProcessModules(process_handle, hMods.get(), hMods_size, &cbNeeded))
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
                        uint64_t mod_base = 0;
                        std::wstring mod_path = szModName;
                        modules_metadata[name].mod_path = mod_path;
                        modules_metadata[name].handle = hMods[i];
                        
                        parse_pe_file(mod_path, mod_base);
                        modules_metadata[name].mod_baseOfDll = mod_base;

                        MODULEINFO modinfo;
                        if (GetModuleInformation(process_handle, hMods[i], &modinfo, sizeof(MODULEINFO)))
                        {                            
                            if (request.do_export_perf_data)
                            {
                                perfDataWriter.RegisterEvent(PerfDataWriter::MMAP, pid, reinterpret_cast<UINT64>(modinfo.lpBaseOfDll), modinfo.SizeOfImage, mod_path, 0);
                            }
                        }
                        else {
                            m_out.GetErrorOutputStream() << "Failed to get module " << szModName << " information" << std::endl;
                        }

                    }
                }
            }

            if (request.do_verbose)
            {
                m_out.GetOutputStream() << L"================================" << std::endl;
                std::vector<GlobalStringType> col_name, col_path;
                std::vector<ULONGLONG> col_address;
                
                m_globalSamplingJSON.m_verbose = true;
                m_globalSamplingJSON.m_modules_table.PresetHeaders();
                
                for (const auto& [key, value] : modules_metadata)
                {
                    m_out.GetOutputStream() << std::setw(32) << key
                        << std::setw(32) << IntToHexWideString((ULONGLONG)value.handle, 20)
                        << L"          " << value.mod_path << std::endl;
                    col_name.push_back(key);
                    col_address.push_back(reinterpret_cast<ULONGLONG>(value.handle));
                    col_path.push_back(value.mod_path);

                }
                m_globalSamplingJSON.m_modules_table.Insert(col_name, col_address, col_path);
            }

            for (auto& [key, value] : modules_metadata)
            {
                std::wstring pdb_path = gen_pdb_name(value.mod_path);
                std::ifstream ifile(pdb_path);
                if (ifile) {
                    PeFileMetaData pefile_metadata;
                    parse_pe_file(value.mod_path, pefile_metadata);
                    pefile_metadata.pdb_file = pdb_path;
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
                    TableOutput<SamplingModuleInfoOutputTraits<GlobalCharType>, GlobalCharType> module_info_table(m_outputType);                    
                    module_info_table.PresetHeaders();
                    module_info_table.InsertExtra(L"module", key);
                    module_info_table.InsertExtra(L"pe_name", value.pe_name);
                    module_info_table.InsertExtra(L"pdb_file", value.pdb_file);
                    std::vector<GlobalStringType> col_name;
                    std::vector<uint64_t> col_offset, col_virtual_size;
                    for (auto& sec : value.sec_info)
                    {
                        m_out.GetOutputStream() << std::setw(32) << sec.name
                            << std::setw(32) << IntToHexWideString(sec.offset, 20)
                            << std::setw(32) << IntToHexWideString(sec.virtual_size)
                            << std::endl;
                        col_name.push_back(sec.name);
                        col_offset.push_back(sec.offset);
                        col_virtual_size.push_back(sec.virtual_size);
                    }
                    module_info_table.Insert(col_name, col_offset, col_virtual_size);
                    m_globalSamplingJSON.m_modules_info_vector.push_back(module_info_table);
                }
            }

            HMODULE module_handle;
            if (request.do_record)
            {
                module_handle = GetModule(process_handle, imageFileName);
            }
            else {
                module_handle = GetModule(process_handle, request.sample_image_name);
            }
            
            MODULEINFO modinfo;
            bool ret = GetModuleInformation(process_handle, module_handle, &modinfo, sizeof(MODULEINFO));
            if (!ret)
            {
                m_out.GetOutputStream() << L"failed to query base address of '" << request.sample_image_name << L"' with " << std::hex << GetLastError() << "\n";
            }
            else
            {
                runtime_vaddr_delta = (UINT64)modinfo.EntryPoint - (image_base + static_entry_point);
                m_out.GetOutputStream() << L"base address of '" << request.sample_image_name
                    << L"': 0x" << std::hex << (UINT64)modinfo.EntryPoint
                    << L", runtime delta: 0x" << runtime_vaddr_delta << std::endl;

                m_globalSamplingJSON.m_base_address = reinterpret_cast<UINT64>(modinfo.EntryPoint);
                m_globalSamplingJSON.m_runtime_delta = runtime_vaddr_delta;

                if (request.do_export_perf_data)
                    perfDataWriter.RegisterEvent(PerfDataWriter::COMM, pid, request.sample_image_name);
            }

            SYSTEMTIME timestamp_a;
            SYSTEMTIME timestamp_b;
            
            std::vector<FrameChain> raw_samples;
            {
                DWORD image_exit_code = 0;

                int64_t sampling_duration_iter = request.count_duration > 0 ?
                    static_cast<int64_t>(request.count_duration * 10) : _I64_MAX;
                int64_t t_count1 = sampling_duration_iter;

                if (request.m_sampling_with_spe)
                {
                    pmu_device.reset(enable_bits);
                    pmu_device.start(enable_bits);
                    pmu_device.spe_start(request.m_sampling_flags);
                }
                else pmu_device.start_sample();

                m_out.GetOutputStream() << L"sampling ...";

                
                GetSystemTime(&timestamp_a);

                do
                {
                    t_count1--;
                    Sleep(100);

                    if ((t_count1 % 10) == 0)
                    {                        
                        if(request.m_sampling_with_spe)
                        {
                            if (pmu_device.spe_get())
                                m_out.GetOutputStream() << L".";
                            else
                                m_out.GetOutputStream() << L"e";
                        } else {
                            if (pmu_device.get_sample(raw_samples))
                                m_out.GetOutputStream() << L".";
                            else
                                m_out.GetOutputStream() << L"e";
                        }
                    }

                    if (GetExitCodeProcess(process_handle, &image_exit_code))
                        if (image_exit_code != STILL_ACTIVE)
                            break;
                }
                while (t_count1 > 0 && no_ctrl_c);

                GetSystemTime(&timestamp_b);

                m_out.GetOutputStream() << " done!" << std::endl;

                if(request.m_sampling_with_spe)
                {
                    // We stop the SPE first so we don't miss any PMU events
                    pmu_device.spe_stop();
                    pmu_device.spe_get();
                    pmu_device.stop(enable_bits);

                    // Now we read jus the core events and print the debugging information
                    pmu_device.core_events_read();
                    pmu_device.print_core_stat(request.ioctl_events[EVT_CORE]);
                    pmu_device.print_core_metrics(request.ioctl_events[EVT_CORE]);
                } else {
                    pmu_device.stop_sample();
                }

                if (request.do_verbose)
                    m_out.GetOutputStream() << "Sampling stopped, process pid=" << pid
                    << L" exited with code " << IntToHexWideString(image_exit_code) << std::endl;
            }

            if (request.do_record)
            {
                TerminateProcess(pi.hProcess, 0);
                CloseHandle(pi.hThread);
                spawned_process = false;
            }
            CloseHandle(process_handle);

            std::map<UINT64, std::wstring> spe_event_map;

            if(request.m_sampling_with_spe && pmu_device.m_has_spe)
            {
                std::ofstream spe_buffer_file("spe.data", std::ios::out | std::ios::binary | std::ios::trunc);
                if(spe_buffer_file.is_open())
                {
                    spe_buffer_file.write(reinterpret_cast<char*>(pmu_device.m_spe_buffer.data()), pmu_device.m_spe_buffer.size() * sizeof(UINT8));
                    if (spe_buffer_file.fail())
                    {
                        m_out.GetErrorOutputStream() << "Error trying to save SPE memory buffer to spe.data file!" << std::endl;
                    }
                    spe_buffer_file.close();
                }
                else {
                    m_out.GetErrorOutputStream() << "Error trying to open spe.data file!" << std::endl;
                }

                spe_device::get_samples(pmu_device.m_spe_buffer, raw_samples, spe_event_map);
            }

            std::vector<SampleDesc> resolved_samples;

            for (const auto& a : raw_samples)
            {
                bool found = false;
                SampleDesc sd;
                uint64_t sec_base = 0;

                // Search in symbol table for image (executable)
                for (const auto& b : sym_info)
                {
                    for (const auto& c : sec_info)
                    {
                        assert(b.sec_idx);
                        if (c.idx == (b.sec_idx - 1))
                        {
                            sec_base = image_base + c.offset + runtime_vaddr_delta;
                            break;
                        }
                    }

                    if (a.pc >= (b.offset + sec_base) && a.pc < (b.offset + sec_base + b.size))
                    {
                        sd.desc = b;
                        sd.module = 0;
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
                                    {
                                        if (!b.sec_idx) // We may not be able to decode all symbols at this time, so we skip
                                            continue;

                                        if (c.idx == (b.sec_idx - 1))
                                        {
                                            sec_base = (UINT64)mmd.handle + c.offset;
                                            break;
                                        }
                                    }

                            for (const auto& b : mmd.sym_info)
                                if (a.pc >= (b.offset + sec_base) && a.pc < (b.offset + sec_base + b.size))
                                {
                                    sd.desc = b;
                                    sd.desc.name = b.name + L":" + key;
                                    sd.module = &mmd;
                                    found = true;
                                    break;
                                }
                        }
                    }
                }

                if (!found)
                    sd.desc.name = L"unknown";

                /* `counter_idx_unmap` carries all the information we need to translate GPCs to event numbers.
                *    We just loop through it, which represents available GPCs.
                */
                bool spe_gone = false;
                for (auto const& [mapped_counter_idx, counter_idx] : pmu_device.counter_idx_unmap)
                {
                    // Check if this sample represents an overflow of this particular GPC
                    if (!request.m_sampling_with_spe)
                    {
                        if (!(a.ov_flags & (1i64 << (UINT64)mapped_counter_idx)))
                            continue;
                    }
                    else {
                        if (spe_gone)
                            continue;
                        spe_gone = true;
                    }

                    bool inserted = false;
                    uint32_t event_src;
                    if(!request.m_sampling_with_spe)
                    {
                        if (counter_idx == 31)
                            event_src = CYCLE_EVT_IDX;
                        else
                            event_src = request.ioctl_events_sample[counter_idx].index;
                    } else {
                        event_src = a.spe_event_idx;
                    }

                    for (auto& c : resolved_samples)
                    {
                        if (c.desc.name == sd.desc.name && c.event_src == event_src)
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
                        sd.pc.clear();
                        sd.pc.push_back(std::make_pair(a.pc, 1));
                        resolved_samples.push_back(sd);
                    }
                }
            }

            std::sort(resolved_samples.begin(), resolved_samples.end(), sort_samples);

            uint32_t prev_evt_src = 0;
            if (resolved_samples.size() > 0)
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
            std::vector<std::wstring> col_symbol;
            std::vector<double> col_overhead;
            std::vector<uint32_t> col_count;

            std::vector<std::pair<GlobalStringType, 
                std::variant<TableOutput<SamplingAnnotateOutputTraitsL<false>, GlobalCharType>,
                             TableOutput<SamplingAnnotateOutputTraitsL<true>, GlobalCharType>>>> annotateTables;
            std::vector<uint64_t> col_pcs, col_pcs_count;
            for (auto &a : resolved_samples)
            {
                if (a.event_src != prev_evt_src)
                {
                    if (prev_evt_src != CYCLE_EVT_IDX - 1)
                    {
                        TableOutput<SamplingOutputTraitsL, GlobalCharType> table(m_outputType);
                        table.PresetHeaders();
                        table.SetAlignment(0, ColumnAlignL::RIGHT);
                        table.SetAlignment(1, ColumnAlignL::RIGHT);
                        table.Insert(col_overhead, col_count, col_symbol);
                        table.InsertExtra(L"interval", request.sampling_inverval[prev_evt_src]);
                        table.InsertExtra(L"printed_sample_num", printed_sample_num);
                        m_out.Print(table);
                        if (!request.m_sampling_with_spe)
                            table.m_event = GlobalStringType(pmu_events::get_event_name(static_cast<uint16_t>(prev_evt_src)));
                        else
                            table.m_event = GlobalStringType(spe_event_map[prev_evt_src]);
                        TableOutput<SamplingPCOutputTraits<GlobalCharType>, GlobalCharType> pcs_table(m_outputType);
                        pcs_table.PresetHeaders();
                        pcs_table.Insert(col_pcs, col_pcs_count);
                        m_globalSamplingJSON.m_map[table.m_event] = std::make_tuple(table, annotateTables, pcs_table);
                        col_overhead.clear();
                        col_count.clear();
                        col_symbol.clear();
                        annotateTables.clear();
                        col_pcs.clear();
                        col_pcs_count.clear();
                    }
                    prev_evt_src = a.event_src;

                    if (printed_sample_num > 0 && printed_sample_num < request.sample_display_row)
                    {
                        const int total_width = PrettyTable<wchar_t>::m_LEFT_MARGIN + PrettyTable<wchar_t>::m_COLUMN_SEPARATOR + static_cast<int>(strlen("overhead"));
                        m_out.GetOutputStream()
                            << DoubleToWideStringExt((double)printed_sample_freq * 100 / (double)total_samples[group_idx], 2, total_width) << L"%"
                            << IntToDecWideString(printed_sample_freq, 6)
                            << std::wstring(PrettyTable<wchar_t>::m_COLUMN_SEPARATOR, L' ') << L"top " << std::dec << printed_sample_num << L" in total" << std::endl;
                        m_out.GetOutputStream() << std::endl;
                    }

                    m_out.GetOutputStream()
                        << L"======================== sample source: ";
                    if(!request.m_sampling_with_spe)
                        m_out.GetOutputStream() << pmu_events::get_event_name(static_cast<uint16_t>(a.event_src));
                    else
                        m_out.GetOutputStream() << spe_event_map[a.event_src];
                    m_out.GetOutputStream() << L", top " << std::dec << request.sample_display_row
                        << L" hot functions ========================" << std::endl;

                    printed_sample_num = 0;
                    printed_sample_freq = 0;
                    group_idx++;
                }

                if (printed_sample_num == request.sample_display_row)
                {
                    const int total_width = PrettyTable<wchar_t>::m_LEFT_MARGIN + PrettyTable<wchar_t>::m_COLUMN_SEPARATOR + static_cast<int>(strlen("overhead"));
                    m_out.GetOutputStream()
                        << DoubleToWideStringExt((double)printed_sample_freq * 100 / (double)total_samples[group_idx], 2, total_width) << L"%"
                        << IntToDecWideString(printed_sample_freq, 6)
                        << std::wstring(PrettyTable<wchar_t>::m_COLUMN_SEPARATOR, L' ') << L"top " << std::dec << request.sample_display_row << L" in total" << std::endl;
                    printed_sample_num++;
                    continue;
                }

                if (printed_sample_num > request.sample_display_row)
                    continue;

                col_overhead.push_back(((double)a.freq * 100 / (double)total_samples[group_idx]));// +L"%");
                col_count.push_back(a.freq);
                col_symbol.push_back(a.desc.name);

                if (request.do_verbose)
                {
                    std::sort(a.pc.begin(), a.pc.end(), sort_pcs);

                    for (int i = 0; i < 10 && i < a.pc.size(); i++)
                    {
                        m_out.GetOutputStream() << L"                   " << IntToHexWideString(a.pc[i].first, 20) << L" " << IntToDecWideString(a.pc[i].second, 8) << std::endl;
                        col_pcs.push_back(a.pc[i].first);
                        col_pcs_count.push_back(a.pc[i].second);
                    }
                }

                if (request.do_export_perf_data)
                {
                    for (const auto& sample : a.pc)
                    {
                        ULONGLONG addr;
                        if (a.module == NULL)
                            addr = (sample.first - runtime_vaddr_delta) & 0xFFFFFF;
                        else
                        {
                            UINT64 mod_vaddr_delta = (UINT64)a.module->handle;
                            addr = (sample.first - mod_vaddr_delta) & 0xFFFFFF;
                        }
                        perfDataWriter.RegisterEvent(PerfDataWriter::SAMPLE, pid, sample.first, request.cores_idx[0], a.event_src);
                    }
                }

                if (request.do_annotate)
                {

                    std::map <MapKey, uint64_t, decltype(MapComp)> hotspots(MapComp);
                    std::map <std::tuple<uint64_t, uint64_t>, std::tuple<std::vector<std::wstring>, std::vector<std::wstring>>> disassemble_map;
                    std::vector<std::wstring> col_source_file, col_inst_addr;
                    std::vector< TableOutput<DisassemblyOutputTraitsL, GlobalCharType>> col_dasm;
                    std::vector<uint64_t> col_line_number, col_hits;
                    if(a.desc.name != L"unknown")
                    {
                        m_out.GetOutputStream() << a.desc.name << std::endl;
                        for (const auto& sample : a.pc)
                        {
                            bool found_line = false;
                            ULONGLONG addr;
                            if(a.module == NULL)
                                addr = (sample.first - runtime_vaddr_delta) & 0xFFFFFF;
                            else
                            {
                                UINT64 mod_vaddr_delta = (UINT64)a.module->handle;
                                addr = (sample.first - mod_vaddr_delta) & 0xFFFFFF;
                            }
                            for (const auto& line : a.desc.lines)
                            {
                                if (line.virtualAddress <= addr && line.virtualAddress + line.length > addr)
                                {
                                    std::wstring dasm_str, hex_ip;
                                    std::vector<std::wstring> col_dasm_instr;
                                    std::vector<std::wstring> col_dasm_addr;
                                    
                                    TableOutput<DisassemblyOutputTraitsL, GlobalCharType> dasmTable;
                                    dasmTable.PresetHeaders();

                                    if(request.do_disassembly)
                                    {
                                        std::wstringstream addr_stream;
                                        std::vector<DisassembledInstruction> lineAsm{ 0 };
                                        uint64_t base = a.module == NULL ? image_base : a.module->mod_baseOfDll;
                                        std::wstring& target = a.module == NULL ? request.sample_pe_file : a.module->mod_path;
                                        addr_stream << std::hex << addr;

                                        if (disassemble_map.find(std::make_tuple(line.virtualAddress, base)) == disassemble_map.end())
                                        {
                                            disassembler.Disassemble(line.virtualAddress + base, line.virtualAddress + line.length + base, target);
                                            disassembler.ParseOutput(lineAsm);

                                            for (const auto& inst : lineAsm)
                                            {
                                                std::wstringstream to_hex;
                                                to_hex << std::hex << inst.m_address;

                                                col_dasm_instr.push_back(inst.m_asm);
                                                col_dasm_addr.push_back(to_hex.str());
                                            }

                                            disassemble_map[std::make_tuple(line.virtualAddress, base)] = std::make_tuple(col_dasm_addr, col_dasm_instr);//dasm_str;
                                        } else {
                                            auto& elem = disassemble_map[std::make_tuple(line.virtualAddress, base)];
                                            col_dasm_addr = std::get<0>(elem);
                                            col_dasm_instr = std::get<1>(elem);
                                        }

                                        hex_ip = addr_stream.str();
                                    }
                                    dasmTable.Insert(col_dasm_addr, col_dasm_instr);

                                    std::wstringstream str_addr;
                                    str_addr << std::hex << addr;

                                    MapKey cur = std::make_tuple(line.source_file, line.lineNum, dasmTable, hex_ip);

                                    if (auto el = hotspots.find(cur); el == hotspots.end())
                                    {
                                        hotspots[cur] = sample.second;
                                    }
                                    else {
                                        hotspots[cur] += sample.second;
                                    }
                                    found_line = true;
                                }
                            }
                            if (!found_line)
                            {
                                m_out.GetErrorOutputStream() << "No line for " << std::hex << addr << " found." << std::endl;
                            }
                        }

                        using ExpandedMapKey = decltype(std::tuple_cat((*(hotspots.begin())).first, std::make_tuple((*(hotspots.begin())).second)));
                        std::vector<ExpandedMapKey>  sorting_annotate;
                        for (const auto& [key, val] : hotspots)
                        {
                            sorting_annotate.push_back(std::tuple_cat(key, std::make_tuple(val)));
                        }

                        std::sort(sorting_annotate.begin(), sorting_annotate.end(), [](const ExpandedMapKey a, const ExpandedMapKey b)->bool { 
                                                                                        constexpr auto length = std::tuple_size_v<decltype(a)>;
                                                                                        return std::get<length-1>(a) > std::get<length-1>(b); });
                        
                        for (const auto& el : sorting_annotate)
                        {
                            col_source_file.push_back(std::get<0>(el));
                            col_line_number.push_back(std::get<1>(el));
                            col_dasm.push_back(std::get<2>(el));
                            col_inst_addr.push_back(std::get<3>(el));
                            col_hits.push_back(std::get<4>(el));
                        }

                        if (col_source_file.size() > 0)
                        {
                            if(request.do_disassembly)
                            {
                                TableOutput<SamplingAnnotateOutputTraitsL<true>, GlobalCharType> annotateTable;
                                annotateTable.PresetHeaders();
                                annotateTable.Insert(col_line_number, col_hits, col_source_file, col_inst_addr, col_dasm);
                                m_out.Print(annotateTable);
                                annotateTables.push_back(std::make_pair(a.desc.name, annotateTable));
                            }
                            else {
                                TableOutput<SamplingAnnotateOutputTraitsL<false>, GlobalCharType> annotateTable;
                                annotateTable.PresetHeaders();
                                annotateTable.Insert(col_line_number, col_hits, col_source_file);
                                m_out.Print(annotateTable);
                                annotateTables.push_back(std::make_pair(a.desc.name, annotateTable));
                            }
                        }
                    }

                    if(!m_out.m_isQuiet && (m_outputType == TableType::PRETTY || m_outputType == TableType::ALL))
                    {
                        m_out.GetOutputStream() << std::endl;
                    }
                }

                printed_sample_freq += a.freq;
                printed_sample_num++;
            }
            
            if (request.do_export_perf_data)
                perfDataWriter.Write();

            TableOutput<SamplingOutputTraitsL, GlobalCharType> table(m_outputType);
            table.PresetHeaders();
            table.SetAlignment(0, ColumnAlignL::RIGHT);
            table.SetAlignment(1, ColumnAlignL::RIGHT);
            table.Insert(col_overhead, col_count, col_symbol);
            table.InsertExtra(L"interval", request.sampling_inverval[prev_evt_src]);
            table.InsertExtra(L"printed_sample_num", printed_sample_num);
            m_out.Print(table);
            if (!request.m_sampling_with_spe)
                table.m_event = GlobalStringType(pmu_events::get_event_name(static_cast<uint16_t>(prev_evt_src)));
            else
                table.m_event = GlobalStringType(spe_event_map[prev_evt_src]);

            TableOutput<SamplingPCOutputTraits<GlobalCharType>, GlobalCharType> pcs_table(m_outputType);
            pcs_table.PresetHeaders();
            pcs_table.Insert(col_pcs, col_pcs_count);
            m_globalSamplingJSON.m_map[table.m_event] = std::make_tuple(table, annotateTables,pcs_table);
            m_globalSamplingJSON.m_sample_display_row = request.sample_display_row;

            if (m_outputType == TableType::JSON || m_outputType == TableType::ALL)
            {
                if (request.m_sampling_with_spe)
                    m_out.Print(m_globalSamplingJSON, m_globalJSON);
                else
                    m_out.Print(m_globalSamplingJSON);
            }

            if (printed_sample_num > 0 && printed_sample_num < request.sample_display_row)
            {
                const int total_width = PrettyTable<wchar_t>::m_LEFT_MARGIN + PrettyTable<wchar_t>::m_COLUMN_SEPARATOR + static_cast<int>(strlen("overhead"));
                m_out.GetOutputStream()
                    << DoubleToWideStringExt((double)printed_sample_freq * 100 / (double)total_samples[group_idx], 2, total_width) << L"%"
                    << IntToDecWideString(printed_sample_freq, 6)
                    << std::wstring(PrettyTable<wchar_t>::m_COLUMN_SEPARATOR, L' ') <<  L"top " << std::dec << printed_sample_num << L" in total" << std::endl;
            }

            const double  duration = timestamps_to_duration(timestamp_a, timestamp_b);
            m_globalJSON.m_duration = duration;

            if (!request.do_timeline)
            {
                m_out.GetOutputStream() << std::endl;
                m_out.GetOutputStream() << std::right << std::setw(20)
                    << duration << L" seconds time elapsed" << std::endl;
            }
        }
    }
    catch (const lock_denied_exception& e)
    {
        m_out.GetErrorOutputStream() << L"warning: other WindowsPerf process hijacked (forced lock) the wperf-driver, see --force-lock." << std::endl;
        m_out.GetErrorOutputStream() << L"Operation terminated, your data was lost!" << std::endl;
        if (request.do_verbose)
            m_out.GetErrorOutputStream() << e.what() << std::endl;
        exit_code = EXIT_FAILURE;
        goto clean_exit;
    }
    catch (fatal_exception& e)
    {
        m_out.GetErrorOutputStream() << e.what() << std::endl;
        exit_code = EXIT_FAILURE;
        goto clean_exit;
    }
    catch (std::exception& e) {
        m_out.GetErrorOutputStream() << L"warning: unknown error, see: " << e.what() << std::endl;
        exit_code = EXIT_FAILURE;
        goto clean_exit;
    }

clean_exit:
#if defined(ENABLE_ETW_TRACING_APP)
    EventUnregisterWindowsPerf_App();
#endif

    if(spawned_process)
    {
        TerminateProcess(pi.hProcess, 0);
        CloseHandle(pi.hThread);
        CloseHandle(process_handle);
    }
    disassembler.Close();
    return exit_code;
}
