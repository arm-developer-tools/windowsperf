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


#include <numeric>
#include <assert.h>
#include "wperf-common/gitver.h"
#include "pmu_device.h"
#include "exception.h"
#include "events.h"
#include "output.h"
#include "utils.h"
#include "parsers.h"
#include "wperf-common/public.h"
#include "wperf.h"
#include "config.h"
#include "timeline.h"

#include <cfgmgr32.h>
#include <devpkey.h>


std::map<uint8_t, wchar_t*> pmu_device::arm64_vendor_names =
{
    {0x41, L"Arm Limited"},
    {0x42, L"Broadcomm Corporation"},
    {0x43, L"Cavium Inc"},
    {0x44, L"Digital Equipment Corporation"},
    {0x46, L"Fujitsu Ltd"},
    {0x49, L"Infineon Technologies AG"},
    {0x4D, L"Motorola or Freescale Semiconductor Inc"},
    {0x4E, L"NVIDIA Corporation"},
    {0x50, L"Applied Micro Circuits Corporation"},
    {0x51, L"Qualcomm Inc"},
    {0x56, L"Marvell International Ltd"},
    {0x69, L"Intel Corporation"},
    {0xC0, L"Ampere Computing"}
};

std::map<std::wstring, struct product_configuration> pmu_device::m_product_configuration = {
#define WPERF_TS_EVENTS(...)
#define WPERF_TS_METRICS(...)
#define WPERF_TS_PRODUCT_CONFIGURATION(A,B,C,D,E,F,G,H,I,J) {std::wstring(L##A),{std::wstring(L##B),C,D,E,F,G,H,std::wstring(L##I),std::wstring(L##J)}},
#define WPERF_TS_ALIAS(...)
#include "wperf-common/telemetry-solution-data.def"
#undef WPERF_TS_EVENTS
#undef WPERF_TS_METRICS
#undef WPERF_TS_PRODUCT_CONFIGURATION
#undef WPERF_TS_ALIAS
};

std::map<std::wstring, std::wstring> pmu_device::m_product_alias =
{
#define WPERF_TS_EVENTS(...)
#define WPERF_TS_METRICS(...)
#define WPERF_TS_PRODUCT_CONFIGURATION(...)
#define WPERF_TS_ALIAS(A,B) {L##A,L##B},
#include "wperf-common/telemetry-solution-data.def"
#undef WPERF_TS_EVENTS
#undef WPERF_TS_METRICS
#undef WPERF_TS_PRODUCT_CONFIGURATION
#undef WPERF_TS_ALIAS
};


pmu_device::pmu_device() : m_device_handle(NULL), count_kernel(false), has_dsu(false), dsu_cluster_num(0),
    dsu_cluster_size(0), has_dmc(false), dmc_num(0), enc_bits(0), core_num(0),
    dmc_idx(0), pmu_ver(0), timeline_mode(false), vendor_name(0), do_verbose(false)
{
    for (int e = EVT_CLASS_FIRST; e < EVT_CLASS_NUM; e++)
        multiplexings[e] = false;

    memset(gpc_nums, 0, sizeof gpc_nums);
    memset(fpc_nums, 0, sizeof fpc_nums);

    init_ts_metrics();
    init_ts_events();
    init_arm_events();
}

void pmu_device::init_ts_metrics()
{   // Initialize Telemetry Solution metrics from each product
#       define WPERF_TS_EVENTS(...)
#       define WPERF_TS_METRICS(A,B,C,D,E,F,G) m_product_metrics[std::wstring(L##A)][std::wstring(L##B)] = { std::wstring(L##B),std::wstring(L##C),std::wstring(L##D),std::wstring(L##E),std::wstring(L##F),std::wstring(L##G) };
#       define WPERF_TS_PRODUCT_CONFIGURATION(...)
#       define WPERF_TS_ALIAS(...)
#       include "wperf-common/telemetry-solution-data.def"
#       undef WPERF_TS_EVENTS
#       undef WPERF_TS_METRICS
#       undef WPERF_TS_PRODUCT_CONFIGURATION
#       undef WPERF_TS_ALIAS
}

void pmu_device::init_ts_events()
{   // Initialize Telemetry Solution events for each product
#       define WPERF_TS_EVENTS(A,B,C,D,E) m_product_events[std::wstring(L##A)][std::wstring(L##D)] = { std::wstring(L##D),uint16_t(C),std::wstring(L##E) };
#       define WPERF_TS_METRICS(...)
#       define WPERF_TS_PRODUCT_CONFIGURATION(...)
#       define WPERF_TS_ALIAS(...)
#       include "wperf-common/telemetry-solution-data.def"
#       undef WPERF_TS_EVENTS
#       undef WPERF_TS_METRICS
#       undef WPERF_TS_PRODUCT_CONFIGURATION
#       undef WPERF_TS_ALIAS
}

void pmu_device::init_arm_events()
{   // Initialize generic armv8-a and armv9-a
#       define WPERF_ARMV8_ARCH_EVENTS(A,B,C,D,E) m_product_events[std::wstring(L##A)][std::wstring(L##D)] = { std::wstring(L##D),uint16_t(C),std::wstring(L##E) };
#       include "wperf-common/armv8-arch-events.def"
#       undef WPERF_ARMV8_ARCH_EVENTS

#       define WPERF_ARMV9_ARCH_EVENTS(A,B,C,D,E) m_product_events[std::wstring(L##A)][std::wstring(L##D)] = { std::wstring(L##D),uint16_t(C),std::wstring(L##E) };
#       include "wperf-common/armv9-arch-events.def"
#       undef WPERF_ARMV9_ARCH_EVENTS
}

HANDLE pmu_device::init_device()
{
    WCHAR G_DevicePath[MAX_DEVPATH_LENGTH];
    HANDLE hDevice = INVALID_HANDLE_VALUE;
    const size_t buf_len = sizeof(G_DevicePath) / sizeof(G_DevicePath[0]);

    if (GetDevicePath((LPGUID)&GUID_DEVINTERFACE_WINDOWSPERF, G_DevicePath, buf_len) == FALSE)
        throw fatal_exception("Error: Failed to find device path");

    hDevice = CreateFile(G_DevicePath,
        GENERIC_READ | GENERIC_WRITE,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        NULL,
        OPEN_EXISTING,
        0,
        NULL);

    if (hDevice == INVALID_HANDLE_VALUE)
        throw fatal_exception("Error: Failed to open device");

    return hDevice;
}

void pmu_device::init()
{
    m_device_handle = init_device();

    struct hw_cfg hw_cfg;
    query_hw_cfg(hw_cfg);

    assert(hw_cfg.core_num <= UCHAR_MAX);
    core_num = (UINT8)hw_cfg.core_num;
    fpc_nums[EVT_CORE] = hw_cfg.fpc_num;
    uint8_t gpc_num = hw_cfg.gpc_num;
    gpc_nums[EVT_CORE] = gpc_num;
    pmu_ver = hw_cfg.pmu_ver;

    vendor_name = get_vendor_name(hw_cfg.vendor_id);
    core_outs = std::make_unique<ReadOut[]>(core_num);
    memset(core_outs.get(), 0, sizeof(ReadOut) * core_num);

    drvconfig::init();

    hw_cfg_detected(hw_cfg);

    // Detect unCore PMU from Arm Ltd - System Cache
    has_dsu = detect_armh_dsu();
    if (has_dsu)
    {
        struct dsu_ctl_hdr ctl;
        struct dsu_cfg cfg;
        DWORD res_len;

        ctl.action = DSU_CTL_INIT;
        assert(dsu_cluster_num <= USHRT_MAX);
        assert(dsu_cluster_size <= USHRT_MAX);
        ctl.cluster_num = (uint16_t)dsu_cluster_num;
        ctl.cluster_size = (uint16_t)dsu_cluster_size;
        BOOL status = DeviceAsyncIoControl(m_device_handle, DSU_CTL_INIT, &ctl, sizeof(dsu_ctl_hdr), &cfg, sizeof(dsu_cfg), &res_len);
        if (!status)
        {
            m_out.GetErrorOutputStream() << L"DSU_CTL_INIT failed" << std::endl;
            throw fatal_exception("ERROR_PMU_INIT");
        }

        if (res_len != sizeof(struct dsu_cfg))
        {
            m_out.GetErrorOutputStream() << L"DSU_CTL_INIT returned unexpected length of data" << std::endl;
            throw fatal_exception("ERROR_PMU_INIT");
        }

        gpc_nums[EVT_DSU] = cfg.gpc_num;
        fpc_nums[EVT_DSU] = cfg.fpc_num;

        dsu_outs = std::make_unique<DSUReadOut[]>(dsu_cluster_num);
        memset(dsu_outs.get(), 0, sizeof(DSUReadOut) * dsu_cluster_num);

        set_builtin_metrics(L"l3_cache", L"/dsu/l3d_cache,/dsu/l3d_cache_refill");
    }

    // unCore PMU - DDR controller
    has_dmc = detect_armh_dma();
    if (has_dmc)
    {
        size_t len = sizeof(dmc_ctl_hdr) + dmc_regions.size() * sizeof(uint64_t) * 2;
        std::unique_ptr<uint8_t[]> buf = std::make_unique<uint8_t[]>(len);

        struct dmc_ctl_hdr* ctl = (struct dmc_ctl_hdr*)buf.get();
        struct dmc_cfg cfg;
        DWORD res_len;

        for (int i = 0; i < dmc_regions.size(); i++)
        {
            ctl->addr[i * 2] = dmc_regions[i].first;
            ctl->addr[i * 2 + 1] = dmc_regions[i].second;
        }

        ctl->action = DMC_CTL_INIT;
        assert(dmc_regions.size() <= UCHAR_MAX);
        ctl->dmc_num = (uint8_t)dmc_regions.size();
        BOOL status = DeviceAsyncIoControl(m_device_handle, DMC_CTL_INIT, ctl, (DWORD)len, &cfg, (DWORD)sizeof(dmc_cfg), &res_len);
        if (!status)
        {
            m_out.GetErrorOutputStream() << L"DMC_CTL_INIT failed" << std::endl;
            throw fatal_exception("ERROR_PMU_INIT");
        }

        gpc_nums[EVT_DMC_CLK] = cfg.clk_gpc_num;
        fpc_nums[EVT_DMC_CLK] = 0;
        gpc_nums[EVT_DMC_CLKDIV2] = cfg.clkdiv2_gpc_num;
        fpc_nums[EVT_DMC_CLKDIV2] = 0;

        dmc_outs = std::make_unique<DMCReadOut[]>(ctl->dmc_num);
        memset(dmc_outs.get(), 0, sizeof(DMCReadOut) * ctl->dmc_num);

        set_builtin_metrics(L"ddr_bw", L"/dmc_clkdiv2/rdwr");
    }
}

void pmu_device::hw_cfg_detected(struct hw_cfg& hw_cfg)
{
    // Support metrics based on Arm's default core implementation
    if (hw_cfg.vendor_id == 0x41 || hw_cfg.vendor_id == 0x51)
    {
        for (const auto& metric_name : metric_get_builtin_metric_names())
        {
            std::wstring metric_str = metric_gen_metric_based_on_gpc_num(metric_name, hw_cfg.gpc_num);
            set_builtin_metrics(metric_name, metric_str);
        }
    }

    // Support metrics from detected HW via Telemetry Solution data
    for (auto const& [alias, name] : m_product_alias)
        if (m_product_configuration.count(name))
            m_product_configuration[alias] = m_product_configuration[name];

    m_product_name = m_PRODUCT_ARMV8A;

    // Add metrcs based on detected Telemetry Solution product
    for (auto const& [prod_name, prod_conf] : m_product_configuration)
        if (hw_cfg.vendor_id == prod_conf.implementer && hw_cfg.part_id == prod_conf.part_num)
            if (m_product_metrics.count(prod_name))
            {
                m_product_name = prod_name;     // Save for later

                for (const auto& [metric_name, metric] : m_product_metrics[prod_name])
                {
                    std::wstring raw_str = L"{" + metric.events_raw + L"}";
                    set_builtin_metrics(metric_name, raw_str);
                }
                break;
            }
}

std::wstring pmu_device::get_product_name_ext()
{
    std::wstring ret;
    
    if (m_product_configuration.count(m_product_name)
        && m_product_name.size())
    {
        std::wstring product_name = m_product_configuration[m_product_name].product_name;
        std::wstring arch_str = m_product_configuration[m_product_name].arch_str;
        std::wstring pmu_architecture = m_product_configuration[m_product_name].pmu_architecture;
        ret = product_name + L" (" + m_product_name + L")" + L", " + arch_str + L", " + pmu_architecture;
    }

    return ret;
}

