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

#include <wchar.h>
#include "wperf-lib.h"
#include "config.h"
#include "exception.h"
#include "padding.h"
#include "parsers.h"
#include "pe_file.h"
#include "pmu_device.h"
#include "process_api.h"
#include "timeline.h"
#include "wperf-common/gitver.h"
#include "wperf-common/public.h"
#include "perfdata.h"
#include <regex>

typedef struct _COUNTING_INFO
{
    uint64_t counter_value;
    uint16_t event_idx;
    uint64_t multiplexed_scheduled;
    uint64_t multiplexed_round;
    uint64_t scaled_value;
    EVENT_NOTE evt_note;
} COUNTING_INFO;

typedef struct _SAMPLING_INFO
{
    std::wstring symbol;
    uint32_t count;
    double overhead;
} SAMPLING_INFO, *PSAMPLING_INFO;

typedef struct _SAMPLE_ANNOTATE_INFO
{
    std::wstring symbol;
    std::wstring source_file;
    uint32_t line_number;
    uint64_t hits;
} SAMPLE_ANNOTATE_INFO, *PSAMPLE_ANNOTATE_INFO;

static pmu_device* __pmu_device = nullptr;
static struct pmu_device_cfg* __pmu_cfg = nullptr;
static std::map<enum evt_class, std::vector<uint16_t>> __list_events;
static std::vector<std::wstring> __list_metrics;
static std::map<std::wstring, std::vector<uint16_t>> __list_metrics_events;
static std::map<enum evt_class, std::vector<struct evt_noted>> __ioctl_events;
static std::map<uint8_t, std::vector<COUNTING_INFO>> __countings;
static std::vector<TEST_INFO> __tests;
static std::vector<uint16_t> __sample_events;
static std::map<uint16_t, std::vector<SAMPLING_INFO>> __samples;
static std::map<uint16_t, std::vector<SAMPLE_ANNOTATE_INFO>> __annotate_samples;
static size_t __annotate_sample_event_index = 0;
static size_t __annotate_sample_index = 0;

extern "C" bool wperf_init()
{
    try
    {
        __pmu_device = new pmu_device();
        __pmu_device->init();
        __pmu_device->core_init();
        __pmu_device->dsu_init();
        __pmu_device->dmc_init();
        __pmu_cfg = new pmu_device_cfg();
        __pmu_device->get_pmu_device_cfg(*__pmu_cfg);
        __pmu_device->do_verbose = false;
    }
    catch (...)
    {
        return false;
    }

    return true;
}

extern "C" bool wperf_close()
{
    try
    {
        if (__pmu_device)
            delete __pmu_device;

        if (__pmu_cfg)
            delete __pmu_cfg;

        __list_events.clear();
        __list_metrics.clear();
        __list_metrics_events.clear();
        __ioctl_events.clear();
        __countings.clear();
        __tests.clear();
        __sample_events.clear();
        __samples.clear();
        __annotate_samples.clear();
    }
    catch(...)
    {
        return false;
    }

    return true;
}

extern "C" bool wperf_set_verbose(bool do_verbose)
{
    try
    {
        __pmu_device->do_verbose = do_verbose;
    }
    catch(...)
    {
        return false;
    }

    return true;
}

extern "C" bool wperf_driver_version(PVERSION_INFO driver_ver)
{
    if (!driver_ver || !__pmu_device)
    {
        // driver_ver and __pmu_device should not be NULL.
        return false;
    }

    try
    {
        struct version_info version;

        __pmu_device->do_version_query(version);
        driver_ver->major = version.major;
        driver_ver->minor = version.minor;
        driver_ver->patch = version.patch;
        wcsncpy_s(driver_ver->gitver, version.gitver, MAX_GITVER_SIZE);
    }
    catch (...)
    {
        return false;
    }

    return true;
}

extern "C" bool wperf_version(PVERSION_INFO wperf_ver)
{
    if (!wperf_ver)
    {
        // wperf_ver should not be NULL.
        return false;
    }

    wperf_ver->major = MAJOR;
    wperf_ver->minor = MINOR;
    wperf_ver->patch = PATCH;
    wcsncpy_s(wperf_ver->gitver, WPERF_GIT_VER_STR, MAX_GITVER_SIZE);

    return true;
}

extern "C" bool wperf_list_events(PLIST_CONF list_conf, PEVENT_INFO einfo)
{
    if (!list_conf || !__pmu_device)
    {
        // list_conf and __pmu_device should not be NULL.
        return false;
    }

    static size_t list_index = 0;

    try
    {
        if (!einfo)
        {
            // Call events_query to initialize __list_events
            // with the list of all supported events.
            __list_events.clear();
            __pmu_device->events_query(__list_events);
            list_index = 0;
        }
        else if (list_conf->list_event_types&CORE_EVT)
        {
            // Yield the next event.
            if (list_index < __list_events[EVT_CORE].size())
            {
                einfo->type = CORE_TYPE;
                einfo->id = __list_events[EVT_CORE][list_index];
                einfo->name = pmu_events::get_core_event_name(einfo->id);
                einfo->desc = __pmu_device->pmu_events_get_evt_desc(einfo->id, EVT_CORE);
                list_index++;
            }
            else
            {
                // No more to yield.
                return false;
            }
        }
        else
        {
            // Only Core PMU events are supported for now.
            return false;
        }
    }
    catch (...)
    {
        return false;
    }

    return true;
}

