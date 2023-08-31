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
#include <Windows.h>

#define PATH_MAX 4096

#define PERF_RECORD_MISC_CPUMODE_MASK		(7 << 0)
#define PERF_RECORD_MISC_CPUMODE_UNKNOWN	(0 << 0)
#define PERF_RECORD_MISC_KERNEL			(1 << 0)
#define PERF_RECORD_MISC_USER			(2 << 0)
#define PERF_RECORD_MISC_HYPERVISOR		(3 << 0)
#define PERF_RECORD_MISC_GUEST_KERNEL		(4 << 0)
#define PERF_RECORD_MISC_GUEST_USER		(5 << 0)

namespace perfdata
{
	/******************* Adapted from Linux Kernel *************************/

	// These data structures have been copied from the kernel. See files under
	// tools/perf/util.

	enum perf_type_id {
		PERF_TYPE_HARDWARE = 0,
		PERF_TYPE_SOFTWARE = 1,
		PERF_TYPE_TRACEPOINT = 2,
		PERF_TYPE_HW_CACHE = 3,
		PERF_TYPE_RAW = 4,
		PERF_TYPE_BREAKPOINT = 5,

		PERF_TYPE_MAX,				/* non-ABI */
	};

	//
	// From tools/perf/util/header.h
	//

	enum {
		HEADER_RESERVED = 0,	/* always cleared */
		HEADER_FIRST_FEATURE = 1,
		HEADER_TRACING_DATA = 1,
		HEADER_BUILD_ID,

		HEADER_HOSTNAME,
		HEADER_OSRELEASE,
		HEADER_VERSION,
		HEADER_ARCH,
		HEADER_NRCPUS,
		HEADER_CPUDESC,
		HEADER_CPUID,
		HEADER_TOTAL_MEM,
		HEADER_CMDLINE,
		HEADER_EVENT_DESC,
		HEADER_CPU_TOPOLOGY,
		HEADER_NUMA_TOPOLOGY,
		HEADER_BRANCH_STACK,
		HEADER_PMU_MAPPINGS,
		HEADER_GROUP_DESC,
		HEADER_AUXTRACE,
		HEADER_STAT,
		HEADER_CACHE,
		HEADER_SAMPLE_TIME,
		HEADER_MEM_TOPOLOGY,
		HEADER_CLOCKID,
		HEADER_DIR_FORMAT,
		HEADER_BPF_PROG_INFO,
		HEADER_BPF_BTF,
		HEADER_COMPRESSED,
		HEADER_CPU_PMU_CAPS,
		HEADER_CLOCK_DATA,
		HEADER_HYBRID_TOPOLOGY,
		HEADER_HYBRID_CPU_PMU_CAPS,
		HEADER_LAST_FEATURE,
		HEADER_FEAT_BITS = 256,
	};

	/*
	 * Hardware event_id to monitor via a performance monitoring event:
	 */
	struct perf_event_attr {
		/*
		 * Major type: hardware/software/tracepoint/etc.
		 */
		UINT32 type;

		/*
		 * Size of the attr structure, for fwd/bwd compat.
		 */
		UINT32 size;

		/*
		 * Type specific configuration information.
		 */
		UINT64 config;

		union {
			UINT64 sample_period;
			UINT64 sample_freq;
		};

		UINT64 sample_type;
		UINT64 read_format;