std::wstring pmu_device::get_all_product_name_str()
{
    std::wstring result;
    auto products = get_product_names();

    for (const auto& s : products)
    {
        if (result.empty())
            result += s;
        else
            result += L"," + s;
    }

    return result;
}

std::wstring pmu_device::get_all_aliases_str()
{
    std::wstring result;
    for (const auto& [key, value] : m_product_alias)
    {
        std::wstring s = L"(" + key + L":" + value + L")";
        if (result.empty())
            result += s;
        else
            result += L"," + s;
    }

    return result;
}

std::vector<std::wstring> pmu_device::get_product_names()
{
    std::set<std::wstring> products;
    std::vector<std::wstring> result;

    for (const auto& [key, value] : m_product_alias)
        products.insert(key);

    for (const auto& [key, value] : m_product_metrics)
        products.insert(key);

    for (const auto& [key, value] : m_product_events)
        products.insert(key);

    for (const auto& v : products)
        result.push_back(v);

    std::sort(result.begin(), result.end());

    return result;
}

void pmu_device::timeline_init()
{
    if (!timeline_mode)
        return;

    timeline::init();

    char buf[MAX_PATH];
    time_t rawtime;
    struct tm timeinfo;

    time(&rawtime);
    localtime_s(&timeinfo, &rawtime);

    for (int e = EVT_CLASS_FIRST; e < EVT_CLASS_NUM; e++)
    {
        if (e == EVT_CORE && !(enc_bits & CTL_FLAG_CORE))
            continue;

        if (e == EVT_DSU && !(enc_bits & CTL_FLAG_DSU))
            continue;

        if ((e == EVT_DMC_CLK || e == EVT_DMC_CLKDIV2) && !(enc_bits & CTL_FLAG_DMC))
            continue;

        if (strftime(buf, sizeof(buf), "%Y_%m_%d_%H_%M_%S.", &timeinfo) == 0)
            throw fatal_exception("timestamp conversion failed in timeline mode");

        std::string timestamp(buf);
        std::string prefix("wperf_system_side_");
        if (all_cores_p() == false)
            prefix = "wperf_core_" + std::to_string(cores_idx[0]) + "_";

        std::string filename = prefix + timestamp +
            MultiByteFromWideString(pmu_events_get_evt_class_name(static_cast<enum evt_class>(e))) + ".csv";

        if (do_verbose)
            m_out.GetOutputStream() << L"timeline file: " << L"'" << std::wstring(filename.begin(), filename.end()) << L"'" << std::endl;

        timeline::timeline_headers[static_cast<enum evt_class>(e)].filename = filename;
    }
}


// post_init members
void pmu_device::post_init(std::vector<uint8_t> cores_idx_init, uint32_t dmc_idx_init, bool timeline_mode_init, uint32_t enable_bits)
{
    // Initliaze core numbers, please note we are sorting cores ascending
    // because we may relay in ascending order for some simple algorithms.
    // For example in wperf-driver::deviceControl() we only init one core
    // per DSU cluster.
    cores_idx = cores_idx_init;
    std::sort(cores_idx.begin(), cores_idx.end());  // Keep this sorting!
    // We want to keep only unique cores
    cores_idx.erase(unique(cores_idx.begin(), cores_idx.end()), cores_idx.end());

    dmc_idx = (uint8_t)dmc_idx_init;
    timeline_mode = timeline_mode_init;
    enc_bits = enable_bits;

    if (has_dsu)
        // Gather all DSU numbers for specified core in set of unique DSU numbers
        for (uint32_t i : cores_idx)
            dsu_cores.insert(i / dsu_cluster_size);

    timeline_init();
}

void pmu_device::timeline_close()
{
    timeline::print();
}

pmu_device::~pmu_device()
{
    timeline_close();
    CloseHandle(m_device_handle);
}

void pmu_device::set_sample_src(std::vector<struct evt_sample_src>& sample_sources, bool sample_kernel)
{
    PMUSampleSetSrcHdr* ctl;
    DWORD res_len;
    size_t sz;

    if (sample_sources.size())
    {
        sz = sizeof(PMUSampleSetSrcHdr) + sample_sources.size() * sizeof(SampleSrcDesc);
        ctl = reinterpret_cast<PMUSampleSetSrcHdr*>(new uint8_t[sz]);
        for (int i = 0; i < sample_sources.size(); i++)
        {
            SampleSrcDesc* dst = ctl->sources + i;
            struct evt_sample_src src = sample_sources[i];
            dst->event_src = src.index;
            dst->interval = src.interval;
            dst->filter_bits = sample_kernel ? 0 : FILTER_BIT_EXCL_EL1;
        }
    }
    else
    {
        sz = sizeof(PMUSampleSetSrcHdr) + sizeof(SampleSrcDesc);
        ctl = reinterpret_cast<PMUSampleSetSrcHdr*>(new uint8_t[sz]);
        ctl->sources[0].event_src = CYCLE_EVT_IDX;
        ctl->sources[0].interval = 0x8000000;
        ctl->sources[0].filter_bits = sample_kernel ? 0 : FILTER_BIT_EXCL_EL1;
    }

    ctl->action = PMU_CTL_SAMPLE_SET_SRC;
    ctl->core_idx = cores_idx[0];   // Only one core for sampling!
    BOOL status = DeviceAsyncIoControl(m_device_handle, PMU_CTL_SAMPLE_SET_SRC, ctl, (DWORD)sz, NULL, 0, &res_len);
    delete[] ctl;
    if (!status)
        throw fatal_exception("PMU_CTL_SAMPLE_SET_SRC failed");
}

// Return false if sample buffer was empty
bool pmu_device::get_sample(std::vector<FrameChain>& sample_info)
{
    struct PMUCtlGetSampleHdr hdr;
    hdr.action = PMU_CTL_SAMPLE_GET;
    hdr.core_idx = cores_idx[0];
    DWORD res_len;

    PMUSamplePayload framesPayload = { 0 };

    BOOL status = DeviceAsyncIoControl(m_device_handle, PMU_CTL_SAMPLE_GET, &hdr, sizeof(struct PMUCtlGetSampleHdr), &framesPayload, sizeof(PMUSamplePayload), &res_len);
    if (!status)
        throw fatal_exception("PMU_CTL_SAMPLE_GET failed");

    if (framesPayload.size == 0)
        return false;

    FrameChain* frames = (FrameChain*)framesPayload.payload;
    for (int i = 0; i < (res_len / sizeof(FrameChain)); i++)
        sample_info.push_back(frames[i]);

    return true;
}

void pmu_device::start_sample()
{
    struct pmu_ctl_hdr ctl;
    DWORD res_len;

    ctl.action = PMU_CTL_SAMPLE_START;
    ctl.cores_idx.cores_count = 1;
    ctl.cores_idx.cores_no[0] = cores_idx[0];
    BOOL status = DeviceAsyncIoControl(m_device_handle, PMU_CTL_SAMPLE_START, &ctl, sizeof(struct pmu_ctl_hdr), NULL, 0, &res_len);
    if (!status)
        throw fatal_exception("PMU_CTL_SAMPLE_START failed");
}

struct pmu_sample_summary
{
    uint64_t sample_generated;
    uint64_t sample_dropped;
};

void pmu_device::stop_sample()
{
    struct pmu_ctl_hdr ctl;
    struct pmu_sample_summary summary;
    DWORD res_len;

    ctl.action = PMU_CTL_SAMPLE_STOP;
    ctl.cores_idx.cores_count = 1;
    ctl.cores_idx.cores_no[0] = cores_idx[0];
    BOOL status = DeviceAsyncIoControl(m_device_handle, PMU_CTL_SAMPLE_STOP, &ctl, sizeof(struct pmu_ctl_hdr), &summary, sizeof(struct pmu_sample_summary), &res_len);
    if (!status)
        throw fatal_exception("PMU_CTL_SAMPLE_STOP failed");

    if (do_verbose)
    {
        double drop_rate = (summary.sample_generated != 0)
            ? ((double)summary.sample_dropped / (double)summary.sample_generated) * 100.0
            : 100.0;
        m_out.GetOutputStream() << L"=================" << std::endl;
        m_out.GetOutputStream() << L"sample generated: " << std::dec << summary.sample_generated << std::endl;
        m_out.GetOutputStream() << L"sample dropped  : " << std::dec << summary.sample_dropped << std::endl;
        m_out.GetOutputStream() << L"sample drop rate: " << std::dec << DoubleToWideString(drop_rate) << L"%" << std::endl;
    }

    sample_summary.sample_generated = summary.sample_generated;
    sample_summary.sample_dropped = summary.sample_dropped;
    m_globalSamplingJSON.m_samples_generated = summary.sample_generated;
    m_globalSamplingJSON.m_samples_dropped = summary.sample_dropped;
}

void pmu_device::set_builtin_metrics(std::wstring metric_name, std::wstring raw_str)
{
    metric_desc mdesc;
    mdesc.raw_str = raw_str;

    struct pmu_device_cfg pmu_cfg;
    get_pmu_device_cfg(pmu_cfg);

    parse_events_str(mdesc.raw_str, mdesc.events, mdesc.groups, metric_name, pmu_cfg);
    builtin_metrics[metric_name] = mdesc;
    mdesc.events.clear();
    mdesc.groups.clear();
}

void pmu_device::start(uint32_t flags = CTL_FLAG_CORE)
{
    struct pmu_ctl_hdr ctl;
    DWORD res_len;

    ctl.action = PMU_CTL_START;
    ctl.cores_idx.cores_count = cores_idx.size();
    std::copy(cores_idx.begin(), cores_idx.end(), ctl.cores_idx.cores_no);

    ctl.dmc_idx = dmc_idx;
    ctl.flags = flags;
    drvconfig::get(L"count.period", ctl.period);

    BOOL status = DeviceAsyncIoControl(m_device_handle, PMU_CTL_START, &ctl, sizeof(struct pmu_ctl_hdr), NULL, 0, &res_len);
    if (!status)
        throw fatal_exception("PMU_CTL_START failed");
}

void pmu_device::stop(uint32_t flags = CTL_FLAG_CORE)
{
    struct pmu_ctl_hdr ctl;
    DWORD res_len;

    ctl.action = PMU_CTL_STOP;
    ctl.cores_idx.cores_count = cores_idx.size();
    std::copy(cores_idx.begin(), cores_idx.end(), ctl.cores_idx.cores_no);
    ctl.dmc_idx = dmc_idx;
    ctl.flags = flags;

    BOOL status = DeviceAsyncIoControl(m_device_handle, PMU_CTL_STOP, &ctl, sizeof(struct pmu_ctl_hdr), NULL, 0, &res_len);
    if (!status)
        throw fatal_exception("PMU_CTL_STOP failed");
}

void pmu_device::reset(uint32_t flags = CTL_FLAG_CORE)
{
    struct pmu_ctl_hdr ctl;
    DWORD res_len;

    ctl.action = PMU_CTL_RESET;
    ctl.cores_idx.cores_count = cores_idx.size();
    std::copy(cores_idx.begin(), cores_idx.end(), ctl.cores_idx.cores_no);
    ctl.dmc_idx = dmc_idx;
    ctl.flags = flags;
    BOOL status = DeviceAsyncIoControl(m_device_handle, PMU_CTL_RESET, &ctl, sizeof(struct pmu_ctl_hdr), NULL, 0, &res_len);
    if (!status)
        throw fatal_exception("PMU_CTL_RESET failed");
}

void pmu_device::timeline_params(const std::map<enum evt_class, std::vector<struct evt_noted>>& events, double count_interval, bool include_kernel)
{
    if (!timeline_mode)
        return;

    bool multiplexing = multiplexings[EVT_CORE];

    for (const auto& [e_class, value] : events)
    {
        auto& timeline_header = timeline::timeline_headers[e_class];

        timeline_header.multiplexing = multiplexing;
        timeline_header.include_kernel = include_kernel;
        timeline_header.count_interval = count_interval;
        timeline_header.vendor_name = vendor_name;
        timeline_header.event_class = pmu_events_get_evt_class_name(e_class);
    }
}

