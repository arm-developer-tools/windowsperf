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

#include "wperf-common\macros.h"

#define MAX_GITVER_SIZE 32
#define MAX_FEATURESTRING_SIZE 128
#define MAX_DEVICE_ID_STR_SIZE 128

//
// Below structures represent binary protocol between wperf and wperf-driver
//
enum evt_class
{
    EVT_CLASS_FIRST,
    EVT_CORE = EVT_CLASS_FIRST,
    EVT_DSU,
    EVT_DMC_CLK,
    EVT_DMC_CLKDIV2,
    EVT_CLASS_NUM,
};

enum status_flag                // Status of the driver lock
{
    STS_IDLE,                   // When no process has the driver locked aka "the default state".
    STS_BUSY,                   // When another process already has the driver lock.
    STS_LOCK_AQUIRED,           // When the calling process has successfully acquired the driver lock.
    STS_INSUFFICIENT_RESOURCES, // When the driver was unable to allocate any GPU.
    STS_UNKNOWN_ERROR,          // Catch all for other unknown errors.
};

struct evt_hdr
{
    enum evt_class evt_class;
    UINT16 num;
};

struct hw_cfg
{
    UINT8   pmu_ver;
    UINT8   fpc_num;
    UINT8   gpc_num;
    UINT8   total_gpc_num;
    UINT8   vendor_id;
    UINT8   variant_id;
    UINT8   arch_id;
    UINT8   rev_id;
    UINT16  part_id;
    UINT16  core_num;
    UINT64  midr_value;
    UINT64  id_aa64dfr0_value;
    UINT8   counter_idx_map[AARCH64_MAX_HWC_SUPP + 1]; // We cannot correctly infer this from user mode.
    wchar_t device_id_str[MAX_DEVICE_ID_STR_SIZE];      // Driver devices and their capabilities
    UINT64  pmbidr_el1_value;                           // FEAT_SPE only
    UINT64  pmsidr_el1_value;                           // FEAT_SPE only
};

struct version_info
{
    UINT8 major;
    UINT8 minor;
    UINT8 patch;
    WCHAR gitver[MAX_GITVER_SIZE];
    WCHAR featurestring[MAX_FEATURESTRING_SIZE];
};

#define PMU_CTL_ACTION_OFFSET 0x900

//
// PMU communication
//
enum pmu_ctl_action
{
	PMU_CTL_START = PMU_CTL_ACTION_OFFSET,
	PMU_CTL_STOP,
	PMU_CTL_RESET,
	PMU_CTL_QUERY_HW_CFG,
	PMU_CTL_QUERY_SUPP_EVENTS,
	PMU_CTL_QUERY_VERSION,
	PMU_CTL_ASSIGN_EVENTS,
	PMU_CTL_READ_COUNTING,
	DSU_CTL_INIT,
	DSU_CTL_READ_COUNTING,
	DMC_CTL_INIT,
	DMC_CTL_READ_COUNTING,
	PMU_CTL_SAMPLE_SET_SRC,
	PMU_CTL_SAMPLE_START,
	PMU_CTL_SAMPLE_STOP,
	PMU_CTL_SAMPLE_GET,
    PMU_CTL_LOCK_ACQUIRE,
	PMU_CTL_LOCK_RELEASE,
    PMU_CTL_SPE_INIT,
    PMU_CTL_SPE_GET_SIZE,
    PMU_CTL_SPE_GET_BUFFER,
    PMU_CTL_SPE_START,
    PMU_CTL_SPE_STOP,
};