extern "C" bool wperf_list_num_events(PLIST_CONF list_conf, int *num_events)
{
    if (!list_conf || !num_events)
    {
        // list_conf and num_events should not be NULL.
        return false;
    }

    if (list_conf->list_event_types & CORE_EVT)
    {
        // Only core events are supported now.
        *num_events = (int)__list_events[EVT_CORE].size();
    }

    return true;
}

extern "C" bool wperf_list_metrics(PLIST_CONF list_conf, PMETRIC_INFO minfo)
{
    if (!list_conf || !__pmu_device)
    {
        // list_conf and __pmu_device should not be NULL.
        return false;
    }

    static size_t metrics_index = 0;
    static size_t event_index = 0;

    try
    {
        if (!minfo)
        {
            // Initialize __list_metrics with the list of all builtin metrics.
            // Initialize __list_metrics_events with the list of events for each metric.
            __list_metrics.clear();
            __list_metrics_events.clear();
            metrics_index = 0;
            event_index = 0;
            std::map<std::wstring, metric_desc>& builtin_metrics = __pmu_device->builtin_metrics;
            for (auto [metric, desc] : builtin_metrics)
            {
                for (const auto& event : desc.groups[EVT_CORE])
                {
                    if (event.type != EVT_HDR)
                    {
                        __list_metrics_events[metric].push_back(event.index);
                    }
                }

                if (__list_metrics_events.count(metric))
                    __list_metrics.push_back(metric);
            }
        }
        else if (list_conf->list_event_types & CORE_EVT)
        {
            if (metrics_index >= __list_metrics.size())
            {
                // No more metrics.
                return false;
            }

            auto& metric = __list_metrics[metrics_index];
            if (event_index >= __list_metrics_events[metric].size())
            {
                // No more events for the current metric, move on to the next one.
                metrics_index++;
                event_index = 0;
                if (metrics_index >= __list_metrics.size())
                {
                    // No more metrics.
                    return false;
                }
                metric = __list_metrics[metrics_index];
            }

            // Yield the next event for the current metric.
            minfo->metric_name = metric.c_str();
            minfo->event_idx = __list_metrics_events[metric][event_index];
            event_index++;
        }
        else
        {
            // Only Core PMU events are supported for now.
            return false;
        }
    }
    catch (...)
    {
        return false;
    }

    return true;
}

extern "C" bool wperf_list_num_metrics(PLIST_CONF list_conf, int *num_metrics)
{
    if (!list_conf || !num_metrics)
    {
        // list_conf and num_metrics should not be NULL.
        return false;
    }

    if (list_conf->list_event_types & CORE_EVT)
    {
        // Only core events are supported now.
        *num_metrics = (int)__list_metrics.size();
    }

    return true;
}

extern "C" bool wperf_list_num_metrics_events(PLIST_CONF list_conf, int *num_metrics_events)
{
    if (!list_conf || !num_metrics_events)
    {
        // list_conf and num_metrics_events should not be NULL.
        return false;
    }

    if (list_conf->list_event_types & CORE_EVT)
    {
        // Only core events are supported now.
        int num_events = 0;
        for (auto& [_, events] : __list_metrics_events)
        {
            num_events += (int)events.size();
        }
        *num_metrics_events = num_events;
    }

    return true;
}