void pmu_device::timeline_header(const std::map<enum evt_class, std::vector<struct evt_noted>>& events)
{
    if (!timeline_mode)
        return;

    bool multiplexing = multiplexings[EVT_CORE];

    for (const auto& [e_class, unit_events] : events)
    {
        auto& timeline_header = timeline::timeline_headers[e_class];
        auto& timeline_header_cores = timeline::timeline_header_cores[e_class];
        auto& timeline_header_event_names = timeline::timeline_header_event_names[e_class];

        timeline_header.multiplexing = multiplexing;

        uint32_t real_event_num = 0;
        for (const auto& e : unit_events)
        {
            if (e.type == EVT_PADDING)
                continue;

            real_event_num++;
        }

        std::vector<uint32_t> iter;
        uint32_t comma_num, comma_num2;
        if (e_class == EVT_DMC_CLK || e_class == EVT_DMC_CLKDIV2)
        {
            if (dmc_idx == ALL_DMC_CHANNEL)
            {
                comma_num = real_event_num * dmc_num - 1;
                comma_num2 = real_event_num - 1;

                iter.resize(dmc_num);
                std::iota(iter.begin(), iter.end(), 0);
            }
            else
            {
                comma_num = real_event_num - 1;
                comma_num2 = comma_num;

                iter.push_back(dmc_idx);
            }
        }
        else if (all_cores_p())
        {
            uint32_t block_num = e_class == EVT_DSU ? dsu_cluster_num : core_num;

            comma_num = multiplexing ? ((real_event_num + 1) * 2 * block_num - 1)
                : ((real_event_num + 1) * block_num - 1);
            comma_num2 = multiplexing ? ((real_event_num + 1) * 2 - 1) : real_event_num;

            iter.resize(cores_idx.size());
            std::copy(cores_idx.begin(), cores_idx.end(), iter.begin());
        }
        else
        {
            comma_num = multiplexing ? ((real_event_num + 1) * 2 - 1) : real_event_num;
            comma_num2 = comma_num;

            iter.resize(cores_idx.size());
            std::copy(cores_idx.begin(), cores_idx.end(), iter.begin());
        }

        uint32_t i_inc = e_class == EVT_DSU ? dsu_cluster_size : 1;
        
        // This sets string "core 0, core 1, ..."
        for (uint32_t idx : iter)
        {
            std::wstring core_str = std::wstring(pmu_events_get_evt_class_name(e_class)) + L" " + std::to_wstring(idx / i_inc);

            timeline_header_cores.push_back(core_str);
            for (uint32_t i = 0; i < comma_num2; i++)
                timeline_header_cores.push_back(core_str);
            idx += i_inc;
        }

        // This sets list of events
        uint32_t event_num = (uint32_t)unit_events.size();
        for (uint32_t i : iter)
        {
            std::wstring event_str;

            if (e_class == EVT_DMC_CLK || e_class == EVT_DMC_CLKDIV2)
            {
                for (uint32_t idx = 0; idx < event_num; idx++)
                {
                    if (unit_events[idx].type == EVT_PADDING)
                        continue;

                    event_str = std::wstring(pmu_events_get_event_name(unit_events[idx].index, e_class));
                    timeline_header_event_names.push_back(event_str);
                }
            }
            else if (multiplexing)
            {
                timeline_header_event_names.push_back(L"cycle");
                timeline_header_event_names.push_back(L"sched_times");

                for (uint32_t idx = 0; idx < event_num; idx++)
                {
                    if (unit_events[idx].type == EVT_PADDING)
                        continue;

                    event_str = std::wstring(pmu_events_get_event_name(unit_events[idx].index));
                    timeline_header_event_names.push_back(event_str);
                    timeline_header_event_names.push_back(L"sched_times");
                }
            }
            else
            {
                timeline_header_event_names.push_back(L"cycle");

                for (uint32_t idx = 0; idx < event_num; idx++)
                {
                    if (unit_events[idx].type == EVT_PADDING)
                        continue;

                    event_str = std::wstring(pmu_events_get_event_name(unit_events[idx].index));
                    timeline_header_event_names.push_back(event_str);
                }
            }

            i += i_inc;
        }
    }
}

void pmu_device::events_assign(uint32_t core_idx, std::map<enum evt_class, std::vector<struct evt_noted>> events, bool include_kernel)
{
    size_t acc_sz = 0;

    for (const auto& a : events)
    {
        enum evt_class e_class = a.first;
        size_t e_num = a.second.size();

        acc_sz += sizeof(struct evt_hdr) + e_num * sizeof(uint16_t);
        multiplexings[e_class] = !!(e_num > gpc_nums[e_class]);
    }

    if (!acc_sz)
        return;

    acc_sz += sizeof(struct pmu_ctl_evt_assign_hdr);

    std::unique_ptr<uint8_t[]> buf = std::make_unique<uint8_t[]>(acc_sz);

    DWORD res_len;
    struct pmu_ctl_evt_assign_hdr* ctl =
        reinterpret_cast<struct pmu_ctl_evt_assign_hdr*>(buf.get());

    ctl->action = PMU_CTL_ASSIGN_EVENTS;
    ctl->core_idx = core_idx;
    ctl->dmc_idx = dmc_idx;
    count_kernel = include_kernel;
    ctl->filter_bits = count_kernel ? 0 : FILTER_BIT_EXCL_EL1;
    uint16_t* ctl2 =
        reinterpret_cast<uint16_t*>(buf.get() + sizeof(struct pmu_ctl_evt_assign_hdr));

    for (const auto& a : events)
    {
        enum evt_class e_class = a.first;
        struct evt_hdr* hdr = reinterpret_cast<struct evt_hdr*>(ctl2);

        hdr->evt_class = e_class;
        hdr->num = (UINT16)a.second.size();
        uint16_t* payload = reinterpret_cast<uint16_t*>(hdr + 1);

        for (const auto& b : a.second)
            *payload++ = b.index;

        ctl2 = payload;
    }

    BOOL status = DeviceAsyncIoControl(m_device_handle, PMU_CTL_ASSIGN_EVENTS, buf.get(), (DWORD)acc_sz, NULL, 0, &res_len);
    if (!status)
        throw fatal_exception("PMU_CTL_ASSIGN_EVENTS failed");
}

void pmu_device::core_events_read_nth(uint8_t core_no)
{
    struct pmu_ctl_hdr ctl;
    DWORD res_len;

    ctl.action = PMU_CTL_READ_COUNTING;
    ctl.cores_idx.cores_count = 1;
    ctl.cores_idx.cores_no[0] = core_no;

    LPVOID out_buf = core_outs.get() + core_no;
    size_t out_buf_len = sizeof(ReadOut);
    BOOL status = DeviceAsyncIoControl(m_device_handle, PMU_CTL_READ_COUNTING, &ctl, (DWORD)sizeof(struct pmu_ctl_hdr), out_buf, (DWORD)out_buf_len, &res_len);
    if (!status)
        throw fatal_exception("PMU_CTL_READ_COUNTING failed");
}

void pmu_device::core_events_read()
{
    for (uint8_t core_no : cores_idx)
    {
        core_events_read_nth(core_no);
    }
}

void pmu_device::dsu_events_read_nth(uint8_t core_no)
{
    struct pmu_ctl_hdr ctl;
    DWORD res_len;

    ctl.action = DSU_CTL_READ_COUNTING;
    ctl.cores_idx.cores_count = 1;
    ctl.cores_idx.cores_no[0] = core_no;

    LPVOID out_buf = dsu_outs.get() + (core_no / dsu_cluster_size);
    size_t out_buf_len = sizeof(DSUReadOut);
    BOOL status = DeviceAsyncIoControl(m_device_handle, DSU_CTL_READ_COUNTING, &ctl, (DWORD)sizeof(struct pmu_ctl_hdr), out_buf, (DWORD)out_buf_len, &res_len);
    if (!status)
        throw fatal_exception("DSU_CTL_READ_COUNTING failed");
}

void pmu_device::dsu_events_read(void)
{
    for (uint8_t core_no : cores_idx)
    {
        dsu_events_read_nth(core_no);
    }
}

void pmu_device::dmc_events_read(void)
{
    struct pmu_ctl_hdr ctl;
    DWORD res_len;

    ctl.action = DMC_CTL_READ_COUNTING;
    ctl.dmc_idx = dmc_idx;

    LPVOID out_buf = dmc_idx == ALL_DMC_CHANNEL ? dmc_outs.get() : dmc_outs.get() + dmc_idx;
    size_t out_buf_len = dmc_idx == ALL_DMC_CHANNEL ? (sizeof(DMCReadOut) * dmc_regions.size()) : sizeof(DMCReadOut);
    BOOL status = DeviceAsyncIoControl(m_device_handle, DMC_CTL_READ_COUNTING, &ctl, (DWORD)sizeof(struct pmu_ctl_hdr), out_buf, (DWORD)out_buf_len, &res_len);
    if (!status)
        throw fatal_exception("DMC_CTL_READ_COUNTING failed");
}

void pmu_device::do_version_query(_Out_ version_info& driver_ver)
{
    struct pmu_ctl_ver_hdr ctl;
    DWORD res_len;

    ctl.action = PMU_CTL_QUERY_VERSION;
    ctl.version.major = MAJOR;
    ctl.version.minor = MINOR;
    ctl.version.patch = PATCH;

    BOOL status = DeviceAsyncIoControl(
        m_device_handle, PMU_CTL_QUERY_VERSION, &ctl, (DWORD)sizeof(struct pmu_ctl_ver_hdr), &driver_ver,
        (DWORD)sizeof(struct version_info), &res_len);

    if (!status)
        throw fatal_exception("PMU_CTL_QUERY_VERSION failed");
}

void pmu_device::events_query(std::map<enum evt_class, std::vector<uint16_t>>& events_out)
{
    events_out.clear();

    std::map<enum evt_class, std::vector<uint16_t>> driver_events;
    events_query_driver(driver_events);

    // Push all CORE events for the products (decided by product name)
    if (m_product_events.count(m_product_name))
    {
        for (const auto& [key, value] : m_product_events[m_product_name])
            events_out[EVT_CORE].push_back(value.index);
        std::sort(events_out[EVT_CORE].begin(), events_out[EVT_CORE].end());
    }

    // Copy not-CORE events to the output
    for (const auto& [key, value] : driver_events)
        if (key != EVT_CORE)
            events_out[key] = value;
}

void pmu_device::events_query_driver(std::map<enum evt_class, std::vector<uint16_t>>& events_out)
{
    events_out.clear();

    enum pmu_ctl_action action = PMU_CTL_QUERY_SUPP_EVENTS;
    DWORD res_len;

    // Armv8's architecture defined + vendor defined events should be within 2K at the moment.
    auto buf_size = 2048 * sizeof(uint16_t);
    std::unique_ptr<uint8_t[]> buf = std::make_unique<uint8_t[]>(buf_size);
    BOOL status = DeviceAsyncIoControl(m_device_handle, PMU_CTL_QUERY_SUPP_EVENTS, &action, (DWORD)sizeof(enum pmu_ctl_action), buf.get(), (DWORD)buf_size, &res_len);
    if (!status)
        throw fatal_exception("PMU_CTL_QUERY_SUPP_EVENTS failed");

    DWORD consumed_sz = 0;
    for (; consumed_sz < res_len;)
    {
        struct evt_hdr* hdr = reinterpret_cast<struct evt_hdr*>(buf.get() + consumed_sz);
        enum evt_class evt_class = hdr->evt_class;
        uint16_t evt_num = hdr->num;
        uint16_t* events = reinterpret_cast<uint16_t*>(hdr + 1);

        for (int idx = 0; idx < evt_num; idx++)
            events_out[evt_class].push_back(events[idx]);

        consumed_sz += sizeof(struct evt_hdr) + evt_num * sizeof(uint16_t);
    }
}