		UINT64 disabled : 1,     /* off by default        */
			inherit : 1,        /* children inherit it   */
			pinned : 1,         /* must always be on PMU */
			exclusive : 1,      /* only group on PMU     */
			exclude_user : 1,   /* don't count user      */
			exclude_kernel : 1, /* ditto kernel          */
			exclude_hv : 1,     /* ditto hypervisor      */
			exclude_idle : 1,   /* don't count when idle */
			mmap : 1,           /* include mmap data     */
			comm : 1,           /* include comm data     */
			freq : 1,           /* use freq, not period  */
			inherit_stat : 1,   /* per task counts       */
			enable_on_exec : 1, /* next exec enables     */
			task : 1,           /* trace fork/exit       */
			watermark : 1,      /* wakeup_watermark      */
			/*
			 * precise_ip:
			 *
			 *  0 - SAMPLE_IP can have arbitrary skid
			 *  1 - SAMPLE_IP must have constant skid
			 *  2 - SAMPLE_IP requested to have 0 skid
			 *  3 - SAMPLE_IP must have 0 skid
			 *
			 *  See also PERF_RECORD_MISC_EXACT_IP
			 */
		precise_ip: 2,    /* skid constraint       */
			mmap_data : 1,     /* non-exec mmap data    */
			sample_id_all : 1, /* sample_type all events */

			exclude_host : 1,  /* don't count in host   */
			exclude_guest : 1, /* don't count in guest  */

			exclude_callchain_kernel : 1, /* exclude kernel callchains */
			exclude_callchain_user : 1,   /* exclude user callchains */
			mmap2 : 1,                    /* include mmap with inode data     */
			comm_exec : 1,      /* flag comm events that are due to an exec */
			use_clockid : 1,    /* use @clockid for time fields */
			context_switch : 1, /* context switch data */
			write_backward : 1, /* Write ring buffer from end to beginning */
			namespaces : 1,     /* include namespaces data */
			ksymbol : 1,        /* include ksymbol events */
			bpf_event : 1,      /* include bpf events */
			aux_output : 1,     /* generate AUX records instead of events */
			cgroup : 1,         /* include cgroup events */
			task_poke : 1,      /* include task_poke events */
			__reserved_1 : 30;

		union {
			UINT32 wakeup_events;    /* wakeup every n events */
			UINT32 wakeup_watermark; /* bytes before wakeup   */
		};

		UINT32 bp_type;
		union {
			UINT64 bp_addr;
			UINT64 config1; /* extension of config */
		};
		union {
			UINT64 bp_len;
			UINT64 config2; /* extension of config1 */
		};
		UINT64 branch_sample_type; /* enum perf_branch_sample_type */

		/*
		 * Defines set of user regs to dump on samples.
		 * See asm/perf_regs.h for details.
		 */
		UINT64 sample_regs_user;

		/*
		 * Defines size of the user stack to dump on samples.
		 */
		UINT32 sample_stack_user;

		/* Align to u64. */
		UINT32 __reserved_2;
	};

	struct perf_file_section {
		UINT64 offset;
		UINT64 size;
	};

	struct perf_file_attr {
		struct perf_event_attr attr;
		struct perf_file_section ids;
	};

	struct perf_file_header {
		UINT64 magic;
		UINT64 size;
		UINT64 attr_size;
		struct perf_file_section attrs;
		struct perf_file_section data;
		struct perf_file_section event_types;
		UINT64 features[4]; //HEADER_FEAT_BITS / 64 + ((HEADER_FEAT_BITS % 64) > 0 ? 1 : 0)
	};

