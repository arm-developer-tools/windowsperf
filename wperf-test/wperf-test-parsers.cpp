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

#include <algorithm>
#include <functional>
#include <sstream>
#include "pch.h"
#include "CppUnitTest.h"

#include "wperf/parsers.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace wperftest
{
	TEST_CLASS(wperftest_parsers)
	{
	public:
		TEST_METHOD(test_parse_events_str_for_sample_rf)
		{
			std::vector<struct evt_sample_src> ioctl_events_sample;
			std::map<uint32_t, uint32_t> sampling_inverval;

			parse_events_str_for_sample(L"rf", ioctl_events_sample, sampling_inverval);

			Assert::AreEqual(ioctl_events_sample.size(), (size_t)1);
			Assert::AreEqual(ioctl_events_sample[0].index, (uint32_t)0x0F);
			Assert::AreEqual(ioctl_events_sample[0].interval, PARSE_INTERVAL_DEFAULT);
		}

		TEST_METHOD(test_parse_events_str_for_sample_r1b)
		{
			std::vector<struct evt_sample_src> ioctl_events_sample;
			std::map<uint32_t, uint32_t> sampling_inverval;

			parse_events_str_for_sample(L"r1b", ioctl_events_sample, sampling_inverval);

			Assert::AreEqual(ioctl_events_sample.size(), (size_t)1);
			Assert::AreEqual(ioctl_events_sample[0].index, (uint32_t)0x1B);
			Assert::AreEqual(ioctl_events_sample[0].interval, PARSE_INTERVAL_DEFAULT);
		}

		TEST_METHOD(test_parse_events_str_for_sample_r12_rab)
		{
			std::vector<struct evt_sample_src> ioctl_events_sample;
			std::map<uint32_t, uint32_t> sampling_inverval;

			parse_events_str_for_sample(L"r12,rab", ioctl_events_sample, sampling_inverval);

			Assert::AreEqual(ioctl_events_sample.size(), (size_t)2);
			Assert::AreEqual(ioctl_events_sample[0].index, (uint32_t)0x12);
			Assert::AreEqual(ioctl_events_sample[0].interval, PARSE_INTERVAL_DEFAULT);

			Assert::AreEqual(ioctl_events_sample[1].index, (uint32_t)0xAB);
			Assert::AreEqual(ioctl_events_sample[1].interval, PARSE_INTERVAL_DEFAULT);
		}

		TEST_METHOD(test_parse_events_str_for_sample_r_7_events)
		{
			std::vector<struct evt_sample_src> ioctl_events_sample;
			std::map<uint32_t, uint32_t> sampling_interval;

			parse_events_str_for_sample(L"ra,r3c,rdab,rdead,r1a2b,rfedcb,r12345", ioctl_events_sample, sampling_interval);

			Assert::AreEqual(ioctl_events_sample.size(), (size_t)7);

			Assert::AreEqual(ioctl_events_sample[0].index, (uint32_t)0xA);
			Assert::AreEqual(ioctl_events_sample[0].interval, PARSE_INTERVAL_DEFAULT);

			Assert::AreEqual(ioctl_events_sample[1].index, (uint32_t)0x3C);
			Assert::AreEqual(ioctl_events_sample[1].interval, PARSE_INTERVAL_DEFAULT);

			Assert::AreEqual(ioctl_events_sample[2].index, (uint32_t)0xDAB);
			Assert::AreEqual(ioctl_events_sample[2].interval, PARSE_INTERVAL_DEFAULT);

			Assert::AreEqual(ioctl_events_sample[3].index, (uint32_t)0xDEAD);
			Assert::AreEqual(ioctl_events_sample[3].interval, PARSE_INTERVAL_DEFAULT);

			Assert::AreEqual(ioctl_events_sample[4].index, (uint32_t)0x1A2B);
			Assert::AreEqual(ioctl_events_sample[4].interval, PARSE_INTERVAL_DEFAULT);

			Assert::AreEqual(ioctl_events_sample[5].index, (uint32_t)0xFEDCB);
			Assert::AreEqual(ioctl_events_sample[5].interval, PARSE_INTERVAL_DEFAULT);

			Assert::AreEqual(ioctl_events_sample[6].index, (uint32_t)0x12345);
			Assert::AreEqual(ioctl_events_sample[6].interval, PARSE_INTERVAL_DEFAULT);
		}

		/****************************************************************************/

		TEST_METHOD(test_parse_events_str_EVT_CORE_rf)
		{
			std::map<enum evt_class, std::deque<struct evt_noted>> events;
			std::map<enum evt_class, std::vector<struct evt_noted>> groups;
			std::wstring note;
			struct pmu_device_cfg pmu_cfg = { 0 };

			pmu_cfg.gpc_nums[EVT_CORE] = 6;	// parse_events_str only uses gpc_nums[]

			parse_events_str(L"rf", events, groups, note, pmu_cfg);

			Assert::AreEqual(events[EVT_CORE].size(), (size_t)1);
			Assert::AreEqual(events[EVT_CORE][0].index, (uint16_t)0xF);
		}

		TEST_METHOD(test_parse_events_str_EVT_CORE_r1b)
		{
			std::map<enum evt_class, std::deque<struct evt_noted>> events;
			std::map<enum evt_class, std::vector<struct evt_noted>> groups;
			std::wstring note;
			struct pmu_device_cfg pmu_cfg = {0};

			pmu_cfg.gpc_nums[EVT_CORE] = 6;	// parse_events_str only uses gpc_nums[]

			parse_events_str(L"r1b", events, groups, note, pmu_cfg);

			Assert::AreEqual(events[EVT_CORE].size(), (size_t)1);
			Assert::AreEqual(events[EVT_CORE][0].index, (uint16_t)0x1B);
		}

		TEST_METHOD(test_parse_events_str_EVT_CORE_r_7_events)
		{
			std::map<enum evt_class, std::deque<struct evt_noted>> events;
			std::map<enum evt_class, std::vector<struct evt_noted>> groups;
			std::wstring note;
			struct pmu_device_cfg pmu_cfg = { 0 };

			pmu_cfg.gpc_nums[EVT_CORE] = 6;	// parse_events_str only uses gpc_nums[]

			parse_events_str(L"ra,r3c,rdab,rdead,r1a2b,rfedc,r1234", events, groups, note, pmu_cfg);

			Assert::AreEqual(events[EVT_CORE].size(), (size_t)7);

			Assert::AreEqual(events[EVT_CORE][0].index, (uint16_t)0xA);
			Assert::AreEqual(events[EVT_CORE][1].index, (uint16_t)0x3C);
			Assert::AreEqual(events[EVT_CORE][2].index, (uint16_t)0xDAB);
			Assert::AreEqual(events[EVT_CORE][3].index, (uint16_t)0xDEAD);
			Assert::AreEqual(events[EVT_CORE][4].index, (uint16_t)0x1A2B);
			Assert::AreEqual(events[EVT_CORE][5].index, (uint16_t)0xFEDC);
			Assert::AreEqual(events[EVT_CORE][6].index, (uint16_t)0x1234);
		}
	};
}