void pmu_device::print_core_stat(std::vector<struct evt_noted>& events)
{
    const enum evt_class e_class = EVT_CORE;

    bool multiplexing = multiplexings[e_class];
    bool print_note = false;

    for (const auto& a : events)
    {
        if (a.type != EVT_NORMAL)
        {
            print_note = true;
            break;
        }
    }

    struct agg_entry
    {
        uint32_t event_idx;
        uint64_t counter_value;
        uint64_t scaled_value;
    };

    uint32_t core_base = cores_idx[0];
    std::unique_ptr<agg_entry[]> overall;
    std::vector<std::wstring> timeline_event_values;

    if (all_cores_p())
    {
        // TODO: This will work only if we are counting EXACTLY same events for N cores
        overall = std::make_unique<agg_entry[]>(core_outs[core_base].evt_num);
        memset(overall.get(), 0, sizeof(agg_entry) * core_outs[core_base].evt_num);

        for (uint32_t i = 0; i < core_outs[core_base].evt_num; i++)
            overall[i].event_idx = core_outs[core_base].evts[i].event_idx;
    }

    for (uint32_t i : cores_idx)
    {
        if (!timeline_mode)
        {
            m_out.GetOutputStream() << std::endl
                << L"Performance counter stats for core " << std::dec << i
                << (multiplexing ? L", multiplexed" : L", no multiplexing")
                << (count_kernel ? L", kernel mode included" : L", kernel mode excluded")
                << L", on " << vendor_name << L" core implementation:"
                << std::endl;

            m_out.GetOutputStream() << L"note: 'e' - normal event, 'gN' - grouped event with group number N, "
                L"metric name will be appended if 'e' or 'g' comes from it"
                << std::endl
                << std::endl;
        }

        UINT32 evt_num = core_outs[i].evt_num;
        struct pmu_event_usr* evts = core_outs[i].evts;
        uint64_t round = core_outs[i].round;

        std::vector<std::wstring> col_event_name, col_event_idx,
            col_multiplexed, col_event_note;
        std::vector<uint64_t> col_counter_value, col_scaled_value;

        for (size_t j = 0; j < evt_num; j++)
        {
            if (j >= 1 && (events[j - 1].type == EVT_PADDING))
                continue;

            struct pmu_event_usr* evt = &evts[j];

            if (multiplexing)
            {
                if (timeline_mode)
                {
                    timeline_event_values.push_back(std::to_wstring(evt->value));
                    timeline_event_values.push_back(std::to_wstring(evt->scheduled));
                }
                else
                {
                    if (evt->event_idx == CYCLE_EVT_IDX) {
                        col_counter_value.push_back(evt->value);
                        col_event_name.push_back(pmu_events_get_event_name((uint16_t)evt->event_idx));
                        col_event_idx.push_back(L"fixed");
                        col_event_note.push_back(L"e");
                        col_multiplexed.push_back(std::to_wstring(evt->scheduled) + L"/" + std::to_wstring(round));
                        col_scaled_value.push_back((uint64_t)((double)evt->value / ((double)evt->scheduled / (double)round)));
                    }
                    else {
                        col_counter_value.push_back(evt->value);
                        col_event_name.push_back(pmu_events_get_event_name((uint16_t)evt->event_idx));
                        col_event_idx.push_back(IntToHexWideString(evt->event_idx, 2));
                        col_event_note.push_back(events[j - 1].note);
                        col_multiplexed.push_back(std::to_wstring(evt->scheduled) + L"/" + std::to_wstring(round));
                        col_scaled_value.push_back((uint64_t)((double)evt->value / ((double)evt->scheduled / (double)round)));
                    }
                }

                if (overall)
                {
                    overall[j].counter_value += evt->value;
                    overall[j].scaled_value += (uint64_t)((double)evt->value / ((double)evt->scheduled / (double)round));
                }
            }
            else
            {
                if (timeline_mode)
                {
                    timeline_event_values.push_back(std::to_wstring(evt->value));
                }
                else
                {
                    if (evt->event_idx == CYCLE_EVT_IDX) {
                        col_counter_value.push_back(evt->value);
                        col_event_name.push_back(pmu_events_get_event_name((uint16_t)evt->event_idx));
                        col_event_idx.push_back(L"fixed");
                        col_event_note.push_back(L"e");
                    }
                    else {
                        col_counter_value.push_back(evt->value);
                        col_event_name.push_back(pmu_events_get_event_name((uint16_t)evt->event_idx));
                        col_event_idx.push_back(IntToHexWideString(evt->event_idx, 2));
                        col_event_note.push_back(events[j - 1].note);
                    }
                }

                if (overall)
                    overall[j].counter_value += evt->value;
            }
        }

        if (!timeline_mode) {
            if (multiplexing)
            {
                TableOutput<PerformanceCounterOutputTraitsL<true>, GlobalCharType> table(m_outputType);
                table.PresetHeaders();
                table.SetAlignment(0, ColumnAlignL::RIGHT);
                table.SetAlignment(4, ColumnAlignL::RIGHT);
                table.SetAlignment(5, ColumnAlignL::RIGHT);
                table.Insert(col_counter_value, col_event_name, col_event_idx, col_event_note, col_multiplexed, col_scaled_value);
                m_out.Print(table);
                table.m_core = GlobalStringType(std::to_wstring(i)); 
                m_globalJSON.m_corePerformanceTables.push_back(table);
            }
            else
            {
                TableOutput<PerformanceCounterOutputTraitsL<false>, GlobalCharType> table(m_outputType);
                table.PresetHeaders();
                table.SetAlignment(0, ColumnAlignL::RIGHT);
                table.Insert(col_counter_value, col_event_name, col_event_idx, col_event_note);
                m_out.Print(table);
                table.m_core = GlobalStringType(std::to_wstring(i));
                m_globalJSON.m_corePerformanceTables.push_back(table);
            }

            m_globalJSON.m_multiplexing = multiplexing;
            m_globalJSON.m_kernel = count_kernel; 

        }
    }

    if (timeline_mode)
        timeline::timeline_header_event_values[e_class].push_back(timeline_event_values);

    if (!overall)
        return;

    if (timeline_mode)
        return;

    m_out.GetOutputStream() << std::endl
        << L"System-wide Overall:" << std::endl;

    std::vector<std::wstring> col_event_idx, col_event_name,
        col_event_note;
    std::vector<uint64_t> col_counter_value, col_scaled_value;

    UINT32 evt_num = core_outs[core_base].evt_num;

    for (size_t j = 0; j < evt_num; j++)
    {
        if (j >= 1 && (events[j - 1].type == EVT_PADDING))
            continue;

        struct agg_entry* entry = overall.get() + j;
        if (multiplexing)
        {
            if (entry->event_idx == CYCLE_EVT_IDX) {
                col_counter_value.push_back(entry->counter_value);
                col_event_name.push_back(pmu_events_get_event_name((uint16_t)entry->event_idx));
                col_event_idx.push_back(L"fixed");
                col_event_note.push_back(L"e");
                col_scaled_value.push_back(entry->scaled_value);
            }
            else {
                col_counter_value.push_back(entry->counter_value);
                col_event_name.push_back(pmu_events_get_event_name((uint16_t)entry->event_idx));
                col_event_idx.push_back(IntToHexWideString(entry->event_idx));
                col_event_note.push_back(events[j - 1].note);
                col_scaled_value.push_back(entry->scaled_value);
            }
        }
        else
        {
            if (entry->event_idx == CYCLE_EVT_IDX) {
                col_counter_value.push_back(entry->counter_value);
                col_event_name.push_back(pmu_events_get_event_name((uint16_t)entry->event_idx));
                col_event_idx.push_back(L"fixed");
                col_event_note.push_back(L"e");
            }
            else {
                col_counter_value.push_back(entry->counter_value);
                col_event_name.push_back(pmu_events_get_event_name((uint16_t)entry->event_idx));
                col_event_idx.push_back(IntToHexWideString(entry->event_idx));
                col_event_note.push_back(events[j - 1].note);
            }
        }
    }

    // Print System-wide Overall
    {        
        if (multiplexing)
        {
            TableOutput<SystemwidePerformanceCounterOutputTraitsL<true>, GlobalCharType> table(m_outputType);
            table.PresetHeaders();
            table.SetAlignment(0, ColumnAlignL::RIGHT);
            table.SetAlignment(4, ColumnAlignL::RIGHT);
            table.Insert(col_counter_value, col_event_name, col_event_idx, col_event_note, col_scaled_value);
            m_out.Print(table);
            m_globalJSON.m_coreOverall = table;
        }
        else {
            TableOutput<SystemwidePerformanceCounterOutputTraitsL<false>, GlobalCharType> table(m_outputType);
            table.PresetHeaders();
            table.SetAlignment(0, ColumnAlignL::RIGHT);
            table.Insert(col_counter_value, col_event_name, col_event_idx, col_event_note);
            m_out.Print(table);
            m_globalJSON.m_coreOverall = table;
        }
    }
}

void pmu_device::print_core_metrics(std::vector<struct evt_noted>& events)
{
    const enum evt_class e_class = EVT_CORE;
    std::vector<std::wstring> col_core, col_product_name, col_metric_name, col_metric_value, metric_unit;

    for (uint32_t i : cores_idx)
    {
        const uint32_t evt_num = core_outs[i].evt_num;
        struct pmu_event_usr* evts = core_outs[i].evts;

        std::map<std::wstring, std::set<int>> event_metrics;        // [metric_name] -> set of groups

        for (const struct evt_noted& event : events)
            if (event.metric.size())
                event_metrics[event.metric].insert(event.group);

        // Seach if we have metric we can calculate with the formula
        for (const auto& [metric, groups] : event_metrics)
        {
            for (const auto group : groups)
            {
                if (m_product_name.empty())
                    break;

                if (m_product_metrics.count(m_product_name) == 0)
                    break;

                if (m_product_metrics[m_product_name].count(metric) == 0)
                    break;

                const auto& product_metric = m_product_metrics[m_product_name][metric];

                std::map<std::wstring, double> vars;
                const std::wstring& formula_sy = product_metric.metric_formula_sy;

                for (auto it = events.begin(); it != events.end(); it++)
                {
                    const auto& event = *it;
                    const auto index = it - events.begin() + 1;
                    assert(index < evt_num);
                    struct pmu_event_usr* evt = &evts[index];

                    if (event.metric == metric && event.group == group)
                    {
                        std::wstring event_name = pmu_events_get_event_name((uint16_t)evt->event_idx);
                        vars[event_name] = static_cast<double>(evt->value);
                    }
                }

                double metric_value = metric_calculate_shunting_yard_expression(vars, formula_sy);

                col_core.push_back(std::to_wstring(i));
                col_product_name.push_back(m_product_name);
                col_metric_name.push_back(product_metric.name);
                col_metric_value.push_back(DoubleToWideString(metric_value, 3));
                metric_unit.push_back(product_metric.metric_unit);
            }
        }
    }

    if (timeline_mode && col_metric_name.size())    // Only add metrics to timeline when metric were calculated
    {
        timeline::timeline_header_metric_values[e_class].push_back(col_metric_value);
        if (timeline::timeline_header_metric_names[e_class].empty())
        {
            // Calulate how many metrics were speciffied per core
            auto metrics_per_code = col_metric_name.size() / cores_idx.size();

            // For every core we must print each metric
            for (auto c = 0; c < cores_idx.size(); c++)
                for (auto m = 0; m < metrics_per_code; m++)
                {
                    auto i = cores_idx[c];
                    timeline::timeline_header_cores[e_class].push_back(L"core " + std::to_wstring(i));
                }

            // Insert metrics names and values to a separate strcuture (we will concatenate it later in print)
            timeline::timeline_header_metric_names[e_class].insert(timeline::timeline_header_metric_names[e_class].end(),
                col_metric_name.begin(), col_metric_name.end());
        }
    }

    if (!timeline_mode && col_core.size())
    {
        m_out.GetOutputStream() << std::endl;
        m_out.GetOutputStream() << L"Telemetry Solution Metrics:" << std::endl;

        TableOutput<TelemetrySolutionMetricOutputTraitsL, GlobalCharType> table(m_outputType);
        table.PresetHeaders();
        table.SetAlignment(0, ColumnAlignL::RIGHT);
        table.SetAlignment(1, ColumnAlignL::LEFT);
        table.SetAlignment(2, ColumnAlignL::LEFT);
        table.SetAlignment(3, ColumnAlignL::RIGHT);
        table.SetAlignment(4, ColumnAlignL::LEFT);
        table.Insert(col_core, col_product_name, col_metric_name, col_metric_value, metric_unit);
        m_globalJSON.m_TSmetric = table;
        m_out.Print(table);
    }
}