	/*
	 * Bits that can be set in attr.sample_type to request information
	 * in the overflow packets.
	 */
	enum perf_event_sample_format {
		PERF_SAMPLE_IP = 1U << 0,
		PERF_SAMPLE_TID = 1U << 1,
		PERF_SAMPLE_TIME = 1U << 2,
		PERF_SAMPLE_ADDR = 1U << 3,
		PERF_SAMPLE_READ = 1U << 4,
		PERF_SAMPLE_CALLCHAIN = 1U << 5,
		PERF_SAMPLE_ID = 1U << 6,
		PERF_SAMPLE_CPU = 1U << 7,
		PERF_SAMPLE_PERIOD = 1U << 8,
		PERF_SAMPLE_STREAM_ID = 1U << 9,
		PERF_SAMPLE_RAW = 1U << 10,
		PERF_SAMPLE_BRANCH_STACK = 1U << 11,
		PERF_SAMPLE_REGS_USER = 1U << 12,
		PERF_SAMPLE_STACK_USER = 1U << 13,
		PERF_SAMPLE_WEIGHT = 1U << 14,
		PERF_SAMPLE_DATA_SRC = 1U << 15,
		PERF_SAMPLE_IDENTIFIER = 1U << 16,
		PERF_SAMPLE_TRANSACTION = 1U << 17,
		PERF_SAMPLE_REGS_INTR = 1U << 18,
		PERF_SAMPLE_PHYS_ADDR = 1U << 19,
		PERF_SAMPLE_AUX = 1U << 20,
		PERF_SAMPLE_CGROUP = 1U << 21,
		PERF_SAMPLE_DATA_PAGE_SIZE = 1U << 22,
		PERF_SAMPLE_CODE_PAGE_SIZE = 1U << 23,
		PERF_SAMPLE_WEIGHT_STRUCT = 1U << 24,

		PERF_SAMPLE_MAX = 1U << 25,		/* non-ABI */

		//__PERF_SAMPLE_CALLCHAIN_EARLY = 1ULL << 63, /* non-ABI; internal use */
	};

	struct perf_event_header {
		UINT32	type;
		UINT16	misc;
		UINT16	size;
	};

	enum perf_event_type {

		/*
		 * If perf_event_attr.sample_id_all is set then all event types will
		 * have the sample_type selected fields related to where/when
		 * (identity) an event took place (TID, TIME, ID, STREAM_ID, CPU,
		 * IDENTIFIER) described in PERF_RECORD_SAMPLE below, it will be stashed
		 * just after the perf_event_header and the fields already present for
		 * the existing fields, i.e. at the end of the payload. That way a newer
		 * perf.data file will be supported by older perf tools, with these new
		 * optional fields being ignored.
		 *
		 * struct sample_id {
		 * 	{ u32			pid, tid; } && PERF_SAMPLE_TID
		 * 	{ u64			time;     } && PERF_SAMPLE_TIME
		 * 	{ u64			id;       } && PERF_SAMPLE_ID
		 * 	{ u64			stream_id;} && PERF_SAMPLE_STREAM_ID
		 * 	{ u32			cpu, res; } && PERF_SAMPLE_CPU
		 *	{ u64			id;	  } && PERF_SAMPLE_IDENTIFIER
		 * } && perf_event_attr::sample_id_all
		 *
		 * Note that PERF_SAMPLE_IDENTIFIER duplicates PERF_SAMPLE_ID.  The
		 * advantage of PERF_SAMPLE_IDENTIFIER is that its position is fixed
		 * relative to header.size.
		 */

		 /*
		  * The MMAP events record the PROT_EXEC mappings so that we can
		  * correlate userspace IPs to code. They have the following structure:
		  *
		  * struct {
		  *	struct perf_event_header	header;
		  *
		  *	u32				pid, tid;
		  *	u64				addr;
		  *	u64				len;
		  *	u64				pgoff;
		  *	char				filename[];
		  * 	struct sample_id		sample_id;
		  * };
		  */
		PERF_RECORD_MMAP = 1,

		/*
		 * struct {
		 *	struct perf_event_header	header;
		 *	u64				id;
		 *	u64				lost;
		 * 	struct sample_id		sample_id;
		 * };
		 */
		PERF_RECORD_LOST = 2,

		/*
		 * struct {
		 *	struct perf_event_header	header;
		 *
		 *	u32				pid, tid;
		 *	char				comm[];
		 * 	struct sample_id		sample_id;
		 * };
		 */
		PERF_RECORD_COMM = 3,

		/*
		 * struct {
		 *	struct perf_event_header	header;
		 *	u32				pid, ppid;
		 *	u32				tid, ptid;
		 *	u64				time;
		 * 	struct sample_id		sample_id;
		 * };
		 */
		PERF_RECORD_EXIT = 4,