extern "C" bool wperf_stat(PSTAT_CONF stat_conf, PSTAT_INFO stat_info)
{
    if (!stat_conf || !__pmu_device || !__pmu_cfg)
    {
        // stat_conf, __pmu_device and __pmu_cfg should not be NULL.
        return false;
    }

    HANDLE process_handle = NULL;
    PROCESS_INFORMATION pi;
    ZeroMemory(&pi, sizeof(pi));
    bool spawned_process = false;

    static size_t event_index = 0;
    static size_t core_index = 0;

    try
    {
        if (!stat_info)
        {
            __ioctl_events.clear();
            __countings.clear();
            event_index = 0;
            core_index = 0;

            std::vector<uint8_t> cores_idx;
            for (int i = 0; i < stat_conf->num_cores; i++)
            {
                cores_idx.push_back(stat_conf->cores[i]);
            }
            // Only CORE events are supported at the moment.
            std::vector<enum evt_class> e_classes = { EVT_CORE };
            uint32_t enable_bits = __pmu_device->enable_bits(e_classes);
            __pmu_device->post_init(cores_idx, 0, stat_conf->timeline, enable_bits);

            HardwareInformation hardwareInformation{ 0 };
            GetHardwareInfo(hardwareInformation);

            uint32_t stop_bits = __pmu_device->stop_bits();
            __pmu_device->stop(stop_bits);

            // Process events from stat_conf.
            std::map<enum evt_class, std::deque<struct evt_noted>> events;
            for (int i = 0; i < stat_conf->num_events; i++)
            {
                events[EVT_CORE].push_back({ stat_conf->events[i], EVT_NORMAL, L"e" });
            }

            // Process groups from stat_conf.
            std::map<enum evt_class, std::vector<struct evt_noted>> groups;
            for (int i = 0; i < stat_conf->group_events.num_groups; i++)
            {
                groups[EVT_CORE].push_back({ (uint16_t)stat_conf->group_events.num_group_events[i], EVT_HDR, L"" });
                for (int j = 0; j < stat_conf->group_events.num_group_events[i]; j++)
                {
                    groups[EVT_CORE].push_back({ stat_conf->group_events.events[i][j], EVT_GROUPED, L"" });
                }
            }

            // Process metrics from stat_conf.
            std::map<std::wstring, metric_desc>& metrics = __pmu_device->builtin_metrics;
            for (int i = 0; i < stat_conf->num_metrics; i++)
            {
                const wchar_t* metric = stat_conf->metric_events[i];
                if (metrics.find(metric) == metrics.end())
                {
                    // Not a valid builtin metric.
                    return false;
                }

                metric_desc desc = metrics[metric];
                for (const auto& x : desc.events)
                    events[x.first].insert(events[x.first].end(), x.second.begin(), x.second.end());
                for (const auto& y : desc.groups)
                    groups[y.first].insert(groups[y.first].end(), y.second.begin(), y.second.end());
            }

            set_event_padding(__ioctl_events, *__pmu_cfg, events, groups);

            bool do_kernel = stat_conf->kernel_mode;
            __pmu_device->timeline_params(__ioctl_events, stat_conf->counting_interval, do_kernel);
            for (uint32_t core_idx : cores_idx)
                __pmu_device->events_assign(core_idx, __ioctl_events, do_kernel);
            __pmu_device->timeline_header(__ioctl_events);

            double count_duration = stat_conf->duration;
            int64_t counting_duration_iter = count_duration > 0 ?
                static_cast<int64_t>(count_duration * 10) : _I64_MAX;

            double count_interval = stat_conf->counting_interval;
            int64_t counting_interval_iter = count_interval > 0 ?
                static_cast<int64_t>(count_interval * 2) : 0;

            drvconfig::set(L"count.period", std::to_wstring(stat_conf->period));

            int counting_timeline_times = stat_conf->count_timeline;

            // === Spawn counting process ===
            bool do_count_process_spawn = stat_conf->pe_file != NULL;
            DWORD pid;
            // === Spawn counting process ===
            if (do_count_process_spawn)
            {
                if (stat_conf->num_cores > 1)
                    throw fatal_exception("you can specify only one core for process spawn");

                SpawnProcess(stat_conf->pe_file, stat_conf->record_commandline, &pi, stat_conf->record_spawn_delay);
                pid = GetProcessId(pi.hProcess);
                process_handle = pi.hProcess;

                if (!SetAffinity(hardwareInformation, pid, stat_conf->cores[0]))
                {
                    TerminateProcess(pi.hProcess, 0);
                    CloseHandle(pi.hThread);
                    CloseHandle(process_handle);
                    throw fatal_exception("Error setting affinity");
                }

                process_handle = OpenProcess(PROCESS_VM_READ | PROCESS_QUERY_INFORMATION, 0, pid);
                spawned_process = true;
            }

            do
            {
                __pmu_device->reset(enable_bits);

                __pmu_device->start(enable_bits);

                int64_t t_count = counting_duration_iter;

                DWORD image_exit_code = 0;
                while (t_count > 0)
                {
                    t_count--;
                    Sleep(100);

                    if (do_count_process_spawn && GetExitCodeProcess(process_handle, &image_exit_code))
                        if (image_exit_code != STILL_ACTIVE)
                            break;
                }

                __pmu_device->stop(enable_bits);

                if (enable_bits & CTL_FLAG_CORE)
                {
                    __pmu_device->core_events_read();
                    const ReadOut* core_outs = __pmu_device->get_core_outs();

                    std::vector<uint8_t> counting_cores = __pmu_device->get_cores_idx();
                    for (auto i : counting_cores)
                    {
                        UINT32 evt_num = core_outs[i].evt_num;
                        const struct pmu_event_usr* evts = core_outs[i].evts;
                        uint64_t round = core_outs[i].round;
                        for (UINT32 j = 0; j < evt_num; j++)
                        {
                            // Ignore padding events.
                            if (j >= 1 && (__ioctl_events[EVT_CORE][j - 1].type == EVT_PADDING))
                                continue;

                            const struct pmu_event_usr* evt = &evts[j];
                            COUNTING_INFO counting_info;
                            counting_info.counter_value = evt->value;
                            counting_info.event_idx = evt->event_idx;
                            if (evt->event_idx == CYCLE_EVT_IDX)
                            {
                                counting_info.evt_note.type = NORMAL_EVT_NOTE;
                            }
                            else
                            {
                                std::wstring note = __ioctl_events[EVT_CORE][j - 1].note;
                                std::wsmatch m;
                                if (note == L"e")
                                {
                                    // This is a NORMAL_EVT_NOTE.
                                    counting_info.evt_note.type = NORMAL_EVT_NOTE;
                                }
                                else if (std::regex_match(note, m, std::wregex(L"g([0-9]+),([a-z0-9_]+)")))
                                {
                                    // This is a METRIC_EVT_NOTE.
                                    auto it = metrics.find(m[2]);
                                    if (it == metrics.end())
                                    {
                                        // Not a valide builtin metric.
                                        return false;
                                    }
                                    counting_info.evt_note.type = METRIC_EVT_NOTE;
                                    counting_info.evt_note.note.metric_note.group_id = std::stoi(m[1]);
                                    counting_info.evt_note.note.metric_note.name = it->first.c_str();
                                }
                                else if (std::regex_match(note, m, std::wregex(L"g([0-9]+)")))
                                {
                                    // This is a GROUP_EVT_NOTE.
                                    counting_info.evt_note.type = GROUP_EVT_NOTE;
                                    counting_info.evt_note.note.group_note.group_id = std::stoi(m[1]);
                                }
                                else
                                {
                                    // Not a valid event note type.
                                    return false;
                                }
                            }

                            counting_info.multiplexed_scheduled = evt->scheduled;
                            counting_info.multiplexed_round = round;
                            counting_info.scaled_value = (uint64_t)((double)evt->value / ((double)evt->scheduled / (double)round));
                            __countings[i].push_back(counting_info);
                        }
                    }
                    if (stat_conf->timeline)
                    {
                        __pmu_device->print_core_stat(__ioctl_events[EVT_CORE]);
                        __pmu_device->print_core_metrics(__ioctl_events[EVT_CORE]);
                    }
                }

                if (stat_conf->timeline)
                {
                    int64_t t_count2 = counting_interval_iter;
                    for (; t_count2 > 0; t_count2--)
                    {
                        Sleep(500);
                    }
                }

                if (counting_timeline_times > 0)
                {
                    --counting_timeline_times;
                    if (counting_timeline_times <= 0)
                        break;
                }

                if (do_count_process_spawn && image_exit_code != STILL_ACTIVE)
                    break;

            } while (stat_conf->timeline);

            if (do_count_process_spawn)
            {
                TerminateProcess(pi.hProcess, 0);
                CloseHandle(pi.hThread);
                CloseHandle(process_handle);
                spawned_process = false;
            }
        }
        else
        {
            std::vector<uint8_t> counting_cores = __pmu_device->get_cores_idx();
            if (core_index >= counting_cores.size())
            {
                // No more core to yield.
                return false;
            }

            uint8_t core_idx = counting_cores[core_index];
            if (event_index >= __countings[core_idx].size())
            {
                // Start all over for the next core.
                event_index = 0;
                core_index++;
                if (core_index >= counting_cores.size())
                {
                    // No more core to yield.
                    return false;
                }
                core_idx = counting_cores[core_index];
            }

            // Yield next stat_info.
            stat_info->core_idx = core_idx;
            stat_info->counter_value = __countings[core_idx][event_index].counter_value;
            stat_info->event_idx = __countings[core_idx][event_index].event_idx;
            stat_info->multiplexed_scheduled = __countings[core_idx][event_index].multiplexed_scheduled;
            stat_info->multiplexed_round = __countings[core_idx][event_index].multiplexed_round;
            stat_info->scaled_value = __countings[core_idx][event_index].scaled_value;
            stat_info->evt_note = __countings[core_idx][event_index].evt_note;
            event_index++;
        }
    }
    catch (...)
    {
        if (spawned_process)
        {
            TerminateProcess(pi.hProcess, 0);
            CloseHandle(pi.hThread);
            CloseHandle(process_handle);
        }
        return false;
    }

    return true;
}

