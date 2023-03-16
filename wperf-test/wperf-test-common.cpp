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
#include <numeric>
#include <windows.h>
#include "wperf-common\inline.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace wperftest_common
{
	TEST_CLASS(wperftest_common_iorequest)
	{
	public:

		TEST_METHOD(test_check_cores_in_pmu_ctl_hdr_p_null)
		{
			Assert::IsFalse(check_cores_in_pmu_ctl_hdr_p(0));
		}

		TEST_METHOD(test_check_cores_in_pmu_ctl_hdr_p_cores_count)
		{
			struct pmu_ctl_hdr ctl_req;
			ctl_req.cores_idx.cores_count = MAX_PMU_CTL_CORES_COUNT;
			std::fill_n(ctl_req.cores_idx.cores_no, MAX_PMU_CTL_CORES_COUNT, 0);

			Assert::IsFalse(check_cores_in_pmu_ctl_hdr_p(&ctl_req));
		}

		TEST_METHOD(test_check_cores_in_pmu_ctl_hdr_p_cores_no_error)
		{
			struct pmu_ctl_hdr ctl_req;
			ctl_req.cores_idx.cores_count = 8;	// Arbitrary value
			std::fill_n(ctl_req.cores_idx.cores_no, MAX_PMU_CTL_CORES_COUNT, MAX_PMU_CTL_CORES_COUNT);

			Assert::IsFalse(check_cores_in_pmu_ctl_hdr_p(&ctl_req));
		}

		TEST_METHOD(test_check_cores_in_pmu_ctl_hdr_p_cores_no)
		{
			for (size_t cores_count = 0; cores_count < MAX_PMU_CTL_CORES_COUNT; cores_count++)
			{
				struct pmu_ctl_hdr ctl_req;
				ctl_req.cores_idx.cores_count = cores_count;
				std::iota(ctl_req.cores_idx.cores_no,
					ctl_req.cores_idx.cores_no + MAX_PMU_CTL_CORES_COUNT, 0);

				Assert::IsTrue(check_cores_in_pmu_ctl_hdr_p(&ctl_req));
			}
		}
	};
}
