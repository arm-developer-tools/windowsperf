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

#include "wperf\user_request.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

/*
bool user_request::is_cli_option_in_args(const wstr_vec& raw_args, std::wstring opt)
{
	// We will search only before double-dash is present as after "--" WindowsPerf CLI options end!
	auto last_iter = std::find_if(raw_args.begin(), raw_args.end(),
		[](const auto& arg) { return arg == std::wstring(L"--"); });
	return std::count_if(raw_args.begin(), last_iter,
		[opt](const auto& arg) { return arg == opt; });
}

bool user_request::is_force_lock(const wstr_vec& raw_args)
{
	return is_cli_option_in_args(raw_args, std::wstring(L"--force-lock"));
}
*/

namespace wperftest
{
	TEST_CLASS(wperftest_user_request)
	{
	public:

		TEST_METHOD(test_user_request_is_cli_option_in_args_1)
		{
			wstr_vec raw_args = { L"--force-lock" };
			Assert::IsTrue(user_request::is_cli_option_in_args(raw_args, L"--force-lock"));
			Assert::IsFalse(user_request::is_cli_option_in_args(raw_args, L"--"));
			Assert::IsFalse(user_request::is_cli_option_in_args(raw_args, L"--annotate"));
			Assert::IsFalse(user_request::is_cli_option_in_args(raw_args, L"wperf"));
			Assert::IsFalse(user_request::is_cli_option_in_args(raw_args, L"stat"));
		}

		TEST_METHOD(test_user_request_is_cli_option_in_args_2)
		{
			wstr_vec raw_args = { L"wperf", L"stat", L"-m", L"imix", L"-c", L"1", L"--force-lock" };
			Assert::IsTrue(user_request::is_cli_option_in_args(raw_args, L"--force-lock"));
			Assert::IsFalse(user_request::is_cli_option_in_args(raw_args, L"--"));
			Assert::IsFalse(user_request::is_cli_option_in_args(raw_args, L"--annotate"));
			Assert::IsTrue(user_request::is_cli_option_in_args(raw_args, L"wperf"));
			Assert::IsTrue(user_request::is_cli_option_in_args(raw_args, L"stat"));
		}

		TEST_METHOD(test_user_request_is_cli_option_in_args_3)
		{
			wstr_vec raw_args = { L"wperf", L"stat", L"--force-lock", L"--"};
			Assert::IsTrue(user_request::is_cli_option_in_args(raw_args, L"--force-lock"));
			Assert::IsFalse(user_request::is_cli_option_in_args(raw_args, L"--"));
			Assert::IsFalse(user_request::is_cli_option_in_args(raw_args, L"--annotate"));
			Assert::IsTrue(user_request::is_cli_option_in_args(raw_args, L"wperf"));
			Assert::IsTrue(user_request::is_cli_option_in_args(raw_args, L"stat"));
		}

		TEST_METHOD(test_user_request_is_cli_option_in_args_4)
		{
			wstr_vec raw_args = { L"wperf", L"stat", L"--", L"--force-lock" };
			Assert::IsFalse(user_request::is_cli_option_in_args(raw_args, L"--force-lock"));
			Assert::IsFalse(user_request::is_cli_option_in_args(raw_args, L"--"));
			Assert::IsFalse(user_request::is_cli_option_in_args(raw_args, L"--annotate"));
			Assert::IsTrue(user_request::is_cli_option_in_args(raw_args, L"wperf"));
			Assert::IsTrue(user_request::is_cli_option_in_args(raw_args, L"stat"));
		}

		TEST_METHOD(test_user_request_is_force_lock_1)
		{
			wstr_vec raw_args = { L"wperf", L"stat", L"--", L"--force-lock" };
			Assert::IsFalse(user_request::is_force_lock(raw_args));
		}

		TEST_METHOD(test_user_request_is_force_lock_2)
		{
			wstr_vec raw_args = { L"wperf", L"stat", L"--force-lock", L"--" };
			Assert::IsTrue(user_request::is_force_lock(raw_args));
		}

		TEST_METHOD(test_user_request_is_force_lock_3)
		{
			wstr_vec raw_args = { L"wperf", L"stat", L"-m", L"imix", L"-c", L"1", L"--force-lock" };
			Assert::IsTrue(user_request::is_force_lock(raw_args));
		}
	};
}
