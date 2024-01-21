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

#include "pch.h"
#include "CppUnitTest.h"

#include <set>
#include "wperf-lib/wperf-lib.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace wperftest
{
	TEST_CLASS(wperflibtest_lib)
	{
	public:

	// Disable these tests if we are not on ARM64 platform
#if defined(TEST_PLATFORM) && TEST_PLATFORM == x64

		BEGIN_TEST_METHOD_ATTRIBUTE(test_lib_version)
		TEST_IGNORE()
		END_TEST_METHOD_ATTRIBUTE()

		BEGIN_TEST_METHOD_ATTRIBUTE(test_lib_list_events)
		TEST_IGNORE()
		END_TEST_METHOD_ATTRIBUTE()

		BEGIN_TEST_METHOD_ATTRIBUTE(test_lib_list_metrics)
		TEST_IGNORE()
		END_TEST_METHOD_ATTRIBUTE()

		BEGIN_TEST_METHOD_ATTRIBUTE(test_lib_stat)
		TEST_IGNORE()
		END_TEST_METHOD_ATTRIBUTE()

		BEGIN_TEST_METHOD_ATTRIBUTE(test_lib_num_cores)
		TEST_IGNORE()
		END_TEST_METHOD_ATTRIBUTE()

#endif

		TEST_METHOD(test_lib_version)
		{

			Assert::IsTrue(wperf_init());

			VERSION_INFO v;
			Assert::IsTrue(wperf_driver_version(&v));
			Assert::IsTrue(wperf_version(&v));

			Assert::IsTrue(wperf_close());
		}

		TEST_METHOD(test_lib_list_events)
		{
			Assert::IsTrue(wperf_init());

			LIST_CONF list_conf = { CORE_EVT /* Only list Core PMU events */ };
			EVENT_INFO einfo;
			Assert::IsTrue(wperf_list_events(&list_conf, NULL));

			int num_events_a = 0;
			Assert::IsTrue(wperf_list_num_events(&list_conf, &num_events_a));

			int num_events_b = 0;
			while (wperf_list_events(&list_conf, &einfo))
			{
				num_events_b++;
			}
			Assert::AreEqual(num_events_a, num_events_b);

			Assert::IsTrue(wperf_close());
		}

		TEST_METHOD(test_lib_list_metrics)
		{
			Assert::IsTrue(wperf_init());

			LIST_CONF list_conf = { CORE_EVT /* Only list Core PMU events */ };
			METRIC_INFO minfo;
			Assert::IsTrue(wperf_list_metrics(&list_conf, NULL));

			int num_metrics_a = 0;
			Assert::IsTrue(wperf_list_num_metrics(&list_conf, &num_metrics_a));

			int num_metrics_events_a = 0;
			Assert::IsTrue(wperf_list_num_metrics_events(&list_conf, &num_metrics_events_a));

			int num_metrics_events_b = 0;
			std::set<std::wstring> metrics;
			while (wperf_list_metrics(&list_conf, &minfo))
			{
				metrics.insert(minfo.metric_name);
				num_metrics_events_b++;
			}
			Assert::AreEqual(num_metrics_events_a, num_metrics_events_b);
			Assert::AreEqual(static_cast<size_t>(num_metrics_a), metrics.size());

			Assert::IsTrue(wperf_close());
		}

		TEST_METHOD(test_lib_stat)
		{
			Assert::IsTrue(wperf_init());
			uint8_t cores[2] = { 0, 3 };
			uint16_t events[2] = { 0x1B, 0x73 };
			int num_group_events[1] = { 2 };
			uint16_t group_events[2] = { 0x70, 0x71 };
			uint16_t* gevents[1] = { group_events };
			const wchar_t* metric_events[1] = { L"dcache" };
			STAT_CONF stat_conf =
			{
				2, // num_cores
				cores, // cores
				2, // num_events
				events, // events
				{1/*num_groups*/, num_group_events/*num_group_events*/, gevents/*events*/}, // group_events
				1, // num_metrics
				metric_events, // metric_events
				1, // duration
				false, // kernel_mode
				10 // period
			};

			Assert::IsTrue(wperf_stat(&stat_conf, NULL));

			STAT_INFO stat_info;
			std::set<uint8_t> list_cores;
			std::set<uint16_t> list_events;
			while (wperf_stat(&stat_conf, &stat_info))
			{
				list_cores.insert(stat_info.core_idx);
				list_events.insert(stat_info.event_idx);
			}
			Assert::AreEqual(static_cast<size_t>(2), list_cores.size());
			Assert::AreEqual(static_cast<size_t>(10), list_events.size());

			Assert::IsTrue(wperf_close());
		}

		TEST_METHOD(test_lib_num_cores)
		{
			Assert::IsTrue(wperf_init());

			int num_cores;
			Assert::IsTrue(wperf_num_cores(&num_cores));

			Assert::IsTrue(wperf_close());
		}
	};
}
