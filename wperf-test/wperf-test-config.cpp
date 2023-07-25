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

#include "wperf/config.h"
#include "wperf-common/macros.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace wperftest
{
	TEST_CLASS(wperftest_config)
	{

	public:

		TEST_METHOD(test_config_get_default_values)
		{
			LONG value;
			drvconfig::init();

			Assert::IsTrue(drvconfig::get(L"count.period", value));
			Assert::IsTrue(value == PMU_CTL_START_PERIOD);

			Assert::IsTrue(drvconfig::get(L"count.period_min", value));
			Assert::IsTrue(value == PMU_CTL_START_PERIOD_MIN);
		}

		TEST_METHOD(test_config_set_ro)
		{
			drvconfig::init();

			Assert::IsFalse(drvconfig::set(L"count.period_min", std::wstring(L"20")));
		}

		TEST_METHOD(test_config_set_value)
		{
			drvconfig::init();

			Assert::IsTrue(drvconfig::set(L"count.period", std::wstring(L"10")));
			Assert::IsTrue(drvconfig::set(L"count.period", std::wstring(L"50")));
			Assert::IsTrue(drvconfig::set(L"count.period", std::wstring(L"100")));

			Assert::IsTrue(drvconfig::set(L"count.period", std::wstring(L"0")));
			Assert::IsTrue(drvconfig::set(L"count.period", std::wstring(L"1")));
			Assert::IsTrue(drvconfig::set(L"count.period", std::wstring(L"2")));
			Assert::IsTrue(drvconfig::set(L"count.period", std::wstring(L"3")));
			Assert::IsTrue(drvconfig::set(L"count.period", std::wstring(L"5")));
			Assert::IsTrue(drvconfig::set(L"count.period", std::wstring(L"8")));
			Assert::IsTrue(drvconfig::set(L"count.period", std::wstring(L"13")));
			Assert::IsTrue(drvconfig::set(L"count.period", std::wstring(L"21")));
			Assert::IsTrue(drvconfig::set(L"count.period", std::wstring(L"34")));
			Assert::IsTrue(drvconfig::set(L"count.period", std::wstring(L"55")));
			Assert::IsTrue(drvconfig::set(L"count.period", std::wstring(L"89")));
			Assert::IsTrue(drvconfig::set(L"count.period", std::wstring(L"144")));
		}

		TEST_METHOD(test_config_set_config_str)
		{
			drvconfig::init();
		
			Assert::IsTrue(drvconfig::set(L"count.period=10"));
		}

		TEST_METHOD(test_config_set_config_str_nok)
		{
			drvconfig::init();

			Assert::IsFalse(drvconfig::set(L"count.period:10"));
			Assert::IsFalse(drvconfig::set(L"count.period="));
			Assert::IsFalse(drvconfig::set(L"="));
			Assert::IsFalse(drvconfig::set(L""));
		}

		TEST_METHOD(test_config_set_get_config_str)
		{
			LONG value;
			drvconfig::init();

			Assert::IsTrue(drvconfig::set(L"count.period=10"));
			Assert::IsTrue(drvconfig::get(L"count.period", value));
			Assert::IsTrue(value == 10);

			Assert::IsTrue(drvconfig::set(L"count.period=50"));
			Assert::IsTrue(drvconfig::get(L"count.period", value));
			Assert::IsTrue(value == 50);

			Assert::IsTrue(drvconfig::set(L"count.period=144"));
			Assert::IsTrue(drvconfig::get(L"count.period", value));
			Assert::IsTrue(value == 144);
		}

		TEST_METHOD(test_config_set_get)
		{
			LONG value;
			drvconfig::init();

			Assert::IsTrue(drvconfig::set(L"count.period", std::wstring(L"10")));			
			Assert::IsTrue(drvconfig::get(L"count.period", value));
			Assert::IsTrue(value == 10);

			Assert::IsTrue(drvconfig::set(L"count.period", std::wstring(L"50")));
			Assert::IsTrue(drvconfig::get(L"count.period", value));
			Assert::IsTrue(value == 50);

			Assert::IsTrue(drvconfig::set(L"count.period", std::wstring(L"144")));
			Assert::IsTrue(drvconfig::get(L"count.period", value));
			Assert::IsTrue(value == 144);
		}
	};
}