extern "C" bool wperf_sample(PSAMPLE_CONF sample_conf, PSAMPLE_INFO sample_info)
{
    if (!sample_conf || !__pmu_device)
    {
        // sample_conf and __pmu_device should not be NULL.
        return false;
    }

    HANDLE process_handle;
    PROCESS_INFORMATION pi;
    ZeroMemory(&pi, sizeof(pi));
    bool spawned_process = false;

    static size_t event_index = 0;
    static size_t sample_index = 0;

    try
    {
        if (!sample_info)
        {
            __sample_events.clear();
            __samples.clear();
            __annotate_samples.clear();
            event_index = 0;
            sample_index = 0;
            __annotate_sample_event_index = 0;
            __annotate_sample_index = 0;

            HardwareInformation hardwareInformation{ 0 };
            GetHardwareInfo(hardwareInformation);

            PerfDataWriter perfDataWriter;

            if (wcslen(sample_conf->pe_file) == 0)
            {
                // PE file not specified.
                return false;
            }

            if (wcslen(sample_conf->pdb_file) == 0)
            {
                // PDB file not specified.
                return false;
            }

            if (sample_conf->core_idx >= __pmu_device->core_num)
            {
                // Invalid core index.
                return false;
            }

            if (sample_conf->num_events < 1 || !sample_conf->events || !sample_conf->intervals)
            {
                // No event specified.
                return false;
            }

            std::vector<uint8_t> cores_idx = { sample_conf->core_idx };
            std::vector<enum evt_class> e_classes = { EVT_CORE };
            uint32_t enable_bits = __pmu_device->enable_bits(e_classes);
            __pmu_device->post_init(cores_idx, 0, false, enable_bits);

            std::vector<SectionDesc> sec_info;          // List of sections in executable
            std::vector<FuncSymDesc> sym_info;
            std::vector<std::wstring> sec_import;       // List of DLL imported by executable
            uint64_t static_entry_point, image_base;

            parse_pe_file(sample_conf->pe_file, static_entry_point, image_base, sec_info, sec_import);
            parse_pdb_file(sample_conf->pdb_file, sym_info, sample_conf->display_short);

            uint32_t stop_bits = CTL_FLAG_CORE;

            __pmu_device->stop(stop_bits);

            std::vector<struct evt_sample_src> ioctl_events_sample;
            for (int i = 0; i < sample_conf->num_events; i++)
            {
                ioctl_events_sample.push_back({ sample_conf->events[i], sample_conf->intervals[i] });
            }
            __pmu_device->set_sample_src(ioctl_events_sample, sample_conf->kernel_mode);

            if (sample_conf->export_perf_data)
            {
                for (auto& events_sample : ioctl_events_sample)
                {
                    perfDataWriter.RegisterSampleEvent(events_sample.index);
                }
            }

            UINT64 runtime_vaddr_delta = 0;

            std::map<std::wstring, PeFileMetaData> dll_metadata;        // [pe_name] -> PeFileMetaData
            std::map<std::wstring, ModuleMetaData> modules_metadata;    // [mod_name] -> ModuleMetaData

            HMODULE hMods[1024];
            DWORD cbNeeded;
            DWORD pid;
            TCHAR imageFileName[MAX_PATH];

            if (sample_conf->record)
            {
                SpawnProcess(sample_conf->pe_file, sample_conf->record_commandline, &pi, sample_conf->record_spawn_delay);
                pid = GetProcessId(pi.hProcess);
                process_handle = pi.hProcess;
                if (!SetAffinity(hardwareInformation, pid, sample_conf->core_idx))
                {
                    TerminateProcess(pi.hProcess, 0);
                    CloseHandle(pi.hThread);
                    CloseHandle(process_handle);
                    throw fatal_exception("Error setting affinity");
                }

                DWORD len = GetModuleFileNameEx(pi.hProcess, NULL, imageFileName, MAX_PATH);
                if (len == 0)
                {
                    // failed to read module name
                    return false;
                }
                spawned_process = true;
            }
            else
            {
                pid = FindProcess(sample_conf->image_name);
            }
            process_handle = OpenProcess(PROCESS_VM_READ | PROCESS_QUERY_INFORMATION, 0, pid);

            if (sample_conf->export_perf_data)
            {
                perfDataWriter.RegisterEvent(PerfDataWriter::COMM, pid, std::wstring(sample_conf->image_name));
            }

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

                        MODULEINFO modinfo;
                        if (GetModuleInformation(process_handle, hMods[i], &modinfo, sizeof(MODULEINFO)))
                        {
                            if (sample_conf->export_perf_data)
                            {
                                perfDataWriter.RegisterEvent(PerfDataWriter::MMAP, pid, reinterpret_cast<UINT64>(modinfo.lpBaseOfDll), modinfo.SizeOfImage, mod_path, 0);
                            }
                        }
                    }
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

                    parse_pdb_file(pdb_path, value.sym_info, sample_conf->display_short);
                    ifile.close();
                }
            }

            HMODULE module_handle;
            if (sample_conf->record)
            {
                module_handle = GetModule(process_handle, imageFileName);
            }
            else
            {
                module_handle = GetModule(process_handle, sample_conf->image_name);
            }

            MODULEINFO modinfo;
            bool ret = GetModuleInformation(process_handle, module_handle, &modinfo, sizeof(MODULEINFO));
            if (ret)
            {
                runtime_vaddr_delta = (UINT64)modinfo.EntryPoint - (image_base + static_entry_point);

                if (sample_conf->export_perf_data)
                    perfDataWriter.RegisterEvent(PerfDataWriter::COMM, pid, std::wstring(sample_conf->image_name));
            }

            std::vector<FrameChain> raw_samples;
            DWORD image_exit_code = 0;

            __pmu_device->start_sample();

            int64_t sampling_duration_iter = sample_conf->duration > 0 ?
                static_cast<int64_t>(sample_conf->duration * 10) : _I64_MAX;
            int64_t t_count1 = sampling_duration_iter;

            do
            {
                t_count1--;
                Sleep(100);
                bool sample = __pmu_device->get_sample(raw_samples);

                if (GetExitCodeProcess(process_handle, &image_exit_code))
                    if (image_exit_code != STILL_ACTIVE)
                        break;
            }
            while (t_count1 > 0);

            __pmu_device->stop_sample();

            if (sample_conf->record)
            {
                TerminateProcess(pi.hProcess, 0);
                CloseHandle(pi.hThread);
                spawned_process = false;
            }
            CloseHandle(process_handle);

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
                                    if (c.idx == (b.sec_idx - 1))
                                    {
                                        sec_base = (UINT64)mmd.handle + c.offset;
                                        break;
                                    }

                            for (const auto& b : mmd.sym_info)
                                if (a.pc >= (b.offset + sec_base) && a.pc < (b.offset + sec_base + b.size))
                                {
                                    sd.desc = b;
                                    sd.desc.name = b.name + L":" + key;
                                    sd.desc.sname = b.name;
                                    sd.module = &mmd;
                                    found = true;
                                    break;
                                }
                        }
                    }
                }

                if (!found)
                    sd.desc.name = L"unknown";

                for (auto const& [mapped_counter_idx, counter_idx] : __pmu_device->counter_idx_unmap)
                {
                    if (!(a.ov_flags & (1i64 << (UINT64)mapped_counter_idx)))
                        continue;

                    bool inserted = false;
                    uint32_t event_src;
                    if (counter_idx == 31)
                        event_src = CYCLE_EVT_IDX;
                    else
                        event_src = ioctl_events_sample[counter_idx].index;
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

            for (auto &a : resolved_samples)
            {
                if (a.event_src != prev_evt_src)
                {
                    __sample_events.push_back(a.event_src);
                    prev_evt_src = a.event_src;
                    group_idx++;
                }

                __samples[a.event_src].push_back({ a.desc.name, a.freq, (double)a.freq * 100 / (double)total_samples[group_idx] });

                if (sample_conf->export_perf_data)
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
                        perfDataWriter.RegisterEvent(PerfDataWriter::SAMPLE, pid, sample.first, sample_conf->core_idx, a.event_src);
                    }
                }

                if (sample_conf->annotate)
                {
                    std::map<std::pair<std::wstring, DWORD>, uint64_t> hotspots;
                    if (a.desc.name != L"unknown")
                    {
                        for (const auto& sample : a.pc)
                        {
                            bool found_line = false;
                            ULONGLONG addr;
                            if (a.module == NULL)
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
                                    std::pair<std::wstring, DWORD> cur = std::make_pair(line.source_file, line.lineNum);
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
                        }

                        std::vector<std::tuple<std::wstring, DWORD, uint64_t>>  sorting_annotate;
                        for (auto& [key, val] : hotspots)
                        {
                            sorting_annotate.push_back(std::make_tuple(key.first, key.second, val));
                        }

                        std::sort(sorting_annotate.begin(), sorting_annotate.end(), [](std::tuple<std::wstring, DWORD, uint64_t>& a, std::tuple<std::wstring, DWORD, uint64_t>& b)->bool { return std::get<2>(a) > std::get<2>(b); });

                        for (auto& el : sorting_annotate)
                        {
                            __annotate_samples[a.event_src].push_back({ a.desc.name, std::get<0>(el), std::get<1>(el), std::get<2>(el) });
                        }
                    }
                }
            }

            if (sample_conf->export_perf_data)
                perfDataWriter.Write();
        }
        else
        {
            if (event_index >= __sample_events.size())
            {
                // No more events to yield.
                return false;
            }

            uint16_t event_id = __sample_events[event_index];
            if (sample_index >= __samples[event_id].size())
            {
                // Start over for the next event.
                sample_index = 0;
                event_index++;
                if (event_index >= __sample_events.size())
                {
                    // No more events to yield.
                    return false;
                }
                event_id = __sample_events[event_index];
            }

            sample_info->event = event_id;
            sample_info->symbol = __samples[event_id][sample_index].symbol.c_str();
            sample_info->count = __samples[event_id][sample_index].count;
            sample_info->overhead = __samples[event_id][sample_index].overhead;
            sample_index++;
        }
    }
    catch (...)
    {
        if (spawned_process)
        {
            TerminateProcess(pi.hProcess, 0);
            CloseHandle(pi.hThread);
        }
        return false;
    }

    return true;
}