#define IOCTL_PMU_CTL_START                     CTL_CODE(WPERF_TYPE,  PMU_CTL_START,                METHOD_BUFFERED, FILE_READ_DATA|FILE_WRITE_DATA)
#define IOCTL_PMU_CTL_STOP                      CTL_CODE(WPERF_TYPE,  PMU_CTL_STOP,                 METHOD_BUFFERED, FILE_READ_DATA|FILE_WRITE_DATA)
#define IOCTL_PMU_CTL_RESET                     CTL_CODE(WPERF_TYPE,  PMU_CTL_RESET,                METHOD_BUFFERED, FILE_READ_DATA|FILE_WRITE_DATA)
#define IOCTL_PMU_CTL_QUERY_HW_CFG              CTL_CODE(WPERF_TYPE,  PMU_CTL_QUERY_HW_CFG,         METHOD_BUFFERED, FILE_READ_DATA|FILE_WRITE_DATA)
#define IOCTL_PMU_CTL_QUERY_SUPP_EVENTS         CTL_CODE(WPERF_TYPE,  PMU_CTL_QUERY_SUPP_EVENTS,    METHOD_BUFFERED, FILE_READ_DATA|FILE_WRITE_DATA)
#define IOCTL_PMU_CTL_QUERY_VERSION 	        CTL_CODE(WPERF_TYPE,  PMU_CTL_QUERY_VERSION,        METHOD_BUFFERED, FILE_READ_DATA|FILE_WRITE_DATA)
#define IOCTL_PMU_CTL_ASSIGN_EVENTS 	        CTL_CODE(WPERF_TYPE,  PMU_CTL_ASSIGN_EVENTS,        METHOD_BUFFERED, FILE_READ_DATA|FILE_WRITE_DATA)
#define IOCTL_PMU_CTL_READ_COUNTING 	        CTL_CODE(WPERF_TYPE,  PMU_CTL_READ_COUNTING,        METHOD_BUFFERED, FILE_READ_DATA|FILE_WRITE_DATA)
#define IOCTL_DSU_CTL_INIT 	                    CTL_CODE(WPERF_TYPE,  DSU_CTL_INIT,                 METHOD_BUFFERED, FILE_READ_DATA|FILE_WRITE_DATA)
#define IOCTL_DSU_CTL_READ_COUNTING 	        CTL_CODE(WPERF_TYPE,  DSU_CTL_READ_COUNTING,        METHOD_BUFFERED, FILE_READ_DATA|FILE_WRITE_DATA)
#define IOCTL_DMC_CTL_INIT 	                    CTL_CODE(WPERF_TYPE,  DMC_CTL_INIT,                 METHOD_BUFFERED, FILE_READ_DATA|FILE_WRITE_DATA)
#define IOCTL_DMC_CTL_READ_COUNTING	            CTL_CODE(WPERF_TYPE,  DMC_CTL_READ_COUNTING,        METHOD_BUFFERED, FILE_READ_DATA|FILE_WRITE_DATA)
#define IOCTL_PMU_CTL_SAMPLE_SET_SRC 	        CTL_CODE(WPERF_TYPE,  PMU_CTL_SAMPLE_SET_SRC,       METHOD_BUFFERED, FILE_READ_DATA|FILE_WRITE_DATA)
#define IOCTL_PMU_CTL_SAMPLE_START 	            CTL_CODE(WPERF_TYPE,  PMU_CTL_SAMPLE_START,         METHOD_BUFFERED, FILE_READ_DATA|FILE_WRITE_DATA)
#define IOCTL_PMU_CTL_SAMPLE_STOP 	            CTL_CODE(WPERF_TYPE,  PMU_CTL_SAMPLE_STOP,          METHOD_BUFFERED, FILE_READ_DATA|FILE_WRITE_DATA)
#define IOCTL_PMU_CTL_SAMPLE_GET 	            CTL_CODE(WPERF_TYPE,  PMU_CTL_SAMPLE_GET,           METHOD_BUFFERED, FILE_READ_DATA|FILE_WRITE_DATA)
#define IOCTL_PMU_CTL_LOCK_ACQUIRE	            CTL_CODE(WPERF_TYPE,  PMU_CTL_LOCK_ACQUIRE,         METHOD_BUFFERED, FILE_READ_DATA|FILE_WRITE_DATA)
#define IOCTL_PMU_CTL_LOCK_RELEASE 	            CTL_CODE(WPERF_TYPE,  PMU_CTL_LOCK_RELEASE,         METHOD_BUFFERED, FILE_READ_DATA|FILE_WRITE_DATA)
#define IOCTL_PMU_CTL_SPE_INIT                  CTL_CODE(WPERF_TYPE,  PMU_CTL_SPE_INIT,             METHOD_BUFFERED, FILE_READ_DATA|FILE_WRITE_DATA)
#define IOCTL_PMU_CTL_SPE_GET_SIZE              CTL_CODE(WPERF_TYPE,  PMU_CTL_SPE_GET_SIZE,         METHOD_BUFFERED, FILE_READ_DATA|FILE_WRITE_DATA)
#define IOCTL_PMU_CTL_SPE_GET_BUFFER            CTL_CODE(WPERF_TYPE,  PMU_CTL_SPE_GET_BUFFER,       METHOD_BUFFERED, FILE_READ_DATA|FILE_WRITE_DATA)
#define IOCTL_PMU_CTL_SPE_START                 CTL_CODE(WPERF_TYPE,  PMU_CTL_SPE_START,            METHOD_BUFFERED, FILE_READ_DATA|FILE_WRITE_DATA)
#define IOCTL_PMU_CTL_SPE_STOP                  CTL_CODE(WPERF_TYPE,  PMU_CTL_SPE_STOP,             METHOD_BUFFERED, FILE_READ_DATA|FILE_WRITE_DATA)

enum lock_flag
{
    LOCK_GET,           // Ask to lock the driver and acquire "session"
    LOCK_GET_FORCE,     // Force driver to release the driver and lock it with the current process
    LOCK_RELEASE,       // Release the driver after acquired lock
};

struct lock_request
{
    enum lock_flag flag;
};

typedef struct
{
    UINT32 event_src;
    UINT32 interval;
    UINT32 filter_bits;
} SampleSrcDesc;