		/*
		 * struct {
		 *	struct perf_event_header	header;
		 *	u64				time;
		 *	u64				id;
		 *	u64				stream_id;
		 * 	struct sample_id		sample_id;
		 * };
		 */
		PERF_RECORD_THROTTLE = 5,
		PERF_RECORD_UNTHROTTLE = 6,

		/*
		 * struct {
		 *	struct perf_event_header	header;
		 *	u32				pid, ppid;
		 *	u32				tid, ptid;
		 *	u64				time;
		 * 	struct sample_id		sample_id;
		 * };
		 */
		PERF_RECORD_FORK = 7,

		/*
		 * struct {
		 *	struct perf_event_header	header;
		 *	u32				pid, tid;
		 *
		 *	struct read_format		values;
		 * 	struct sample_id		sample_id;
		 * };
		 */
		PERF_RECORD_READ = 8,

		/*
		 * struct {
		 *	struct perf_event_header	header;
		 *
		 *	#
		 *	# Note that PERF_SAMPLE_IDENTIFIER duplicates PERF_SAMPLE_ID.
		 *	# The advantage of PERF_SAMPLE_IDENTIFIER is that its position
		 *	# is fixed relative to header.
		 *	#
		 *
		 *	{ u64			id;	  } && PERF_SAMPLE_IDENTIFIER
		 *	{ u64			ip;	  } && PERF_SAMPLE_IP
		 *	{ u32			pid, tid; } && PERF_SAMPLE_TID
		 *	{ u64			time;     } && PERF_SAMPLE_TIME
		 *	{ u64			addr;     } && PERF_SAMPLE_ADDR
		 *	{ u64			id;	  } && PERF_SAMPLE_ID
		 *	{ u64			stream_id;} && PERF_SAMPLE_STREAM_ID
		 *	{ u32			cpu, res; } && PERF_SAMPLE_CPU
		 *	{ u64			period;   } && PERF_SAMPLE_PERIOD
		 *
		 *	{ struct read_format	values;	  } && PERF_SAMPLE_READ
		 *
		 *	{ u64			nr,
		 *	  u64			ips[nr];  } && PERF_SAMPLE_CALLCHAIN
		 *
		 *	#
		 *	# The RAW record below is opaque data wrt the ABI
		 *	#
		 *	# That is, the ABI doesn't make any promises wrt to
		 *	# the stability of its content, it may vary depending
		 *	# on event, hardware, kernel version and phase of
		 *	# the moon.
		 *	#
		 *	# In other words, PERF_SAMPLE_RAW contents are not an ABI.
		 *	#
		 *
		 *	{ u32			size;
		 *	  char                  data[size];}&& PERF_SAMPLE_RAW
		 *
		 *	{ u64                   nr;
		 *	  { u64	hw_idx; } && PERF_SAMPLE_BRANCH_HW_INDEX
		 *        { u64 from, to, flags } lbr[nr];
		 *      } && PERF_SAMPLE_BRANCH_STACK
		 *
		 * 	{ u64			abi; # enum perf_sample_regs_abi
		 * 	  u64			regs[weight(mask)]; } && PERF_SAMPLE_REGS_USER
		 *
		 * 	{ u64			size;
		 * 	  char			data[size];
		 * 	  u64			dyn_size; } && PERF_SAMPLE_STACK_USER
		 *
		 *	{ union perf_sample_weight
		 *	 {
		 *		u64		full; && PERF_SAMPLE_WEIGHT
		 *	#if defined(__LITTLE_ENDIAN_BITFIELD)
		 *		struct {
		 *			u32	var1_dw;
		 *			u16	var2_w;
		 *			u16	var3_w;
		 *		} && PERF_SAMPLE_WEIGHT_STRUCT
		 *	#elif defined(__BIG_ENDIAN_BITFIELD)
		 *		struct {
		 *			u16	var3_w;
		 *			u16	var2_w;
		 *			u32	var1_dw;
		 *		} && PERF_SAMPLE_WEIGHT_STRUCT
		 *	#endif
		 *	 }
		 *	}
		 *	{ u64			data_src; } && PERF_SAMPLE_DATA_SRC
		 *	{ u64			transaction; } && PERF_SAMPLE_TRANSACTION
		 *	{ u64			abi; # enum perf_sample_regs_abi
		 *	  u64			regs[weight(mask)]; } && PERF_SAMPLE_REGS_INTR
		 *	{ u64			phys_addr;} && PERF_SAMPLE_PHYS_ADDR
		 *	{ u64			size;
		 *	  char			data[size]; } && PERF_SAMPLE_AUX
		 *	{ u64			data_page_size;} && PERF_SAMPLE_DATA_PAGE_SIZE
		 *	{ u64			code_page_size;} && PERF_SAMPLE_CODE_PAGE_SIZE
		 * };
		 */
		PERF_RECORD_SAMPLE = 9,

