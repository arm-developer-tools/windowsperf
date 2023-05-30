#pragma once
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

#include <windows.h>
#include <deque>
#include <fstream>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <vector>

#include "events.h"
#include "metric.h"
#include "wperf-common/iorequest.h"


struct evt_sample_src
{
    uint32_t index;
    uint32_t interval;
};

struct pmu_device_cfg
{
    uint8_t gpc_nums[EVT_CLASS_NUM];
    uint8_t fpc_nums[EVT_CLASS_NUM];
    uint8_t core_num;
    uint32_t pmu_ver;
    uint32_t dsu_cluster_num;
    uint32_t dsu_cluster_size;
    uint32_t dmc_num;
    bool has_dsu;
    bool has_dmc;
};

struct product_alias
{
    std::wstring alias;
    std::wstring name;
};

struct product_configuration
{
    std::wstring arch_str;
    uint8_t implementer;
    uint8_t major_revision;
    uint8_t minor_revision;
    uint8_t num_bus_slots;
    uint8_t num_slots;
    uint16_t part_num;
    std::wstring pmu_architecture;
    std::wstring product_name;
};

struct product_metric
{
    std::wstring name;              // :^)
    std::wstring events_raw;        // Raw string of events, comma separated e.g. "cpu_cycles,stall_backend"
    std::wstring metric_formula;    // Raw string with metric formula, e.g. "((STALL_BACKEND / CPU_CYCLES) * 100)"
    std::wstring metric_formula_sy; // Raw string with RPN equivalent expression of metric formula e.g. "STALL_BACKEND CPU_CYCLES / 100 *"
    std::wstring metric_unit;       // Raw string with metric unit "percent of cycles"
    std::wstring title;             // Metric title / short description
};


class pmu_device
{
public:
    pmu_device();
    ~pmu_device();

    void init();
    HANDLE init_device();
    void hw_cfg_detected(struct hw_cfg& hw_cfg);

    // post_init members
    void post_init(std::vector<uint8_t> cores_idx_init, uint32_t dmc_idx_init, bool timeline_mode_init, uint32_t enable_bits);

    // Sampling
    struct pmu_sample_summary
    {
        uint64_t sample_generated;
        uint64_t sample_dropped;
    };

    void set_sample_src(std::vector<struct evt_sample_src>& sample_sources, bool sample_kernel);
    bool get_sample(std::vector<FrameChain>& sample_info);  // Return false if sample buffer was empty
    void start_sample();
    void stop_sample();
    // Sampling

    // Timeline
    void timeline_init();
    void timeline_release();
    void timeline_params(const std::map<enum evt_class, std::vector<struct evt_noted>>& events, double count_interval, bool include_kernel);
    void timeline_header(const std::map<enum evt_class, std::vector<struct evt_noted>>& events);
    // Timeline

    void set_builtin_metrics(std::wstring metric_name, std::wstring raw_str);
    void start(uint32_t flags);
    void stop(uint32_t flags);
    void reset(uint32_t flags);
    void events_assign(uint32_t core_idx, std::map<enum evt_class, std::vector<struct evt_noted>> events, bool include_kernel);
    void core_events_read_nth(uint8_t core_no);
    void core_events_read();
    void dsu_events_read_nth(uint8_t core_no);
    void dsu_events_read(void);
    void dmc_events_read(void);
    void events_query(std::map<enum evt_class, std::vector<uint16_t>>& events_out);
    void print_core_stat(std::vector<struct evt_noted>& events); 
    void print_dsu_stat(std::vector<struct evt_noted>& events, bool report_l3_metric);
    void print_dmc_stat(std::vector<struct evt_noted>& clk_events, std::vector<struct evt_noted>& clkdiv2_events, bool report_ddr_bw_metric);
    void do_list(const std::map<std::wstring, metric_desc>& metrics);
    void do_list_prep_events(_Out_ std::vector<std::wstring>& col_alias_name,
        _Out_ std::vector<std::wstring>& col_raw_index, _Out_ std::vector<std::wstring>& col_event_type);   // part of do_list()
    void do_list_prep_metrics(_Out_ std::vector<std::wstring>& col_metric,
        _Out_ std::vector<std::wstring>& col_events, _In_ const std::map<std::wstring, metric_desc>& metrics);  // part of do_list()
    void do_test(uint32_t enable_bits, std::map<enum evt_class, std::vector<struct evt_noted>>& ioctl_events);
    void do_test_prep_tests(_Out_ std::vector<std::wstring>& col_test_name, _Out_ std::vector<std::wstring>& col_test_result,
        _In_ uint32_t enable_bits, std::map<enum evt_class, _In_ std::vector<struct evt_noted>>& ioctl_events); // part of do_test()
    void do_version(_Out_ version_info& driver_ver);
    void do_version_query(_Out_ version_info& driver_ver);    // part of do_version()