void pmu_device::print_dsu_stat(std::vector<struct evt_noted>& events, bool report_l3_metric)
{
    const enum evt_class e_class = EVT_DSU;

    bool multiplexing = multiplexings[e_class];
    bool print_note = false;

    for (auto& a : events)
    {
        if (a.type != EVT_NORMAL)
        {
            print_note = true;
            break;
        }
    }

    struct agg_entry
    {
        uint32_t event_idx;
        uint64_t counter_value;
        uint64_t scaled_value;
    };

    std::unique_ptr<agg_entry[]> overall;
    std::vector<std::wstring > event_values;

    if (all_cores_p())
    {
        // TODO: because now all DSU cores count same events we use first core
        //       to gather events info.
        uint32_t dsu_core_0 = *(dsu_cores.begin());

        overall = std::make_unique<agg_entry[]>(dsu_outs[dsu_core_0].evt_num);
        memset(overall.get(), 0, sizeof(agg_entry) * dsu_outs[dsu_core_0].evt_num);

        for (uint32_t i = 0; i < dsu_outs[dsu_core_0].evt_num; i++)
            overall[i].event_idx = dsu_outs[dsu_core_0].evts[i].event_idx;
    }

    for (uint32_t dsu_core : dsu_cores)
    {
        if (!timeline_mode)
        {
            m_out.GetOutputStream() << std::endl
                << L"Performance counter stats for DSU cluster "
                << dsu_core
                << (multiplexing ? L", multiplexed" : L", no multiplexing")
                << L", on " << vendor_name
                << L" core implementation:"
                << std::endl;

            m_out.GetOutputStream() << L"note: 'e' - normal event, 'gN' - grouped event with group number N, "
                L"metric name will be appended if 'e' or 'g' comes from it"
                << std::endl
                << std::endl;
        }

        UINT32 evt_num = dsu_outs[dsu_core].evt_num;
        struct pmu_event_usr* evts = dsu_outs[dsu_core].evts;
        uint64_t round = dsu_outs[dsu_core].round;

        std::vector<std::wstring> col_event_idx, col_event_name,
            col_multiplexed, col_event_note;
        std::vector<uint64_t> col_counter_value, col_scaled_value;

        for (size_t j = 0; j < evt_num; j++)
        {
            if (j >= 1 && (events[j - 1].type == EVT_PADDING))
                continue;

            struct pmu_event_usr* evt = &evts[j];

            if (multiplexing)
            {
                if (timeline_mode)
                {
                    event_values.push_back(std::to_wstring(evt->value));
                    event_values.push_back(std::to_wstring(evt->scheduled));
                }
                else
                {
                    if (evt->event_idx == CYCLE_EVT_IDX) {

                        col_counter_value.push_back(evt->value);
                        col_event_name.push_back(pmu_events_get_event_name((uint16_t)evt->event_idx));
                        col_event_idx.push_back(L"fixed");
                        col_event_note.push_back(L"e");
                        col_multiplexed.push_back(std::to_wstring(evt->scheduled) + L"/" + std::to_wstring(round));
                        col_scaled_value.push_back((uint64_t)((double)evt->value / ((double)evt->scheduled / (double)round)));
                    }
                    else {
                        col_counter_value.push_back(evt->value);
                        col_event_name.push_back(pmu_events_get_event_name((uint16_t)evt->event_idx));
                        col_event_idx.push_back(IntToHexWideString(evt->event_idx));
                        col_event_note.push_back(events[j - 1].note);
                        col_multiplexed.push_back(std::to_wstring(evt->scheduled) + L"/" + std::to_wstring(round));
                        col_scaled_value.push_back((uint64_t)((double)evt->value / ((double)evt->scheduled / (double)round)));
                    }
                }

                if (overall)
                {
                    overall[j].counter_value += evt->value;
                    overall[j].scaled_value += (uint64_t)((double)evt->value / ((double)evt->scheduled / (double)round));
                }
            }
            else
            {
                if (timeline_mode)
                {
                    event_values.push_back(std::to_wstring(evt->value));
                }
                else
                {
                    if (evt->event_idx == CYCLE_EVT_IDX) {

                        col_counter_value.push_back(evt->value);// IntToDecWithCommas(evt->value));
                        col_event_name.push_back(pmu_events_get_event_name((uint16_t)evt->event_idx));
                        col_event_idx.push_back(L"fixed");
                        col_event_note.push_back(L"e");
                    }
                    else {
                        col_counter_value.push_back(evt->value);// IntToDecWithCommas(evt->value));
                        col_event_name.push_back(pmu_events_get_event_name((uint16_t)evt->event_idx));
                        col_event_idx.push_back(IntToHexWideString(evt->event_idx));
                        col_event_note.push_back(events[j - 1].note);
                    }
                }

                if (overall)
                    overall[j].counter_value += evt->value;
            }
        }

        // Print performance counter stats for DSU cluster
        {
            if (multiplexing)
            {
                TableOutput<PerformanceCounterOutputTraitsL<true>, GlobalCharType> table(m_outputType);
                table.PresetHeaders();
                table.SetAlignment(0, ColumnAlignL::RIGHT);
                table.SetAlignment(4, ColumnAlignL::RIGHT);
                table.SetAlignment(5, ColumnAlignL::RIGHT);
                table.Insert(col_counter_value, col_event_name, col_event_idx, col_event_note, col_multiplexed, col_scaled_value);
                table.m_core = GlobalStringType(std::to_wstring(dsu_core));
                m_globalJSON.m_DSUPerformanceTables.push_back(table);

                m_out.Print(table);
            }
            else
            {
                TableOutput<PerformanceCounterOutputTraitsL<false>, GlobalCharType> table(m_outputType);
                table.PresetHeaders();
                table.SetAlignment(0, ColumnAlignL::RIGHT);
                table.Insert(col_counter_value, col_event_name, col_event_idx, col_event_note);
                table.m_core = GlobalStringType(std::to_wstring(dsu_core));
                m_globalJSON.m_DSUPerformanceTables.push_back(table);

                m_out.Print(table);
            }
        }
    }

    if (timeline_mode)
    {
        timeline::timeline_header_event_values[e_class].push_back(event_values);
    }

    if (!overall)
    {
        if (!timeline_mode && report_l3_metric)
        {
            m_out.GetOutputStream() << std::endl
                << L"L3 cache metrics:" << std::endl;

            std::vector<std::wstring> col_cluster, col_cores, col_read_bandwith, col_miss_rate;

            for (uint32_t dsu_core : dsu_cores)
            {
                UINT32 evt_num = dsu_outs[dsu_core].evt_num;
                struct pmu_event_usr* evts = dsu_outs[dsu_core].evts;
                uint64_t l3_cache_access_num = 0, l3_cache_refill_num = 0;

                for (size_t j = FIXED_COUNTERS_NO; j < evt_num; j++)
                {
                    if (j >= 1 && (events[j - 1].type == EVT_PADDING))
                        continue;

                    if (events[j - 1].index == PMU_EVENT_L3D_CACHE)
                    {
                        l3_cache_access_num = evts[j].value;
                    }
                    else if (events[j - 1].index == PMU_EVENT_L3D_CACHE_REFILL)
                    {
                        l3_cache_refill_num = evts[j].value;
                    }
                }

                double miss_rate_pct = (l3_cache_access_num != 0)
                    ? ((double)(l3_cache_refill_num)) / ((double)(l3_cache_access_num)) * 100
                    : 100.0;

                std::wstring core_list;
                for (uint8_t core_idx : cores_idx)
                    if ((core_idx / dsu_cluster_size) == (uint8_t)dsu_core)
                        if (core_list.empty())
                            core_list = L"" + std::to_wstring(core_idx);
                        else
                            core_list += L", " + std::to_wstring(core_idx);

                col_cluster.push_back(std::to_wstring(dsu_core));
                col_cores.push_back(core_list);
                col_read_bandwith.push_back(DoubleToWideString(((double)(l3_cache_access_num * 64)) / 1024.0 / 1024.0) + L"MB");
                col_miss_rate.push_back(DoubleToWideString(miss_rate_pct) + L"%");
            }

            {
                TableOutput<L3CacheMetricOutputTraitsL, GlobalCharType> table(m_outputType);
                table.PresetHeaders();
                table.SetAlignment(0, ColumnAlignL::RIGHT);
                table.SetAlignment(1, ColumnAlignL::RIGHT);
                table.SetAlignment(2, ColumnAlignL::RIGHT);
                table.SetAlignment(3, ColumnAlignL::RIGHT);
                table.Insert(col_cluster, col_cores, col_read_bandwith, col_miss_rate);
                m_globalJSON.m_DSUL3metric = table;
                m_out.Print(table);
            }
        }

        return;
    }

    if (timeline_mode)
        return;

    m_out.GetOutputStream() << std::endl;

    m_out.GetOutputStream() << L"System-wide Overall:" << std::endl;

    uint32_t dsu_core_0 = *(dsu_cores.begin());
    UINT32 evt_num = dsu_outs[dsu_core_0].evt_num;

    std::vector<std::wstring> col_event_name, col_event_idx,
        col_event_note;
    std::vector<uint64_t> col_counter_value, col_scaled_value;
    for (size_t j = 0; j < evt_num; j++)
    {
        if (j >= 1 && (events[j - 1].type == EVT_PADDING))
            continue;

        struct agg_entry* entry = overall.get() + j;
        if (multiplexing)
        {
            if (entry->event_idx == CYCLE_EVT_IDX) {
                col_counter_value.push_back(entry->counter_value);
                col_event_name.push_back(pmu_events_get_event_name((uint16_t)entry->event_idx));
                col_event_idx.push_back(L"fixed");
                col_event_note.push_back(L"e");
                col_scaled_value.push_back(entry->scaled_value);
            }
            else {
                col_counter_value.push_back(entry->counter_value);
                col_event_name.push_back(pmu_events_get_event_name((uint16_t)entry->event_idx));
                col_event_idx.push_back(IntToHexWideString(entry->event_idx));
                col_event_note.push_back(events[j - 1].note);
                col_scaled_value.push_back(entry->scaled_value);
            }
        }
        else
        {
            if (entry->event_idx == CYCLE_EVT_IDX) {
                col_counter_value.push_back(entry->counter_value);
                col_event_name.push_back(pmu_events_get_event_name((uint16_t)entry->event_idx));
                col_event_idx.push_back(L"fixed");
                col_event_note.push_back(L"e");
            }
            else {
                col_counter_value.push_back(entry->counter_value);
                col_event_name.push_back(pmu_events_get_event_name((uint16_t)entry->event_idx));
                col_event_idx.push_back(IntToHexWideString(entry->event_idx));
                col_event_note.push_back(events[j - 1].note);
            }
        }
    }

    {        
        if (multiplexing)
        {
            TableOutput<SystemwidePerformanceCounterOutputTraitsL<true>, GlobalCharType> table(m_outputType);
            table.PresetHeaders();
            table.SetAlignment(0, ColumnAlignL::RIGHT);
            table.SetAlignment(4, ColumnAlignL::RIGHT);
            table.Insert(col_counter_value, col_event_name, col_event_idx, col_event_note, col_scaled_value);
            m_globalJSON.m_DSUOverall = table;
            m_out.Print(table);
        }
        else {
            TableOutput<SystemwidePerformanceCounterOutputTraitsL<false>, GlobalCharType> table(m_outputType);
            table.PresetHeaders();
            table.SetAlignment(0, ColumnAlignL::RIGHT);
            table.Insert(col_counter_value, col_event_name, col_event_idx, col_event_note);
            m_globalJSON.m_DSUOverall = table;
            m_out.Print(table);
        }
    }

    if (report_l3_metric)
    {
        m_out.GetOutputStream() << std::endl
            << L"L3 cache metrics:" << std::endl;

        std::vector<std::wstring> col_cluster, col_cores, col_read_bandwith, col_miss_rate;

        for (uint32_t dsu_core : dsu_cores)
        {
            UINT32 evt_num2 = dsu_outs[dsu_core].evt_num;
            struct pmu_event_usr* evts = dsu_outs[dsu_core].evts;
            uint64_t l3_cache_access_num = 0, l3_cache_refill_num = 0;

            for (size_t j = FIXED_COUNTERS_NO; j < evt_num2; j++)
            {
                if (j >= 1 && (events[j - 1].type == EVT_PADDING))
                    continue;

                if (events[j - 1].index == PMU_EVENT_L3D_CACHE)
                {
                    l3_cache_access_num = evts[j].value;
                }
                else if (events[j - 1].index == PMU_EVENT_L3D_CACHE_REFILL)
                {
                    l3_cache_refill_num = evts[j].value;
                }
            }

            double miss_rate_pct = (l3_cache_access_num != 0)
                ? ((double)(l3_cache_refill_num)) / ((double)(l3_cache_access_num)) * 100
                : 100.0;

            std::wstring core_list;
            for (uint8_t core_idx : cores_idx)
                if ((core_idx / dsu_cluster_size) == (uint8_t)dsu_core)
                    if (core_list.empty())
                        core_list = L"" + std::to_wstring(core_idx);
                    else
                        core_list += L", " + std::to_wstring(core_idx);

            col_cluster.push_back(std::to_wstring(dsu_core));
            col_cores.push_back(core_list);
            col_read_bandwith.push_back(DoubleToWideString(((double)(l3_cache_access_num * 64)) / 1024.0 / 1024.0) + L"MB");
            col_miss_rate.push_back(DoubleToWideString(miss_rate_pct) + L"%");
        }

        uint32_t dsu_core = *dsu_cores.begin();
        UINT32 evt_num2 = dsu_outs[dsu_core].evt_num;
        uint64_t acc_l3_cache_access_num = 0, acc_l3_cache_refill_num = 0;

        for (size_t j = 0; j < evt_num2; j++)
        {
            if (j >= 1 && (events[j - 1].type == EVT_PADDING))
                continue;

            struct agg_entry* entry = overall.get() + j;
            if (entry->event_idx == PMU_EVENT_L3D_CACHE)
            {
                acc_l3_cache_access_num = entry->counter_value;
            }
            else if (entry->event_idx == PMU_EVENT_L3D_CACHE_REFILL)
            {
                acc_l3_cache_refill_num = entry->counter_value;
            }
        }

        double acc_miss_rate_pct = (acc_l3_cache_access_num != 0)
            ? ((double)(acc_l3_cache_refill_num)) / ((double)(acc_l3_cache_access_num)) * 100
            : 100.0;

        col_cluster.push_back(L"all");
        col_cores.push_back(L"all");
        col_read_bandwith.push_back(DoubleToWideString(((double)(acc_l3_cache_access_num * 64)) / 1024.0 / 1024.0) + L"MB");
        col_miss_rate.push_back(DoubleToWideString(acc_miss_rate_pct) + L"%");

        {
            TableOutput<L3CacheMetricOutputTraitsL, GlobalCharType> table(m_outputType);
            table.PresetHeaders();
            table.SetAlignment(0, ColumnAlignL::RIGHT);
            table.SetAlignment(1, ColumnAlignL::RIGHT);
            table.SetAlignment(2, ColumnAlignL::RIGHT);
            table.SetAlignment(3, ColumnAlignL::RIGHT);
            table.Insert(col_cluster, col_cores, col_read_bandwith, col_miss_rate);
            m_globalJSON.m_DSUL3metric = table;
            m_out.Print(table);
        }
    }
}