#pragma warning(push)
#pragma warning(disable:4200)
typedef struct
{
    UINT32 core_idx;
    SampleSrcDesc sources[0];
} PMUSampleSetSrcHdr;
#pragma warning(pop)

struct PMUSampleSummary
{
    UINT64 sample_generated;
    UINT64 sample_dropped;
};

typedef struct
{
    UINT64 lr;
    UINT64 pc;
    UINT64 ov_flags;
    UINT32 spe_event_idx;
} FrameChain;

struct PMUCtlGetSampleHdr
{
    UINT32 core_idx;
};

struct PMUSamplePayload
{
    UINT32 size;                                // How many framechains in payload
    FrameChain payload[SAMPLE_CHAIN_BUFFER_SIZE];   
};

struct pmu_ctl_ver_hdr
{
    struct version_info version;
};

struct pmu_ctl_cores_count_hdr
{
    size_t cores_count;                         //!< How many cores are stored in cores_idx. Values: 0...CORE_NUM
    UINT8  cores_no[MAX_PMU_CTL_CORES_COUNT];   //!< List of core NOs, indexed from 0 to cores_count-1
};

struct pmu_ctl_hdr
{
    struct pmu_ctl_cores_count_hdr cores_idx;
    LONG period;
	UINT8 dmc_idx;
#define CTL_FLAG_CORE (0x1 << 0)
#define CTL_FLAG_DSU  (0x1 << 1)
#define CTL_FLAG_DMC  (0x1 << 2)
#define CTL_FLAG_SPE  (0x1 << 3)
#define CTL_FLAG_MAX  (0x1 << 4)
#define CTRL_FLAG_VALID(flag_s) (flag_s < CTL_FLAG_MAX)
	UINT32 flags;
};

struct pmu_ctl_evt_assign_hdr
{
    UINT32 core_idx;
    UINT8 dmc_idx;
    UINT64 filter_bits;
};

struct pmu_event_usr
{
    UINT32 event_idx;
    UINT64 filter_bits;
    UINT64 value;
    UINT64 scheduled;
};

typedef struct pmu_event_read_out
{
    UINT32 evt_num;
    UINT64 round;
    struct pmu_event_usr evts[MAX_MANAGED_CORE_EVENTS];
} ReadOut;

//
// SPE communication
// 
struct spe_ctl_hdr
{
    struct pmu_ctl_cores_count_hdr cores_idx;
    UINT64 buffer_size;
    UINT8 operation_filter;
#define SPE_OPERATON_FILTER_B  0b001
#define SPE_OPERATON_FILTER_LD 0b010
#define SPE_OPERATON_FILTER_ST 0b100
    UINT64 event_filter;
    UINT64 config_flags;
#define SPE_CTL_FLAG_RND (0x1 << 0)     // config_flags: jitter filter flag
#define SPE_CTL_FLAG_TS  (0x1 << 1)     // config_flags: ts_enable filter flag
#define SPE_CTL_FLAG_MIN (0x1 << 2)     // config_flags: min_latency=<n> filter flag
#define SPE_CTL_FLAG_VAL_MASK 0xFFFF    // PMSLATFR_EL1.MINLAT is 16-bit wide
#define SPE_CTL_FLAG_VAL_12_BIT_MASK 0x0FFF    // PMSLATFR_EL1.MINLAT is 12-bit wide if CountSize == 0b0010
    UINT32 interval;
#define SPE_CTL_INTERVAL_VAL_MASK 0x0FFFFF    // INTERVAL, bits [31:8] is interval counter reload value
};

//
// DSU communication
//
struct dsu_ctl_hdr
{
    UINT16 cluster_num;
    UINT16 cluster_size;
};

typedef struct dsu_read_counting_out
{
    UINT32 evt_num;
    UINT64 round;
    struct pmu_event_usr evts[MAX_MANAGED_DSU_EVENTS];
} DSUReadOut;

struct dsu_cfg
{
    UINT8   fpc_num;
    UINT8   gpc_num;
};

//
// DMC communication
//
struct dmc_cfg
{
    UINT8   clk_fpc_num;
    UINT8   clk_gpc_num;
    UINT8   clkdiv2_fpc_num;
    UINT8   clkdiv2_gpc_num;
};

typedef struct dmc_pmu_event_read_out
{
	struct pmu_event_usr clk_events[MAX_MANAGED_DMC_CLK_EVENTS];
	struct pmu_event_usr clkdiv2_events[MAX_MANAGED_DMC_CLKDIV2_EVENTS];
	UINT8 clk_events_num;
	UINT8 clkdiv2_events_num;
} DMCReadOut;

#pragma warning(push)
#pragma warning(disable:4200)
struct dmc_ctl_hdr
{
    UINT8 dmc_num;
    UINT64 addr[0];
};
#pragma warning(pop)