bool wperf_sample_annotate(PSAMPLE_CONF sample_conf, PANNOTATE_INFO annotate_info)
{
    if (!sample_conf || !annotate_info)
    {
        // sample_conf and annotate_info should not be NULL.
        return false;
    }

    if (__annotate_sample_event_index >= __sample_events.size())
    {
        // No more events to yield.
        return false;
    }

    uint16_t event_id = __sample_events[__annotate_sample_event_index];
    if (__annotate_sample_index >= __annotate_samples[event_id].size())
    {
        // Start over for the next event.
        __annotate_sample_index = 0;
        __annotate_sample_event_index++;
        if (__annotate_sample_event_index >= __sample_events.size())
        {
            // No more events to yield.
            return false;
        }
        event_id = __sample_events[__annotate_sample_event_index];
    }

    annotate_info->event = event_id;
    annotate_info->symbol = __annotate_samples[event_id][__annotate_sample_index].symbol.c_str();
    annotate_info->source_file = __annotate_samples[event_id][__annotate_sample_index].source_file.c_str();
    annotate_info->line_number = __annotate_samples[event_id][__annotate_sample_index].line_number;
    annotate_info->hits = __annotate_samples[event_id][__annotate_sample_index].hits;
    __annotate_sample_index++;
    return true;
}

