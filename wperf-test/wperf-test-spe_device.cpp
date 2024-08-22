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

#include "wperf\spe_device.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace wperftest
{
	TEST_CLASS(wperftest_spe_device)
	{
	public:

		TEST_METHOD(test_spe_device_filter_name)
		{
			Assert::IsTrue(spe_device::is_filter_name(L"load_filter"));
			Assert::IsTrue(spe_device::is_filter_name(L"store_filter"));
			Assert::IsTrue(spe_device::is_filter_name(L"branch_filter"));
		}

		TEST_METHOD(test_spe_device_filter_name_as_alias)
		{
			Assert::IsTrue(spe_device::is_filter_name(L"ld"));
			Assert::IsTrue(spe_device::is_filter_name(L"st"));
			Assert::IsTrue(spe_device::is_filter_name(L"b"));
		}

		TEST_METHOD(test_spe_device_filter_name_is_alias)
		{
			Assert::IsTrue(spe_device::is_filter_name_alias (L"ld"));
			Assert::IsTrue(spe_device::is_filter_name_alias(L"st"));
			Assert::IsTrue(spe_device::is_filter_name_alias(L"b"));
		}
	};
}
