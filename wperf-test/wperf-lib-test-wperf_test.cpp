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

#include <map>
#include "wperf-lib/wperf-lib.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace wperftest
{
	TEST_CLASS(wperflibtest_wperf_test)
	{

	// Disable these tests if we are not on ARM64 platform
#if defined(TEST_PLATFORM) && TEST_PLATFORM == x64

		BEGIN_TEST_METHOD_ATTRIBUTE(test_lib_check_wperf_test_types)
		TEST_IGNORE()
		END_TEST_METHOD_ATTRIBUTE()

#endif

	public:
		TEST_METHOD(test_lib_check_wperf_test_types)
		{
			std::map<std::wstring, RESULT_TYPE> types = {
				{ L"request.ioctl_events [EVT_CORE]", BOOL_RESULT },
				{ L"request.ioctl_events [EVT_DSU]", BOOL_RESULT },
				{ L"request.ioctl_events [EVT_DMC_CLK/EVT_DMC_CLKDIV2]", BOOL_RESULT },
				{ L"pmu_device.vendor_name", WSTRING_RESULT },
				{ L"pmu_device.product_name", WSTRING_RESULT },
				{ L"pmu_device.product_name(extended)", WSTRING_RESULT },
				{ L"pmu_device.events_query(events) [EVT_CORE]", NUM_RESULT },
				{ L"pmu_device.events_query(events) [EVT_DSU]", NUM_RESULT },
				{ L"pmu_device.events_query(events) [EVT_DMC_CLK]", NUM_RESULT },
				{ L"pmu_device.events_query(events) [EVT_DMC_CLKDIV2]", NUM_RESULT },
				{ L"PMU_CTL_QUERY_HW_CFG [arch_id]", NUM_RESULT },
				{ L"PMU_CTL_QUERY_HW_CFG [core_num]", NUM_RESULT },
				{ L"PMU_CTL_QUERY_HW_CFG [fpc_num]", NUM_RESULT },
				{ L"PMU_CTL_QUERY_HW_CFG [gpc_num]", NUM_RESULT },
				{ L"PMU_CTL_QUERY_HW_CFG [total_gpc_num]", NUM_RESULT },
				{ L"PMU_CTL_QUERY_HW_CFG [part_id]", NUM_RESULT },
				{ L"PMU_CTL_QUERY_HW_CFG [pmu_ver]", NUM_RESULT },
				{ L"PMU_CTL_QUERY_HW_CFG [rev_id]", NUM_RESULT },
				{ L"PMU_CTL_QUERY_HW_CFG [variant_id]", NUM_RESULT },
				{ L"PMU_CTL_QUERY_HW_CFG [vendor_id]", NUM_RESULT },
				{ L"PMU_CTL_QUERY_HW_CFG [midr_value]", NUM_RESULT },
				{ L"gpc_nums[EVT_CORE]", NUM_RESULT },
				{ L"gpc_nums[EVT_DSU]", NUM_RESULT },
				{ L"gpc_nums[EVT_DMC_CLK]", NUM_RESULT },
				{ L"gpc_nums[EVT_DMC_CLKDIV2]", NUM_RESULT },
				{ L"fpc_nums[EVT_CORE]", NUM_RESULT },
				{ L"fpc_nums[EVT_DSU]", NUM_RESULT },
				{ L"fpc_nums[EVT_DMC_CLK]", NUM_RESULT },
				{ L"fpc_nums[EVT_DMC_CLKDIV2]", NUM_RESULT },
				{ L"ioctl_events[EVT_CORE]", EVT_NOTE_RESULT },
			};

			Assert::IsTrue(wperf_init());
			uint16_t events[] = { 0x1b /*inst_spec*/, 0x73 /*dp_spec*/ };
			TEST_CONF test_conf = {
				/* events */
				sizeof(events)/sizeof(events[0]), events,
				/* metric events */
				0, nullptr,
			};
			TEST_INFO tinfo = {};
			if (wperf_test(&test_conf, NULL))
			{
				while (wperf_test(&test_conf, &tinfo))
				{
					if (types.find(tinfo.name) == types.end())
					{
						Assert::Fail();
					}
					Assert::IsTrue(tinfo.type == types[tinfo.name]);
				}
			}
			Assert::IsTrue(wperf_close());
		}
	};
}
