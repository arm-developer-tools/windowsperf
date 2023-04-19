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

#include <windows.h>
#include "wperf/events.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace wperftest
{
	TEST_CLASS(wperftest_events)
	{
	public:

		TEST_METHOD(test_get_core_event_index)
		{
			Assert::AreEqual(pmu_events::get_core_event_index(L"sw_incr"), 0x0);
			Assert::AreEqual(pmu_events::get_core_event_index(L"Sw_incr"), 0x0);
			Assert::AreEqual(pmu_events::get_core_event_index(L"SW_INCR"), 0x0);

			Assert::AreEqual(pmu_events::get_core_event_index(L"unaligned_ldst_retired"), 0x0F);
			Assert::AreEqual(pmu_events::get_core_event_index(L"unalIGNED_Ldst_retired"), 0x0F);
			Assert::AreEqual(pmu_events::get_core_event_index(L"UNALIGNED_LDST_RETIRED"), 0x0F);
		}

		TEST_METHOD(test_get_dmc_clk_event_index)
		{
			Assert::AreEqual(pmu_events::get_dmc_clk_event_index(L"cycle_count"), 0x000);
			Assert::AreEqual(pmu_events::get_dmc_clk_event_index(L"cYcle_Count"), 0x000);
			Assert::AreEqual(pmu_events::get_dmc_clk_event_index(L"CYCLE_COUNT"), 0x000);

			Assert::AreEqual(pmu_events::get_dmc_clk_event_index(L"request"), 0x01);
			Assert::AreEqual(pmu_events::get_dmc_clk_event_index(L"request"), 0x01);
			Assert::AreEqual(pmu_events::get_dmc_clk_event_index(L"REQUEST"), 0x01);

			Assert::AreEqual(pmu_events::get_dmc_clk_event_index(L"upload_stal"), 0x02);
			Assert::AreEqual(pmu_events::get_dmc_clk_event_index(L"Upload_Stal"), 0x02);
			Assert::AreEqual(pmu_events::get_dmc_clk_event_index(L"UPLOAD_STAL"), 0x02);
		}

		TEST_METHOD(test_get_dmc_clkdiv2_event_index)
		{
			Assert::AreEqual(pmu_events::get_dmc_clkdiv2_event_index(L"cycle_count"), 0x0);
			Assert::AreEqual(pmu_events::get_dmc_clkdiv2_event_index(L"Allocate"), 0x01);
			Assert::AreEqual(pmu_events::get_dmc_clkdiv2_event_index(L"QUEUE_DEPTH"), 0x02);
		}
	};
}