extern "C" bool wperf_sample_stats(PSAMPLE_CONF sample_conf, PSAMPLE_STATS sample_stats)
{
    if (!sample_conf || !sample_stats || !__pmu_device)
    {
        // sample_conf, sample_stats and __pmu_device should not be NULL.
        return false;
    }

    sample_stats->sample_generated = __pmu_device->sample_summary.sample_generated;
    sample_stats->sample_dropped = __pmu_device->sample_summary.sample_dropped;
    return true;
}

extern "C" bool wperf_num_cores(int *num_cores)
{
    if (!num_cores || !__pmu_device)
    {
        // num_cores and __pmu_device should not be NULL.
        return false;
    }

    *num_cores = __pmu_device->core_num;
    return true;
}

static inline void wperf_test_insert_num(const wchar_t *name, uint64_t result)
{
    TEST_INFO tinfo;
    tinfo.name = name;
    tinfo.type = NUM_RESULT;
    tinfo.num_result = result;
    __tests.push_back(tinfo);
}

static inline void wperf_test_insert_bool(const wchar_t *name, bool result)
{
    TEST_INFO tinfo;
    tinfo.name = name;
    tinfo.type = BOOL_RESULT;
    tinfo.bool_result = result;
    __tests.push_back(tinfo);
}

static inline void wperf_test_insert_evt_note(const wchar_t *name, struct evt_noted result)
{
    TEST_INFO tinfo;
    tinfo.name = name;
    tinfo.type = EVT_NOTE_RESULT;
    tinfo.evt_note_result.index = result.index;
    tinfo.evt_note_result.type = result.type;
    __tests.push_back(tinfo);
}