		/*
		 * The MMAP2 records are an augmented version of MMAP, they add
		 * maj, min, ino numbers to be used to uniquely identify each mapping
		 *
		 * struct {
		 *	struct perf_event_header	header;
		 *
		 *	u32				pid, tid;
		 *	u64				addr;
		 *	u64				len;
		 *	u64				pgoff;
		 *	union {
		 *		struct {
		 *			u32		maj;
		 *			u32		min;
		 *			u64		ino;
		 *			u64		ino_generation;
		 *		};
		 *		struct {
		 *			u8		build_id_size;
		 *			u8		__reserved_1;
		 *			u16		__reserved_2;
		 *			u8		build_id[20];
		 *		};
		 *	};
		 *	u32				prot, flags;
		 *	char				filename[];
		 * 	struct sample_id		sample_id;
		 * };
		 */
		PERF_RECORD_MMAP2 = 10,

		/*
		 * Records that new data landed in the AUX buffer part.
		 *
		 * struct {
		 * 	struct perf_event_header	header;
		 *
		 * 	u64				aux_offset;
		 * 	u64				aux_size;
		 *	u64				flags;
		 * 	struct sample_id		sample_id;
		 * };
		 */
		PERF_RECORD_AUX = 11,

		/*
		 * Indicates that instruction trace has started
		 *
		 * struct {
		 *	struct perf_event_header	header;
		 *	u32				pid;
		 *	u32				tid;
		 *	struct sample_id		sample_id;
		 * };
		 */
		PERF_RECORD_ITRACE_START = 12,

		/*
		 * Records the dropped/lost sample number.
		 *
		 * struct {
		 *	struct perf_event_header	header;
		 *
		 *	u64				lost;
		 *	struct sample_id		sample_id;
		 * };
		 */
		PERF_RECORD_LOST_SAMPLES = 13,

		/*
		 * Records a context switch in or out (flagged by
		 * PERF_RECORD_MISC_SWITCH_OUT). See also
		 * PERF_RECORD_SWITCH_CPU_WIDE.
		 *
		 * struct {
		 *	struct perf_event_header	header;
		 *	struct sample_id		sample_id;
		 * };
		 */
		PERF_RECORD_SWITCH = 14,

		/*
		 * CPU-wide version of PERF_RECORD_SWITCH with next_prev_pid and
		 * next_prev_tid that are the next (switching out) or previous
		 * (switching in) pid/tid.
		 *
		 * struct {
		 *	struct perf_event_header	header;
		 *	u32				next_prev_pid;
		 *	u32				next_prev_tid;
		 *	struct sample_id		sample_id;
		 * };
		 */
		PERF_RECORD_SWITCH_CPU_WIDE = 15,

		/*
		 * struct {
		 *	struct perf_event_header	header;
		 *	u32				pid;
		 *	u32				tid;
		 *	u64				nr_namespaces;
		 *	{ u64				dev, inode; } [nr_namespaces];
		 *	struct sample_id		sample_id;
		 * };
		 */
		PERF_RECORD_NAMESPACES = 16,