void pmu_device::print_dmc_stat(std::vector<struct evt_noted>& clk_events, std::vector<struct evt_noted>& clkdiv2_events, bool report_ddr_bw_metric)
{
    struct agg_entry
    {
        uint32_t event_idx;
        uint64_t counter_value;
    };

    std::unique_ptr<agg_entry[]> overall_clk, overall_clkdiv2;
    size_t clkdiv2_events_num = clkdiv2_events.size();
    size_t clk_events_num = clk_events.size();
    uint8_t ch_base = 0, ch_end = 0;
    std::vector<std::wstring> event_values_clk, event_values_clkdiv2;

    if (dmc_idx == ALL_DMC_CHANNEL)
    {
        ch_base = 0;
        ch_end = (uint8_t)dmc_regions.size();
    }
    else
    {
        ch_base = dmc_idx;
        ch_end = dmc_idx + 1;
    }

    if (clk_events_num)
    {
        overall_clk = std::make_unique<agg_entry[]>(clk_events_num);
        memset(overall_clk.get(), 0, sizeof(agg_entry) * clk_events_num);

        for (uint32_t i = 0; i < clk_events_num; i++)
            overall_clk[i].event_idx = dmc_outs[ch_base].clk_events[i].event_idx;
    }

    if (clkdiv2_events_num)
    {
        overall_clkdiv2 = std::make_unique<agg_entry[]>(clkdiv2_events_num);
        memset(overall_clkdiv2.get(), 0, sizeof(agg_entry) * clkdiv2_events_num);

        for (uint32_t i = 0; i < clkdiv2_events_num; i++)
            overall_clkdiv2[i].event_idx = dmc_outs[ch_base].clkdiv2_events[i].event_idx;
    }

    std::vector<std::wstring> col_pmu_id, col_event_name,
        col_event_idx, col_event_note;
    std::vector<uint64_t> col_counter_value;

    for (uint32_t i = ch_base; i < ch_end; i++)
    {
        int32_t evt_num = dmc_outs[i].clk_events_num;

        struct pmu_event_usr* evts = dmc_outs[i].clk_events;
        for (int j = 0; j < evt_num; j++)
        {
            struct pmu_event_usr* evt = &evts[j];

            if (timeline_mode)
            {
                event_values_clk.push_back(std::to_wstring(evt->value));
            }
            else
            {
                col_pmu_id.push_back(L"dmc " + std::to_wstring(i));
                col_counter_value.push_back(evt->value);
                col_event_name.push_back(pmu_events_get_event_name((uint16_t)evt->event_idx, EVT_DMC_CLK));
                col_event_idx.push_back(IntToHexWideString(evt->event_idx));
                col_event_note.push_back(clk_events[j].note);
            }

            if (overall_clk)
                overall_clk[j].counter_value += evt->value;
        }

        evt_num = dmc_outs[i].clkdiv2_events_num;
        evts = dmc_outs[i].clkdiv2_events;
        for (int j = 0; j < evt_num; j++)
        {
            struct pmu_event_usr* evt = &evts[j];

            if (timeline_mode)
            {
                event_values_clkdiv2.push_back(std::to_wstring(evt->value));
            }
            else
            {
                col_pmu_id.push_back(L"dmc " + std::to_wstring(i));
                col_counter_value.push_back(evt->value);
                col_event_name.push_back(pmu_events_get_event_name((uint16_t)evt->event_idx, EVT_DMC_CLKDIV2));
                col_event_idx.push_back(IntToHexWideString(evt->event_idx));
                col_event_note.push_back(clkdiv2_events[j].note);
            }

            if (overall_clkdiv2)
                overall_clkdiv2[j].counter_value += evt->value;
        }
    }

    for (size_t j = 0; j < clk_events_num; j++)
    {
        struct agg_entry* entry = overall_clk.get() + j;
        col_pmu_id.push_back(L"overall");
        col_counter_value.push_back(entry->counter_value);
        col_event_name.push_back(pmu_events_get_event_name((uint16_t)entry->event_idx, EVT_DMC_CLK));
        col_event_idx.push_back(IntToHexWideString(entry->event_idx));
        col_event_note.push_back(clk_events[j].note);
    }

    for (size_t j = 0; j < clkdiv2_events_num; j++)
    {
        struct agg_entry* entry = overall_clkdiv2.get() + j;
        col_pmu_id.push_back(L"overall");
        col_counter_value.push_back(entry->counter_value);
        col_event_name.push_back(pmu_events_get_event_name((uint16_t)entry->event_idx, EVT_DMC_CLKDIV2));
        col_event_idx.push_back(IntToHexWideString(entry->event_idx));
        col_event_note.push_back(clkdiv2_events[j].note);
    }

    if (!timeline_mode)
    {
        m_out.GetOutputStream() << std::endl;

        TableOutput<PMUPerformanceCounterOutputTraitsL, GlobalCharType> table(m_outputType);
        table.PresetHeaders();
        table.SetAlignment(1, ColumnAlignL::RIGHT);
        table.Insert(col_pmu_id, col_counter_value, col_event_name, col_event_idx, col_event_note);

        m_globalJSON.m_pmu = table;
        m_out.Print(table);
    }
    else
    {
        timeline::timeline_header_event_values[EVT_DMC_CLK].push_back(event_values_clk);
        timeline::timeline_header_event_values[EVT_DMC_CLKDIV2].push_back(event_values_clkdiv2);
    }

    if (!overall_clk && !overall_clkdiv2)
    {
        if (!timeline_mode && report_ddr_bw_metric)
        {
            std::vector<std::wstring> col_channel, col_rw_bandwidth;

            m_out.GetOutputStream() << std::endl
                << L"ddr metrics:" << std::endl;

            for (uint32_t i = ch_base; i < ch_end; i++)
            {
                int32_t evt_num = dmc_outs[i].clkdiv2_events_num;
                struct pmu_event_usr* evts = dmc_outs[i].clkdiv2_events;
                uint64_t ddr_rd_num = 0;

                for (int j = 0; j < evt_num; j++)
                {
                    if (evts[j].event_idx == DMC_EVENT_RDWR)
                        ddr_rd_num = evts[j].value;
                }

                col_channel.push_back(std::to_wstring(i));
                col_rw_bandwidth.push_back(DoubleToWideString(((double)(ddr_rd_num * 128)) / 1000.0 / 1000.0) + L"MB");
            }

            TableOutput<DDRMetricOutputTraitsL, GlobalCharType> table(m_outputType);
            table.PresetHeaders();
            table.SetAlignment(0, ColumnAlignL::RIGHT);
            table.SetAlignment(1, ColumnAlignL::RIGHT);
            table.Insert(col_channel, col_rw_bandwidth);

            m_globalJSON.m_DMCDDDR = table;
            m_out.Print(table);

        }
        return;
    }

    if (timeline_mode)
        return;

    if (report_ddr_bw_metric)
    {
        m_out.GetOutputStream() << std::endl
            << L"ddr metrics:" << std::endl;

        std::vector<std::wstring> col_channel, col_rw_bandwidth;

        for (uint32_t i = ch_base; i < ch_end; i++)
        {
            int32_t evt_num = dmc_outs[i].clkdiv2_events_num;
            struct pmu_event_usr* evts = dmc_outs[i].clkdiv2_events;
            uint64_t ddr_rd_num = 0;

            for (int j = 0; j < evt_num; j++)
            {
                if (evts[j].event_idx == DMC_EVENT_RDWR)
                    ddr_rd_num = evts[j].value;
            }

            col_channel.push_back(std::to_wstring(i));
            col_rw_bandwidth.push_back(DoubleToWideString(((double)(ddr_rd_num * 128)) / 1000.0 / 1000.0) + L"MB");
        }

        uint64_t evt_num = dmc_outs[ch_base].clkdiv2_events_num;
        uint64_t ddr_rd_num = 0;

        for (int j = 0; j < evt_num; j++)
        {
            struct agg_entry* entry = overall_clkdiv2.get() + j;
            if (entry->event_idx == DMC_EVENT_RDWR)
                ddr_rd_num = entry->counter_value;
        }

        col_channel.push_back(L"all");
        col_rw_bandwidth.push_back(DoubleToWideString(((double)(ddr_rd_num * 128)) / 1000.0 / 1000.0) + L"MB");

        TableOutput<DDRMetricOutputTraitsL, GlobalCharType> table(m_outputType);
        table.PresetHeaders();
        table.SetAlignment(0, ColumnAlignL::RIGHT);
        table.SetAlignment(1, ColumnAlignL::RIGHT);
        table.Insert(col_channel, col_rw_bandwidth);

        m_globalJSON.m_DMCDDDR = table;
        m_out.Print(table);

    }
}

void pmu_device::do_list_prep_events(_Out_ std::vector<std::wstring>& col_alias_name,
    _Out_ std::vector<std::wstring>& col_raw_index,
    _Out_ std::vector<std::wstring>& col_event_type,
    _Out_ std::vector<std::wstring>& col_desc)
{
    std::map<enum evt_class, std::vector<uint16_t>> events;
    events_query(events);

    // Function will "refill" parameters
    col_alias_name.clear();
    col_raw_index.clear();
    col_event_type.clear();
    col_desc.clear();

    for (const auto& a : events)
    {
        const wchar_t* prefix = pmu_events_get_evt_name_prefix(a.first);
        for (auto b : a.second) {
            col_alias_name.push_back(std::wstring(prefix) + std::wstring(pmu_events_get_event_name(b, a.first)));
            col_raw_index.push_back(IntToHexWideString(b, 4));
            col_event_type.push_back(L"[" + std::wstring(pmu_events_get_evt_class_name(a.first)) + std::wstring(L" PMU event") + L"]");
            col_desc.push_back(std::wstring(pmu_events_get_evt_desc(b, a.first)));
        }
    }

    // Add extra-events specified with -E <filename> or -E "e:v,e:v,e:v"
    for (const auto& a : pmu_events::extra_events)
    {
        const wchar_t* prefix = pmu_events_get_evt_name_prefix(a.first);
        for (const struct extra_event& b : a.second) {
            col_alias_name.push_back(std::wstring(prefix) + std::wstring(pmu_events_get_event_name(b.hdr.num, a.first)));
            col_raw_index.push_back(IntToHexWideString(b.hdr.num, 4));
            col_event_type.push_back(L"[" + std::wstring(pmu_events_get_evt_class_name(a.first)) + std::wstring(L" PMU event*") + L"]");
            col_desc.push_back(L"<extra event>");
        }
    }
}

void pmu_device::do_list_prep_metrics(_Out_ std::vector<std::wstring>& col_metric,
    _Out_ std::vector<std::wstring>& col_events,
    _Out_ std::vector<std::wstring>& col_formula,
    _Out_ std::vector<std::wstring>& col_unit,
    _Out_ std::vector<std::wstring>& col_desc,
    _In_ const std::map<std::wstring, metric_desc>& metrics)
{
    col_metric.clear();
    col_events.clear();
    col_formula.clear();
    col_unit.clear();
    col_desc.clear();

    for (const auto& [key, value] : metrics) {
        col_metric.push_back(key);
        col_events.push_back(value.raw_str);

        if (m_product_metrics.count(m_product_name)
            && m_product_metrics[m_product_name].count(key))
        {
            struct product_metric& metric = m_product_metrics[m_product_name][key];
            col_formula.push_back(metric.metric_formula);
            col_unit.push_back(metric.metric_unit);
            col_desc.push_back(metric.title);
        }
        else
        {
            col_formula.push_back(L"---");
            col_unit.push_back(L"---");
            col_desc.push_back(L"---");
        }
    }
}