static inline void wperf_test_insert_wstring(const wchar_t *name, const wchar_t* result)
{
    TEST_INFO tinfo;
    tinfo.name = name;
    tinfo.type = WSTRING_RESULT;
    tinfo.wstring_result = result;
    __tests.push_back(tinfo);
}

extern "C" bool wperf_test(PTEST_CONF tconf, PTEST_INFO tinfo)
{
    if (!tconf || !__pmu_device || !__pmu_cfg)
    {
        // tconf, __pmu_device, and __pmu_cfg should not be NULL.
        return false;
    }

    static size_t test_index = 0;

    try
    {
        if (!tinfo)
        {
            __ioctl_events.clear();
            __tests.clear();
            test_index = 0;
            struct hw_cfg hw_cfg;
            __pmu_device->query_hw_cfg(hw_cfg);

            // Only CORE events are supported at the moment.
            std::vector<enum evt_class> e_classes = { EVT_CORE };
            uint32_t enable_bits = __pmu_device->enable_bits(e_classes);

            // Tests for request.ioctl_events
            wperf_test_insert_bool(L"request.ioctl_events [EVT_CORE]", enable_bits & CTL_FLAG_CORE);
            wperf_test_insert_bool(L"request.ioctl_events [EVT_DSU]", enable_bits & CTL_FLAG_DSU);
            wperf_test_insert_bool(L"request.ioctl_events [EVT_DMC_CLK/EVT_DMC_CLKDIV2]", enable_bits & CTL_FLAG_DMC);

            // Test for pmu_device.vendor_name
            wperf_test_insert_wstring(L"pmu_device.vendor_name", __pmu_device->get_vendor_name(hw_cfg.vendor_id));

            // Test for product name (if available)
            wperf_test_insert_wstring(L"pmu_device.product_name", __pmu_device->m_product_name.c_str());
            wperf_test_insert_wstring(L"pmu_device.product_name(extended)", __pmu_device->get_product_name_ext().c_str());

            // Tests for pmu_device.events_query(events)
            std::map<enum evt_class, std::vector<uint16_t>> events;
            __pmu_device->events_query(events);
            wperf_test_insert_num(L"pmu_device.events_query(events) [EVT_CORE]", events.count(EVT_CORE) ? events[EVT_CORE].size() : 0);
            wperf_test_insert_num(L"pmu_device.events_query(events) [EVT_DSU]", events.count(EVT_DSU) ? events[EVT_DSU].size() : 0);
            wperf_test_insert_num(L"pmu_device.events_query(events) [EVT_DMC_CLK]", events.count(EVT_DMC_CLK) ? events[EVT_DMC_CLK].size() : 0);
            wperf_test_insert_num(L"pmu_device.events_query(events) [EVT_DMC_CLKDIV2]", events.count(EVT_DMC_CLKDIV2) ? events[EVT_DMC_CLKDIV2].size() : 0);

            // Tests for PMU_CTL_QUERY_HW_CFG
            wperf_test_insert_num(L"PMU_CTL_QUERY_HW_CFG [arch_id]", hw_cfg.arch_id);
            wperf_test_insert_num(L"PMU_CTL_QUERY_HW_CFG [core_num]", hw_cfg.core_num);
            wperf_test_insert_num(L"PMU_CTL_QUERY_HW_CFG [fpc_num]", hw_cfg.fpc_num);
            wperf_test_insert_num(L"PMU_CTL_QUERY_HW_CFG [gpc_num]", hw_cfg.gpc_num);
            wperf_test_insert_num(L"PMU_CTL_QUERY_HW_CFG [total_gpc_num]", hw_cfg.total_gpc_num);
            wperf_test_insert_num(L"PMU_CTL_QUERY_HW_CFG [part_id]", hw_cfg.part_id);
            wperf_test_insert_num(L"PMU_CTL_QUERY_HW_CFG [pmu_ver]", hw_cfg.pmu_ver);
            wperf_test_insert_num(L"PMU_CTL_QUERY_HW_CFG [rev_id]", hw_cfg.rev_id);
            wperf_test_insert_num(L"PMU_CTL_QUERY_HW_CFG [variant_id]", hw_cfg.variant_id);
            wperf_test_insert_num(L"PMU_CTL_QUERY_HW_CFG [vendor_id]", hw_cfg.vendor_id);
            wperf_test_insert_num(L"PMU_CTL_QUERY_HW_CFG [midr_value]", hw_cfg.midr_value);

            // Tests General Purpose Counters detection
            wperf_test_insert_num(L"gpc_nums[EVT_CORE]", __pmu_device->gpc_nums[EVT_CORE]);
            wperf_test_insert_num(L"gpc_nums[EVT_DSU]", __pmu_device->gpc_nums[EVT_DSU]);
            wperf_test_insert_num(L"gpc_nums[EVT_DMC_CLK]", __pmu_device->gpc_nums[EVT_DMC_CLK]);
            wperf_test_insert_num(L"gpc_nums[EVT_DMC_CLKDIV2]", __pmu_device->gpc_nums[EVT_DMC_CLKDIV2]);

            // Tests Fixed Purpose Counters detection
            wperf_test_insert_num(L"fpc_nums[EVT_CORE]", __pmu_device->fpc_nums[EVT_CORE]);
            wperf_test_insert_num(L"fpc_nums[EVT_DSU]", __pmu_device->fpc_nums[EVT_DSU]);
            wperf_test_insert_num(L"fpc_nums[EVT_DMC_CLK]", __pmu_device->fpc_nums[EVT_DMC_CLK]);
            wperf_test_insert_num(L"fpc_nums[EVT_DMC_CLKDIV2]", __pmu_device->fpc_nums[EVT_DMC_CLKDIV2]);

            std::map<enum evt_class, std::deque<struct evt_noted>> events_map;
            for (int i = 0; i < tconf->num_events; i++)
            {
                events_map[EVT_CORE].push_back({tconf->events[i], EVT_NORMAL, L"e"});
            }
            std::map<std::wstring, metric_desc>& metrics = __pmu_device->builtin_metrics;
            for (int i = 0; i < tconf->num_metrics; i++)
            {
                const wchar_t* metric = tconf->metric_events[i];
                if (metrics.find(metric) == metrics.end())
                {
                    // Not a valid builtin metric.
                    return false;
                }

                metric_desc desc = metrics[metric];
                for (const auto& x : desc.events)
                    events_map[x.first].insert(events_map[x.first].end(), x.second.begin(), x.second.end());
            }

            std::map<enum evt_class, std::vector<struct evt_noted>> groups;
            set_event_padding(__ioctl_events, *__pmu_cfg, events_map, groups);

            for (const auto& e : __ioctl_events[EVT_CORE])
            {
                wperf_test_insert_evt_note(L"ioctl_events[EVT_CORE]", e);
            }

            // Configuration
            std::vector<std::wstring> config_strs;
            drvconfig::get_configs(config_strs);

            for (auto& name : config_strs)
            {
                LONG l = 0;
                std::wstring s = L"<unknown>";

                std::wstring config_name = L"config." + name;
                if (drvconfig::get(name, l))
                    wperf_test_insert_num(config_name.c_str(), l);
                else if (drvconfig::get(name, s))
                    wperf_test_insert_wstring(config_name.c_str(), s.c_str());
                else
                    wperf_test_insert_wstring(config_name.c_str(), s.c_str());
            }
        }
        else
        {
            if (test_index >= __tests.size())
            {
                return false;
            }

            *tinfo = __tests[test_index];
            test_index++;
        }
    }
    catch (...)
    {
        return false;
    }

    return true;
}