		/*
		 * Record ksymbol register/unregister events:
		 *
		 * struct {
		 *	struct perf_event_header	header;
		 *	u64				addr;
		 *	u32				len;
		 *	u16				ksym_type;
		 *	u16				flags;
		 *	char				name[];
		 *	struct sample_id		sample_id;
		 * };
		 */
		PERF_RECORD_KSYMBOL = 17,

		/*
		 * Record bpf events:
		 *  enum perf_bpf_event_type {
		 *	PERF_BPF_EVENT_UNKNOWN		= 0,
		 *	PERF_BPF_EVENT_PROG_LOAD	= 1,
		 *	PERF_BPF_EVENT_PROG_UNLOAD	= 2,
		 *  };
		 *
		 * struct {
		 *	struct perf_event_header	header;
		 *	u16				type;
		 *	u16				flags;
		 *	u32				id;
		 *	u8				tag[BPF_TAG_SIZE];
		 *	struct sample_id		sample_id;
		 * };
		 */
		PERF_RECORD_BPF_EVENT = 18,

		/*
		 * struct {
		 *	struct perf_event_header	header;
		 *	u64				id;
		 *	char				path[];
		 *	struct sample_id		sample_id;
		 * };
		 */
		PERF_RECORD_CGROUP = 19,

		/*
		 * Records changes to kernel text i.e. self-modified code. 'old_len' is
		 * the number of old bytes, 'new_len' is the number of new bytes. Either
		 * 'old_len' or 'new_len' may be zero to indicate, for example, the
		 * addition or removal of a trampoline. 'bytes' contains the old bytes
		 * followed immediately by the new bytes.
		 *
		 * struct {
		 *	struct perf_event_header	header;
		 *	u64				addr;
		 *	u16				old_len;
		 *	u16				new_len;
		 *	u8				bytes[];
		 *	struct sample_id		sample_id;
		 * };
		 */
		PERF_RECORD_TEXT_POKE = 20,

		/*
		 * Data written to the AUX area by hardware due to aux_output, may need
		 * to be matched to the event by an architecture-specific hardware ID.
		 * This records the hardware ID, but requires sample_id to provide the
		 * event ID. e.g. Intel PT uses this record to disambiguate PEBS-via-PT
		 * records from multiple events.
		 *
		 * struct {
		 *	struct perf_event_header	header;
		 *	u64				hw_id;
		 *	struct sample_id		sample_id;
		 * };
		 */
		PERF_RECORD_AUX_OUTPUT_HW_ID = 21,

		PERF_RECORD_MAX,			/* non-ABI */
	};

	/********* LOCAL *****************/
	const UINT64 PERF_FILE_MAGIC = 0x32454c4946524550LL;

	struct perf_data_string
	{
		UINT32 size;
		char* data; // data[size] string
	};

	struct perf_data_multi_string
	{
		UINT32 count;
		perf_data_string* data;
	};

	struct perf_data_comm_event {
		struct perf_event_header header;
		UINT32 pid, tid;
		char comm[16];
	};

	struct perf_data_sample_event {
		struct perf_event_header header;
		UINT64 ip; // Instruction point
		UINT32 pid, tid;
		UINT64 id;
		UINT32 cpu, res;
	};

	struct perf_data_mmap_event {
		struct perf_event_header header;
		UINT32 pid, tid;
		UINT64 start;
		UINT64 len;
		UINT64 pgoff;
		char filename[PATH_MAX];
	};
}

#include "user_request.h"
#include "utils.h"

class PerfDataWriter
{
public:
	typedef std::variant<perfdata::perf_data_comm_event, perfdata::perf_data_sample_event, perfdata::perf_data_mmap_event> PerfEvent;

private:
	std::ofstream m_file;
	perfdata::perf_file_header m_file_header { 0 };
	std::vector<perfdata::perf_file_attr> m_attributes;
	std::vector<UINT64> m_sampling_events;