    BOOL DeviceAsyncIoControl(_In_ HANDLE hDevice, _In_ LPVOID lpBuffer, _In_ DWORD nNumberOfBytesToWrite,
        _Out_ LPVOID lpOutBuffer, _In_ DWORD nOutBufferSize, _Out_ LPDWORD lpBytesReturned);

    void get_pmu_device_cfg(struct pmu_device_cfg& cfg);

    uint32_t stop_bits();
    uint32_t enable_bits(_In_ std::vector<enum evt_class>& e_classes);

    uint8_t core_num;
    std::map<std::wstring, metric_desc> builtin_metrics;
    bool has_dsu;
    bool has_dmc;
    uint32_t dsu_cluster_num;
    uint32_t dsu_cluster_size;
    uint32_t dmc_num;
    bool do_verbose;

    uint8_t gpc_nums[EVT_CLASS_NUM];
    uint8_t fpc_nums[EVT_CLASS_NUM];

    static std::map<uint8_t, wchar_t*> arm64_vendor_names;

    // Telemetry Solution meta-data
    static std::map<std::wstring, struct product_configuration> m_product_configuration;
    static std::map<std::wstring, std::wstring> m_product_alias;
    std::map<std::wstring, std::map<std::wstring, struct product_metric>> m_product_metrics;     // [product] -> [metrics_name -> product_metric]
    std::wstring m_product_name;        // Product name used to index Telemetry Solution data structures
    std::wstring get_product_name_ext();    // Human friendly product string

    const ReadOut* get_core_outs() { return core_outs.get();  };
    std::vector<uint8_t> get_cores_idx() { return cores_idx; };

private:
    /// <summary>
    /// This two dimentional array stores names of cores (DeviceDesc) for all (DSU clusers) x (cluster core no)
    ///
    /// You can access them like this:
    ///
    ///     [dsu_cluster_num][core_num_idx] -> WSTRING(DeviceDesc)
    ///
    /// Where (example for 80 core ARMv8 CPU:
    ///
    ///     dsu_cluster_num - number of DSU clusters, e.g. 0-39 (40 clusters)
    ///     core_num_idx    - index of core in DSU clusters, e.g. 0-1 (2 cores per cluster)
    ///
    /// You can print it like this:
    ///
    /// int i = 0;
    /// for (const auto& a : m_dsu_cluster_makeup)
    /// {
    ///     int j = 0;
    ///     for (const auto& b : a)
    ///     {
    ///         std::wcout << L"[" << i << L"]" << L"[" << j << L"]" << b << std::endl;
    ///         j++;
    ///     }
    ///     i++;
    /// }
    ///
    /// </summary>
    std::vector <std::vector < std::wstring >> m_dsu_cluster_makeup;

    bool all_cores_p() {
        return cores_idx.size() > 1;
    }

    bool detect_armh_dsu();
    bool detect_armh_dma();
    
    void query_hw_cfg(struct hw_cfg& out);

    const wchar_t* get_vendor_name(uint8_t vendor_id);

    // Use this function to print to wcerr runtime warnings in verbose mode.
    // Do not use this function for debug. Instead use WindowsPerfDbgPrint().
    void warning(const std::wstring wrn);

    HANDLE m_device_handle;
    uint32_t pmu_ver;
    const wchar_t* vendor_name;
    std::vector<uint8_t> cores_idx;                     // Cores
    std::set<uint32_t, std::less<uint32_t>> dsu_cores;  // DSU used by cores in 'cores_idx'
    uint8_t dmc_idx;
    std::unique_ptr<ReadOut[]> core_outs;
    std::unique_ptr<DSUReadOut[]> dsu_outs;
    std::unique_ptr<DMCReadOut[]> dmc_outs;
    bool multiplexings[EVT_CLASS_NUM];
    bool timeline_mode;
    bool count_kernel;
    uint32_t enc_bits;
    std::wofstream timeline_outfiles[EVT_CLASS_NUM];
    std::vector<std::pair<uint64_t, uint64_t>> dmc_regions;
    uint32_t m_enable_bits = 0;
};