void pmu_device::do_list(const std::map<std::wstring, metric_desc>& metrics)
{
    // Print pre-defined events:
    {
        m_out.GetOutputStream() << std::endl
            << L"List of pre-defined events (to be used in -e)"
            << std::endl << std::endl;

        std::vector<std::wstring> col_alias_name, col_raw_index, col_event_type, col_event_desc;
        do_list_prep_events(col_alias_name, col_raw_index, col_event_type, col_event_desc);

        if (do_verbose)
        {
            TableOutput<PredefinedEventsOutputTraitsL<true>, GlobalCharType> table(m_outputType);
            table.PresetHeaders();
            table.SetAlignment(1, ColumnAlignL::RIGHT);
            table.Insert(col_alias_name, col_raw_index, col_event_type, col_event_desc);

            m_globalListJSON.isVerbose = do_verbose;
            m_globalListJSON.m_Events = table;
            m_out.Print(table);
        }
        else
        {
            TableOutput<PredefinedEventsOutputTraitsL<false>, GlobalCharType> table(m_outputType);
            table.PresetHeaders();
            table.SetAlignment(1, ColumnAlignL::RIGHT);
            table.Insert(col_alias_name, col_raw_index, col_event_type);

            m_globalListJSON.isVerbose = do_verbose;
            m_globalListJSON.m_Events = table;
            m_out.Print(table);
        }
    }

    // Print supported metrics
    m_out.GetOutputStream() << std::endl
        << L"List of supported metrics (to be used in -m)"
        << std::endl << std::endl;

    std::vector<std::wstring> col_metric, col_events, col_formula, col_unit, col_desc;
    do_list_prep_metrics(col_metric, col_events, col_formula, col_unit, col_desc, metrics);

    if (do_verbose)
    {
        TableOutput<MetricOutputTraitsL<true>, GlobalCharType> table(m_outputType);
        table.PresetHeaders();
        table.Insert(col_metric, col_events, col_formula, col_unit, col_desc);

        m_globalListJSON.isVerbose = do_verbose;
        m_globalListJSON.m_Metrics = table;
        m_out.Print(table);
    }
    else
    {
        TableOutput<MetricOutputTraitsL<false>, GlobalCharType> table(m_outputType);
        table.PresetHeaders();
        table.Insert(col_metric, col_events);

        m_globalListJSON.isVerbose = do_verbose;
        m_globalListJSON.m_Metrics = table;
        m_out.Print(table);
    }

    m_out.Print(m_globalListJSON);
}

void pmu_device::get_event_scheduling_test_data(_In_ std::map<enum evt_class, std::vector<struct evt_noted>>& ioctl_events,
    _Out_ std::wstring& evt_indexes, _Out_ std::wstring& evt_notes, enum evt_class e_class)
{
    evt_indexes.clear();
    evt_notes.clear();

    for (const auto& e : ioctl_events[e_class])
    {
        evt_indexes += std::to_wstring(e.index) + L",";
        evt_notes += L"(" + e.note + L")" + L",";
    }
    if (!evt_indexes.empty() && evt_indexes.back() == L',')
        evt_indexes.pop_back();
    if (!evt_notes.empty() && evt_notes.back() == L',')
        evt_notes.pop_back();
}

void pmu_device::do_test_prep_tests(_Out_ std::vector<std::wstring>& col_test_name, _Out_  std::vector<std::wstring>& col_test_result,
    _In_ uint32_t enable_bits, _In_ std::map<enum evt_class, std::vector<struct evt_noted>>& ioctl_events)
{
    col_test_name.clear();
    col_test_result.clear();

    // Tests for request.ioctl_events
    col_test_name.push_back(L"request.ioctl_events [EVT_CORE]");
    col_test_result.push_back(enable_bits & CTL_FLAG_CORE ? L"True" : L"False");
    col_test_name.push_back(L"request.ioctl_events [EVT_DSU]");
    col_test_result.push_back(enable_bits & CTL_FLAG_DSU ? L"True" : L"False");
    col_test_name.push_back(L"request.ioctl_events [EVT_DMC_CLK/EVT_DMC_CLKDIV2]");
    col_test_result.push_back(enable_bits & CTL_FLAG_DMC ? L"True" : L"False");

    // Test for pmu_device.vendor_name
    col_test_name.push_back(L"pmu_device.vendor_name");
    col_test_result.push_back(vendor_name);

    // Test for product name (if available)
    col_test_name.push_back(L"pmu_device.product_name");
    col_test_result.push_back(m_product_name);

    col_test_name.push_back(L"pmu_device.product_name(extended)");
    col_test_result.push_back(get_product_name_ext());

    col_test_name.push_back(L"pmu_device.product []");
    col_test_result.push_back(get_all_product_name_str());

    col_test_name.push_back(L"pmu_device.m_product_alias");
    col_test_result.push_back(get_all_aliases_str());

    // Tests for pmu_device.events_query(events)
    std::map<enum evt_class, std::vector<uint16_t>> events;
    events_query(events);
    col_test_name.push_back(L"pmu_device.events_query(events) [EVT_CORE]");
    if (events.count(EVT_CORE))
        col_test_result.push_back(std::to_wstring(events[EVT_CORE].size()));
    else
        col_test_result.push_back(L"0");
    col_test_name.push_back(L"pmu_device.events_query(events) [EVT_DSU]");
    if (events.count(EVT_DSU))
        col_test_result.push_back(std::to_wstring(events[EVT_DSU].size()));
    else
        col_test_result.push_back(L"0");
    col_test_name.push_back(L"pmu_device.events_query(events) [EVT_DMC_CLK]");
    if (events.count(EVT_DMC_CLK))
        col_test_result.push_back(std::to_wstring(events[EVT_DMC_CLK].size()));
    else
        col_test_result.push_back(L"0");
    col_test_name.push_back(L"pmu_device.events_query(events) [EVT_DMC_CLKDIV2]");
    if (events.count(EVT_DMC_CLKDIV2))
        col_test_result.push_back(std::to_wstring(events[EVT_DMC_CLKDIV2].size()));
    else
        col_test_result.push_back(L"0");

    // Tests for PMU_CTL_QUERY_HW_CFG
    struct hw_cfg hw_cfg;
    query_hw_cfg(hw_cfg);
    col_test_name.push_back(L"PMU_CTL_QUERY_HW_CFG [arch_id]");
    col_test_result.push_back(IntToHexWideString(hw_cfg.arch_id));
    col_test_name.push_back(L"PMU_CTL_QUERY_HW_CFG [core_num]");
    col_test_result.push_back(IntToHexWideString(hw_cfg.core_num));
    col_test_name.push_back(L"PMU_CTL_QUERY_HW_CFG [fpc_num]");
    col_test_result.push_back(IntToHexWideString(hw_cfg.fpc_num));
    col_test_name.push_back(L"PMU_CTL_QUERY_HW_CFG [gpc_num]");
    col_test_result.push_back(IntToHexWideString(hw_cfg.gpc_num));
    col_test_name.push_back(L"PMU_CTL_QUERY_HW_CFG [total_gpc_num]");
    col_test_result.push_back(IntToHexWideString(hw_cfg.total_gpc_num));
    col_test_name.push_back(L"PMU_CTL_QUERY_HW_CFG [part_id]");
    col_test_result.push_back(IntToHexWideString(hw_cfg.part_id));
    col_test_name.push_back(L"PMU_CTL_QUERY_HW_CFG [pmu_ver]");
    col_test_result.push_back(IntToHexWideString(hw_cfg.pmu_ver));
    col_test_name.push_back(L"PMU_CTL_QUERY_HW_CFG [rev_id]");
    col_test_result.push_back(IntToHexWideString(hw_cfg.rev_id));
    col_test_name.push_back(L"PMU_CTL_QUERY_HW_CFG [variant_id]");
    col_test_result.push_back(IntToHexWideString(hw_cfg.variant_id));
    col_test_name.push_back(L"PMU_CTL_QUERY_HW_CFG [vendor_id]");
    col_test_result.push_back(IntToHexWideString(hw_cfg.vendor_id));
    col_test_name.push_back(L"PMU_CTL_QUERY_HW_CFG [midr_value]");
    col_test_result.push_back(IntToHexWideString(hw_cfg.midr_value, 20));

    // Tests General Purpose Counters detection
    col_test_name.push_back(L"gpc_nums[EVT_CORE]");
    col_test_result.push_back(std::to_wstring(gpc_nums[EVT_CORE]));
    col_test_name.push_back(L"gpc_nums[EVT_DSU]");
    col_test_result.push_back(std::to_wstring(gpc_nums[EVT_DSU]));
    col_test_name.push_back(L"gpc_nums[EVT_DMC_CLK]");
    col_test_result.push_back(std::to_wstring(gpc_nums[EVT_DMC_CLK]));
    col_test_name.push_back(L"gpc_nums[EVT_DMC_CLKDIV2]");
    col_test_result.push_back(std::to_wstring(gpc_nums[EVT_DMC_CLKDIV2]));

    // Tests Fixed Purpose Counters detection
    col_test_name.push_back(L"fpc_nums[EVT_CORE]");
    col_test_result.push_back(std::to_wstring(fpc_nums[EVT_CORE]));
    col_test_name.push_back(L"fpc_nums[EVT_DSU]");
    col_test_result.push_back(std::to_wstring(fpc_nums[EVT_DSU]));
    col_test_name.push_back(L"fpc_nums[EVT_DMC_CLK]");
    col_test_result.push_back(std::to_wstring(fpc_nums[EVT_DMC_CLK]));
    col_test_name.push_back(L"fpc_nums[EVT_DMC_CLKDIV2]");
    col_test_result.push_back(std::to_wstring(fpc_nums[EVT_DMC_CLKDIV2]));

    // Tests for event scheduling
    std::wstring evt_indexes, evt_notes;
    get_event_scheduling_test_data(ioctl_events, evt_indexes, evt_notes, EVT_CORE);
    col_test_name.push_back(L"ioctl_events[EVT_CORE].index");
    col_test_result.push_back(evt_indexes);
    col_test_name.push_back(L"ioctl_events[EVT_CORE].note");
    col_test_result.push_back(evt_notes);

    get_event_scheduling_test_data(ioctl_events, evt_indexes, evt_notes, EVT_DSU);
    col_test_name.push_back(L"ioctl_events[EVT_DSU].index");
    col_test_result.push_back(evt_indexes);
    col_test_name.push_back(L"ioctl_events[EVT_DSU].note");
    col_test_result.push_back(evt_notes);

    get_event_scheduling_test_data(ioctl_events, evt_indexes, evt_notes, EVT_DMC_CLK);
    col_test_name.push_back(L"ioctl_events[EVT_DMC_CLK].index");
    col_test_result.push_back(evt_indexes);
    col_test_name.push_back(L"ioctl_events[EVT_DMC_CLK].note");
    col_test_result.push_back(evt_notes);

    get_event_scheduling_test_data(ioctl_events, evt_indexes, evt_notes, EVT_DMC_CLKDIV2);
    col_test_name.push_back(L"ioctl_events[EVT_DMC_CLKDIV2].index");
    col_test_result.push_back(evt_indexes);
    col_test_name.push_back(L"ioctl_events[EVT_DMC_CLKDIV2].note");
    col_test_result.push_back(evt_notes);

    // Configuration
    std::vector<std::wstring> config_strs;
    drvconfig::get_configs(config_strs);

    for (auto& name : config_strs)
    {
        LONG l = 0;
        std::wstring s = L"<unknown>";

        col_test_name.push_back(L"config." + name);
        if (drvconfig::get(name, l))
            col_test_result.push_back(std::to_wstring(l));
        else if (drvconfig::get(name, s))
            col_test_result.push_back(s);
        else
            col_test_result.push_back(s);
    }
}

void pmu_device::do_test(uint32_t enable_bits,
    std::map<enum evt_class, std::vector<struct evt_noted>>& ioctl_events)
{
    std::vector<std::wstring> col_test_name, col_test_result;
    do_test_prep_tests(col_test_name, col_test_result, enable_bits, ioctl_events);

    TableOutput<TestOutputTraitsL, GlobalCharType> table(m_outputType);
    table.PresetHeaders();
    table.Insert(col_test_name, col_test_result);
    m_out.Print(table, true);
}