	std::vector<PerfEvent> m_events;

	perfdata::perf_data_multi_string m_cmdline { 0 };
	size_t m_cmdline_size = 0;
	bool m_has_cmdline = false;

	short m_feature_count = 0;

	inline void set_feature(UINT64 feature)
	{
		m_file_header.features[feature / 64] |= (1ULL << (feature % 64));
	}

	inline size_t get_features_section_size()
	{
		return m_feature_count * sizeof(perfdata::perf_file_section);
	}

	size_t WriteAttributeSection(size_t data_offset);
	size_t WriteIDs(size_t offset);
	size_t WriteFeatures(size_t file_section_offset, size_t data_offset);
	size_t WriteDataSection(size_t offset);

	PerfEvent PerfDataWriter::get_comm_event(DWORD pid, std::wstring& command);
	PerfEvent PerfDataWriter::get_sample_event(DWORD pid, UINT64 ip, UINT32 cpu, UINT64 event_type);
	PerfEvent PerfDataWriter::get_mmap_event(DWORD pid, UINT64 addr, UINT64 len, std::wstring& filename, UINT64 pgoff);
	
public:
	enum PerfSupportedEventTypes
	{
		COMM,
		SAMPLE,
		MMAP
	};

	PerfDataWriter()
	{
		m_file_header.magic = perfdata::PERF_FILE_MAGIC;
		m_file_header.size = sizeof(perfdata::perf_file_header);
		m_file_header.attr_size = sizeof(perfdata::perf_file_attr);
		m_file_header.attrs.offset = sizeof(perfdata::perf_file_header);
		m_file_header.data.offset = 0;
		m_file_header.data.size = 0;
		m_file_header.event_types.offset = 0;
		m_file_header.event_types.size = 0;

		m_cmdline.count = 0;
		m_cmdline.data = NULL;
	}

	~PerfDataWriter()
	{
		if (m_cmdline.data)
		{
			for (UINT32 i = 0; i < m_cmdline.count; i++)
			{
				if (m_cmdline.data[i].data) delete[] m_cmdline.data[i].data;
			}
			delete[] m_cmdline.data;
		}
	}

	void WriteCommandLine(const int argc, const wchar_t* argv[]);

	void RegisterSampleEvent(UINT64 windows_perf_sampling_event)
	{
		perfdata::perf_file_attr fattr{ 0 };
		fattr.attr.size = sizeof(perfdata::perf_event_attr);
		fattr.attr.sample_type = perfdata::PERF_SAMPLE_IP | perfdata::PERF_SAMPLE_TID | perfdata::PERF_SAMPLE_CPU | perfdata::PERF_SAMPLE_ID;
		fattr.attr.mmap = 1;
		fattr.attr.type = perfdata::PERF_TYPE_HARDWARE;

		// [TODO] Understand how to actually map WindowsPerf events here
		fattr.attr.config = 0;

		fattr.ids.offset = 0;
		fattr.ids.size = sizeof(UINT64);
		m_attributes.push_back(fattr);
		m_sampling_events.push_back(windows_perf_sampling_event);
	}

	template <typename... Ts>
	void RegisterEvent(PerfSupportedEventTypes type, Ts... args)
	{
		PerfEvent event;
		switch (type)
		{
		case COMM:
		{
			if constexpr (sizeof...(Ts) == 2)
			{
				event = get_comm_event(args...);
			}
			break;
		}
		case SAMPLE:
		{
			if constexpr (sizeof...(Ts) == 4)
			{
				event = get_sample_event(args...);
			}
			break;
		}
		case MMAP:
		{
			if constexpr (sizeof...(Ts) == 5)
			{
				event = get_mmap_event(args...);
			}
			break;
		}
		}
		m_events.push_back(event);
	}

	void Write(std::string filename = "perf.data");
};
