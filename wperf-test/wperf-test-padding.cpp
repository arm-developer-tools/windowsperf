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

#include "pch.h"
#include "CppUnitTest.h"

#include <algorithm>
#include <windows.h>
#include "wperf/events.h"
#include "wperf/parsers.h"
#include "wperf/padding.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace wperftest_common
{
	TEST_CLASS(wperftest_padding)
	{
	public:
		TEST_METHOD(test_padding_gpc_nums_5_no_groups)
		{
			std::map<enum evt_class, std::vector<struct evt_noted>> ioctl_events;
			std::map<enum evt_class, std::deque<struct evt_noted>> events;
			std::map<enum evt_class, std::vector<struct evt_noted>> groups;
			std::wstring note;
			struct pmu_device_cfg pmu_cfg = { 0 };

			pmu_cfg.gpc_nums[EVT_CORE] = 5;	// parse_events_str only uses gpc_nums[]

			parse_events_str(L"inst_spec,vfp_spec,ase_spec,br_immed_spec,crypto_spec,ld_spec,st_spec", events, groups, note, pmu_cfg);

			Assert::AreEqual(events[EVT_CORE].size(), (size_t)7);

			set_event_padding(ioctl_events, pmu_cfg, events, groups);

			Assert::AreEqual(ioctl_events[EVT_CORE].size(), (size_t)7);
		}

		TEST_METHOD(test_padding_gpc_nums_5_groups_331)
		{
			std::map<enum evt_class, std::vector<struct evt_noted>> ioctl_events;
			std::map<enum evt_class, std::deque<struct evt_noted>> events;
			std::map<enum evt_class, std::vector<struct evt_noted>> groups;
			std::wstring note;
			struct pmu_device_cfg pmu_cfg = { 0 };

			pmu_cfg.gpc_nums[EVT_CORE] = 5;	// parse_events_str only uses gpc_nums[]

			parse_events_str(L"{inst_spec,vfp_spec,ase_spec},{br_immed_spec,crypto_spec,ld_spec},st_spec", events, groups, note, pmu_cfg);

			Assert::AreEqual(events[EVT_CORE].size(), (size_t)1);

			set_event_padding(ioctl_events, pmu_cfg, events, groups);

			Assert::AreEqual(ioctl_events[EVT_CORE].size(), (size_t)10);
		}

		TEST_METHOD(test_padding_gpc_nums_6_groups_53)
		{
			std::map<enum evt_class, std::vector<struct evt_noted>> ioctl_events;
			std::map<enum evt_class, std::deque<struct evt_noted>> events;
			std::map<enum evt_class, std::vector<struct evt_noted>> groups;
			std::wstring note;
			struct pmu_device_cfg pmu_cfg = { 0 };

			pmu_cfg.gpc_nums[EVT_CORE] = 6;	// parse_events_str only uses gpc_nums[]

			parse_events_str(L"inst_spec,vfp_spec,ase_spec,dp_spec,ld_spec,{st_spec,br_immed_spec,crypto_spec}", events, groups, note, pmu_cfg);

			Assert::AreEqual(events[EVT_CORE].size(), (size_t)5);

			set_event_padding(ioctl_events, pmu_cfg, events, groups);

			Assert::AreEqual(ioctl_events[EVT_CORE].size(), (size_t)12);
		}

		TEST_METHOD(test_padding_gpc_nums_5_groups_233)
		{
			std::map<enum evt_class, std::vector<struct evt_noted>> ioctl_events;
			std::map<enum evt_class, std::deque<struct evt_noted>> events;
			std::map<enum evt_class, std::vector<struct evt_noted>> groups;
			std::wstring note;
			struct pmu_device_cfg pmu_cfg = { 0 };

			pmu_cfg.gpc_nums[EVT_CORE] = 6;	// parse_events_str only uses gpc_nums[]

			parse_events_str(L"{inst_spec,vfp_spec},ase_spec,dp_spec,ld_spec,{st_spec,br_immed_spec,crypto_spec}", events, groups, note, pmu_cfg);

			Assert::AreEqual(events[EVT_CORE].size(), (size_t)3);

			set_event_padding(ioctl_events, pmu_cfg, events, groups);

			Assert::AreEqual(ioctl_events[EVT_CORE].size(), (size_t)12);
		}

		TEST_METHOD(test_padding_gpc_nums_5__rf)
		{
			std::map<enum evt_class, std::vector<struct evt_noted>> ioctl_events;
			std::map<enum evt_class, std::deque<struct evt_noted>> events;
			std::map<enum evt_class, std::vector<struct evt_noted>> groups;
			std::wstring note;
			struct pmu_device_cfg pmu_cfg = { 0 };

			pmu_cfg.gpc_nums[EVT_CORE] = 5;	// parse_events_str only uses gpc_nums[]

			parse_events_str(L"{rf}", events, groups, note, pmu_cfg);

			Assert::AreEqual(events[EVT_CORE].size(), (size_t)1);

			set_event_padding(ioctl_events, pmu_cfg, events, groups);

			Assert::AreEqual(ioctl_events[EVT_CORE].size(), (size_t)1);

			size_t padding = std::count_if(ioctl_events[EVT_CORE].begin(), ioctl_events[EVT_CORE].end(), [](auto& e) { return e.type == EVT_PADDING; });
			size_t normal = std::count_if(ioctl_events[EVT_CORE].begin(), ioctl_events[EVT_CORE].end(), [](auto& e) { return e.type == EVT_NORMAL; });
			size_t grouped = std::count_if(ioctl_events[EVT_CORE].begin(), ioctl_events[EVT_CORE].end(), [](auto& e) { return e.type == EVT_GROUPED; });

			Assert::IsTrue(padding == 0);
			Assert::IsTrue(normal == 1);
			Assert::IsTrue(grouped == 0);
		}

		TEST_METHOD(test_padding_gpc_nums_6__rf)
		{
			std::map<enum evt_class, std::vector<struct evt_noted>> ioctl_events;
			std::map<enum evt_class, std::deque<struct evt_noted>> events;
			std::map<enum evt_class, std::vector<struct evt_noted>> groups;
			std::wstring note;
			struct pmu_device_cfg pmu_cfg = { 0 };

			pmu_cfg.gpc_nums[EVT_CORE] = 6;	// parse_events_str only uses gpc_nums[]

			parse_events_str(L"{rf}", events, groups, note, pmu_cfg);

			Assert::AreEqual(events[EVT_CORE].size(), (size_t)1);

			set_event_padding(ioctl_events, pmu_cfg, events, groups);

			Assert::AreEqual(ioctl_events[EVT_CORE].size(), (size_t)1);

			size_t padding = std::count_if(ioctl_events[EVT_CORE].begin(), ioctl_events[EVT_CORE].end(), [](auto& e) { return e.type == EVT_PADDING; });
			size_t normal = std::count_if(ioctl_events[EVT_CORE].begin(), ioctl_events[EVT_CORE].end(), [](auto& e) { return e.type == EVT_NORMAL; });
			size_t grouped = std::count_if(ioctl_events[EVT_CORE].begin(), ioctl_events[EVT_CORE].end(), [](auto& e) { return e.type == EVT_GROUPED; });

			Assert::IsTrue(padding == 0);
			Assert::IsTrue(normal == 1);
			Assert::IsTrue(grouped == 0);
		}

		TEST_METHOD(test_padding_gpc_nums_6__3_group_mix)
		{
			std::map<enum evt_class, std::vector<struct evt_noted>> ioctl_events;
			std::map<enum evt_class, std::deque<struct evt_noted>> events;
			std::map<enum evt_class, std::vector<struct evt_noted>> groups;
			std::wstring note;
			struct pmu_device_cfg pmu_cfg = { 0 };

			pmu_cfg.gpc_nums[EVT_CORE] = 6;	// parse_events_str only uses gpc_nums[]

			parse_events_str(L"{inst_spec,vfp_spec},{ase_spec,br_immed_spec},crypto_spec,{ld_spec,st_spec}", events, groups, note, pmu_cfg);

			Assert::AreEqual(events[EVT_CORE].size(), (size_t)1);

			set_event_padding(ioctl_events, pmu_cfg, events, groups);

			Assert::IsTrue((ioctl_events[EVT_CORE].size() % pmu_cfg.gpc_nums[EVT_CORE]) == 0);
			Assert::AreEqual(ioctl_events[EVT_CORE].size(), (size_t)12);

			size_t padding = std::count_if(ioctl_events[EVT_CORE].begin(), ioctl_events[EVT_CORE].end(), [](auto& e) { return e.type == EVT_PADDING; });
			size_t normal = std::count_if(ioctl_events[EVT_CORE].begin(), ioctl_events[EVT_CORE].end(), [](auto& e) { return e.type == EVT_NORMAL; });
			size_t grouped = std::count_if(ioctl_events[EVT_CORE].begin(), ioctl_events[EVT_CORE].end(), [](auto& e) { return e.type == EVT_GROUPED; });

			Assert::IsTrue(padding == 5);
			Assert::IsTrue(normal == 1);
			Assert::IsTrue(grouped == 6);
		}

		TEST_METHOD(test_padding_gpc_nums_5__topdown)
		{
			std::map<enum evt_class, std::vector<struct evt_noted>> ioctl_events;
			std::map<enum evt_class, std::deque<struct evt_noted>> events;
			std::map<enum evt_class, std::vector<struct evt_noted>> groups;
			std::wstring note;
			struct pmu_device_cfg pmu_cfg = { 0 };

			pmu_cfg.gpc_nums[EVT_CORE] = 5;	// parse_events_str only uses gpc_nums[]

			parse_events_str(L"{l1i_tlb_refill,l1i_tlb},{inst_spec,ld_spec},"
				"{st_spec,inst_spec},{inst_retired,br_mis_pred_retired},"
				"{l1d_tlb_refill,inst_retired},{itlb_walk,l1i_tlb},"
				"{inst_spec,dp_spec},{l2d_tlb_refill,inst_retired},"
				"{stall_backend,cpu_cycles},{crypto_spec,inst_spec},"
				"{l1i_tlb_refill,inst_retired},{ll_cache_rd,ll_cache_miss_rd},"
				"{inst_retired,cpu_cycles},{l2d_cache_refill,l2d_cache},"
				"{br_indirect_spec,inst_spec,br_immed_spec},{l2d_tlb_refill,l2d_tlb},"
				"{inst_retired,itlb_walk},{dtlb_walk,inst_retired},"
				"{l1i_cache,l1i_cache_refill},{l1d_cache_refill,l1d_cache},"
				"{br_mis_pred_retired,br_retired},{inst_retired,l2d_cache_refill},"
				"{vfp_spec,inst_spec},{stall_frontend,cpu_cycles},"
				"{inst_spec,ase_spec},{ll_cache_rd,ll_cache_miss_rd},"
				"{inst_retired,l1d_cache_refill},{l1d_tlb,dtlb_walk},"
				"{inst_retired,ll_cache_miss_rd},{l1i_cache_refill,inst_retired},"
				"{l1d_tlb_refill,l1d_tlb}", events, groups, note, pmu_cfg);

			Assert::AreEqual(events[EVT_CORE].size(), (size_t)0);

			set_event_padding(ioctl_events, pmu_cfg, events, groups);

			Assert::IsTrue((ioctl_events[EVT_CORE].size() % pmu_cfg.gpc_nums[EVT_CORE]) == 0);
			Assert::AreEqual(ioctl_events[EVT_CORE].size(), (size_t)80);
		}

		TEST_METHOD(test_padding_gpc_nums_6__topdown)
		{
			std::map<enum evt_class, std::vector<struct evt_noted>> ioctl_events;
			std::map<enum evt_class, std::deque<struct evt_noted>> events;
			std::map<enum evt_class, std::vector<struct evt_noted>> groups;
			std::wstring note;
			struct pmu_device_cfg pmu_cfg = { 0 };

			pmu_cfg.gpc_nums[EVT_CORE] = 6;	// parse_events_str only uses gpc_nums[]

			parse_events_str(L"{l1i_tlb_refill,l1i_tlb},{inst_spec,ld_spec},"
				"{st_spec,inst_spec},{inst_retired,br_mis_pred_retired},"
				"{l1d_tlb_refill,inst_retired},{itlb_walk,l1i_tlb},"
				"{inst_spec,dp_spec},{l2d_tlb_refill,inst_retired},"
				"{stall_backend,cpu_cycles},{crypto_spec,inst_spec},"
				"{l1i_tlb_refill,inst_retired},{ll_cache_rd,ll_cache_miss_rd},"
				"{inst_retired,cpu_cycles},{l2d_cache_refill,l2d_cache},"
				"{br_indirect_spec,inst_spec,br_immed_spec},{l2d_tlb_refill,l2d_tlb},"
				"{inst_retired,itlb_walk},{dtlb_walk,inst_retired},"
				"{l1i_cache,l1i_cache_refill},{l1d_cache_refill,l1d_cache},"
				"{br_mis_pred_retired,br_retired},{inst_retired,l2d_cache_refill},"
				"{vfp_spec,inst_spec},{stall_frontend,cpu_cycles},"
				"{inst_spec,ase_spec},{ll_cache_rd,ll_cache_miss_rd},"
				"{inst_retired,l1d_cache_refill},{l1d_tlb,dtlb_walk},"
				"{inst_retired,ll_cache_miss_rd},{l1i_cache_refill,inst_retired},"
				"{l1d_tlb_refill,l1d_tlb}", events, groups, note, pmu_cfg);

			Assert::AreEqual(events[EVT_CORE].size(), (size_t)0);

			set_event_padding(ioctl_events, pmu_cfg, events, groups);

			Assert::IsTrue((ioctl_events[EVT_CORE].size() % pmu_cfg.gpc_nums[EVT_CORE]) == 0);
			Assert::AreEqual(ioctl_events[EVT_CORE].size(), (size_t)66);
		}
	};
}