void pmu_device::do_version(_Out_ version_info& driver_ver)
{
    do_version_query(driver_ver);

    std::vector<std::wstring> col_component, col_version, col_gitver;
    col_component.push_back(L"wperf");
    col_version.push_back(std::to_wstring(MAJOR) + L"." +
        std::to_wstring(MINOR) + L"." +
        std::to_wstring(PATCH));
    col_component.push_back(L"wperf-driver");
    col_gitver.push_back(WPERF_GIT_VER_STR);
    col_version.push_back(std::to_wstring(driver_ver.major) + L"." +
        std::to_wstring(driver_ver.minor) + L"." +
        std::to_wstring(driver_ver.patch));
    col_gitver.push_back(driver_ver.gitver);
    TableOutput<VersionOutputTraitsL, GlobalCharType> table(m_outputType);
    table.PresetHeaders();
    table.Insert(col_component, col_version, col_gitver);
    m_out.Print(table, true);
}

bool pmu_device::detect_armh_dsu()
{
    ULONG DeviceListLength = 0;
    CONFIGRET cr = CR_SUCCESS;
    DEVPROPTYPE PropertyType;
    PWSTR DeviceList = NULL;
    WCHAR DeviceDesc[512];
    PWSTR CurrentDevice;
    ULONG PropertySize;
    DEVINST Devinst;

    cr = CM_Get_Device_ID_List_Size(&DeviceListLength, NULL, CM_GETIDLIST_FILTER_PRESENT);
    if (cr != CR_SUCCESS)
    {
        warning(L"warning: detect uncore: failed CM_Get_Device_ID_List_Size, cancel unCore support");
        goto fail0;
    }

    DeviceList = (PWSTR)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY,
        DeviceListLength * sizeof(WCHAR));
    if (!DeviceList)
    {
        warning(L"detect uncore: failed HeapAlloc for DeviceList, cancel unCore support");
        goto fail0;
    }

    cr = CM_Get_Device_ID_ListW(L"ACPI\\ARMHD500", DeviceList, DeviceListLength,
        CM_GETIDLIST_FILTER_PRESENT | CM_GETIDLIST_FILTER_ENUMERATOR);
    if (cr != CR_SUCCESS)
    {
        warning(L"warning: detect uncore: failed CM_Get_Device_ID_ListW, cancel unCore support");
        goto fail0;
    }

    dsu_cluster_num = 0;
    for (CurrentDevice = DeviceList; *CurrentDevice; CurrentDevice += wcslen(CurrentDevice) + 1, dsu_cluster_num++)
    {
        cr = CM_Locate_DevNodeW(&Devinst, CurrentDevice, CM_LOCATE_DEVNODE_NORMAL);
        if (cr != CR_SUCCESS)
        {
            warning(L"warning: detect uncore: failed CM_Locate_DevNodeW, cancel unCore support");
            goto fail0;
        }

        PropertySize = sizeof(DeviceDesc);
        cr = CM_Get_DevNode_PropertyW(Devinst, &DEVPKEY_Device_Siblings, &PropertyType,
            (PBYTE)DeviceDesc, &PropertySize, 0);
        if (cr != CR_SUCCESS)
        {
            warning(L"warning: detect uncore: failed CM_Get_DevNode_PropertyW, cancel unCore support");
            goto fail0;
        }

        std::vector < std::wstring > siblings;
        uint32_t core_num_idx = 0;
        for (PWSTR sibling = DeviceDesc; *sibling; sibling += wcslen(sibling) + 1, core_num_idx++)
            siblings.push_back(std::wstring(sibling));

        m_dsu_cluster_makeup.push_back(siblings);

        if (dsu_cluster_size)
        {
            if (core_num_idx != dsu_cluster_size)
            {
                warning(L"detect uncore: failed CM_Get_DevNode_PropertyW, cancel unCore support");
                goto fail0;
            }
        }
        else
        {
            dsu_cluster_size = core_num_idx;
        }
    }

    if (!dsu_cluster_num)
    {
        warning(L"detect uncore: failed finding cluster, cancel unCore support");
        goto fail0;
    }

    if (!dsu_cluster_size)
    {
        warning(L"warning: detect uncore: failed finding core inside cluster, cancel unCore support");
        goto fail0;
    }

    return true;
fail0:

    if (DeviceList)
        HeapFree(GetProcessHeap(), 0, DeviceList);

    return false;
}

bool pmu_device::detect_armh_dma()
{
    ULONG DeviceListLength = 0;
    CONFIGRET cr = CR_SUCCESS;
    PWSTR DeviceList = NULL;
    PWSTR CurrentDevice;
    DEVINST Devinst;

    cr = CM_Get_Device_ID_List_Size(&DeviceListLength, NULL, CM_GETIDLIST_FILTER_PRESENT);
    if (cr != CR_SUCCESS)
        goto fail1;

    DeviceList = (PWSTR)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY,
        DeviceListLength * sizeof(WCHAR));
    if (!DeviceList)
        goto fail1;

    cr = CM_Get_Device_ID_ListW(L"ACPI\\ARMHD620", DeviceList, DeviceListLength,
        CM_GETIDLIST_FILTER_PRESENT | CM_GETIDLIST_FILTER_ENUMERATOR);
    if (cr != CR_SUCCESS)
        goto fail1;

    for (CurrentDevice = DeviceList; *CurrentDevice; CurrentDevice += wcslen(CurrentDevice) + 1, dmc_num++)
    {
        cr = CM_Locate_DevNodeW(&Devinst, CurrentDevice, CM_LOCATE_DEVNODE_NORMAL);
        if (cr != CR_SUCCESS)
            goto fail1;

        LOG_CONF config;
        cr = CM_Get_First_Log_Conf(&config, Devinst, BOOT_LOG_CONF);
        if (cr != CR_SUCCESS)
            goto fail1;

        int log_conf_num = 0;
        do
        {
            RES_DES prev_resdes = (RES_DES)config;
            RES_DES resdes = 0;
            ULONG data_size;

            while (CM_Get_Next_Res_Des(&resdes, prev_resdes, ResType_Mem, NULL, 0) == CR_SUCCESS)
            {
                if (prev_resdes != config)
                    CM_Free_Res_Des_Handle(prev_resdes);

                prev_resdes = resdes;
                if (CM_Get_Res_Des_Data_Size(&data_size, resdes, 0) != CR_SUCCESS)
                    continue;

                std::unique_ptr<BYTE[]> resdes_data = std::make_unique<BYTE[]>(data_size);

                if (CM_Get_Res_Des_Data(resdes, resdes_data.get(), data_size, 0) != CR_SUCCESS)
                    continue;

                PMEM_RESOURCE mem_data = (PMEM_RESOURCE)resdes_data.get();
                if (mem_data->MEM_Header.MD_Alloc_End - mem_data->MEM_Header.MD_Alloc_Base + 1)
                    dmc_regions.push_back(std::make_pair(mem_data->MEM_Header.MD_Alloc_Base, mem_data->MEM_Header.MD_Alloc_End));
            }

            LOG_CONF config2;
            cr = CM_Get_Next_Log_Conf(&config2, config, 0);
            CONFIGRET cr2 = CM_Free_Log_Conf_Handle(config);

            if (cr2 != CR_SUCCESS)
                goto fail1;

            config = config2;
            log_conf_num++;

            if (log_conf_num > 1)
                goto fail1;
        } while (cr != CR_NO_MORE_LOG_CONF);
    }

    if (!dmc_num)
        goto fail1;

    return !dmc_regions.empty();
fail1:
    if (DeviceList)
        HeapFree(GetProcessHeap(), 0, DeviceList);

    return false;
}

void pmu_device::query_hw_cfg(struct hw_cfg& out)
{
    struct hw_cfg buf;
    DWORD res_len;

    enum pmu_ctl_action action = PMU_CTL_QUERY_HW_CFG;
    BOOL status = DeviceAsyncIoControl(m_device_handle, PMU_CTL_QUERY_HW_CFG, &action, (DWORD)sizeof(enum pmu_ctl_action), &buf, (DWORD)sizeof(struct hw_cfg), &res_len);
    if (!status)
        throw fatal_exception("PMU_CTL_QUERY_HW_CFG failed");

    if (res_len != sizeof(struct hw_cfg))
        throw fatal_exception("PMU_CTL_QUERY_HW_CFG returned unexpected length of data");

    out = buf;
}

const wchar_t* pmu_device::get_vendor_name(uint8_t vendor_id)
{
    if (arm64_vendor_names.count(vendor_id))
        return arm64_vendor_names[vendor_id];
    return L"Unknown Vendor";
}

// Use this function to print to wcerr runtime warnings in verbose mode.
// Do not use this function for debug. Instead use WindowsPerfDbgPrint().
void pmu_device::warning(const std::wstring wrn)
{
    if (do_verbose)
        m_out.GetErrorOutputStream() << L"warning: " << wrn << std::endl;
}

void pmu_device::get_pmu_device_cfg(struct pmu_device_cfg& cfg)
{
    memcpy(cfg.gpc_nums, gpc_nums, sizeof(gpc_nums));
    memcpy(cfg.fpc_nums, fpc_nums, sizeof(fpc_nums));
    cfg.core_num = core_num;
    cfg.pmu_ver = pmu_ver;
    cfg.dsu_cluster_num = dsu_cluster_num;
    cfg.dsu_cluster_size = dsu_cluster_size;
    cfg.dmc_num = dmc_num;
    cfg.has_dsu = has_dsu;
    cfg.has_dmc = has_dmc;
}

uint32_t pmu_device::stop_bits()
{
    uint32_t stop_bits = CTL_FLAG_CORE;
    if (has_dsu)
        stop_bits |= CTL_FLAG_DSU;
    if (has_dmc)
        stop_bits |= CTL_FLAG_DMC;

    return stop_bits;
}

uint32_t pmu_device::enable_bits(_In_ std::vector<enum evt_class>& e_classes)
{
    for (const auto& e : e_classes)
    {
        if (e == EVT_CORE)
        {
            m_enable_bits |= CTL_FLAG_CORE;
        }
        else if (e == EVT_DSU)
        {
            m_enable_bits |= CTL_FLAG_DSU;
        }
        else if (e == EVT_DMC_CLK || e == EVT_DMC_CLKDIV2)
        {
            m_enable_bits |= CTL_FLAG_DMC;
        }
        else
            throw fatal_exception("Unrecognized EVT_CLASS when mapping enable_bits");
    }

    return m_enable_bits;
}

const wchar_t* pmu_device::pmu_events_get_evt_class_name(enum evt_class e_class)
{
    return pmu_events::get_evt_class_name(e_class);
}

const wchar_t* pmu_device::pmu_events_get_event_name(uint16_t index, enum evt_class e_class)
{
    if (e_class == EVT_CORE && m_product_events.count(m_product_name))
        for (const auto& [key, value] : m_product_events[m_product_name])
            if (value.index == index)
                return value.name.c_str();

    return pmu_events::get_event_name(index, e_class);
}

const wchar_t* pmu_device::pmu_events_get_evt_name_prefix(enum evt_class e_class)
{
    return pmu_events::get_evt_name_prefix(e_class);
}

const wchar_t* pmu_device::pmu_events_get_evt_desc(uint16_t index, enum evt_class e_class)
{
    if (e_class == EVT_CORE && m_product_events.count(m_product_name))
        for (const auto& [key, value] : m_product_events[m_product_name])
            if (value.index == index)
                return value.title.c_str();

    return L"---";
}

#include "debug.h"

#pragma warning(push)
#pragma warning(disable:4100)
BOOL pmu_device::DeviceAsyncIoControl(
    _In_    HANDLE  hDevice,
    _In_    ULONG   IoControlCode,
    _In_    LPVOID  lpBuffer,
    _In_    DWORD   nNumberOfBytesToWrite,
    _Out_   LPVOID  lpOutBuffer,
    _In_    DWORD   nOutBufferSize,
    _Out_   LPDWORD lpBytesReturned
)
{
    *lpBytesReturned = 0;
    DEFINE_CUSTOM_IOCTL_RUNTIME(IoControlCode, IoControlCode);
    if (!DeviceIoControl(hDevice, IoControlCode, lpBuffer, nNumberOfBytesToWrite, lpOutBuffer, nOutBufferSize, lpBytesReturned, NULL))
    {
        WindowsPerfDbgPrint("Error: DeviceIoControl failed: GetLastError=%d\n", GetLastError());
        return FALSE;
    }
    return TRUE;
}
#pragma warning(pop)
