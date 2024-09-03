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

		TEST_METHOD(test_user_request_check_timeout_arg)
		{
			std::unordered_map<std::wstring, double> unit_map = { {L"s", 1}, { L"m", 60 }, {L"ms", 0.001}, {L"h", 3600}, {L"d" , 86400} };
			
			Assert::IsTrue(user_request::check_timeout_arg(std::wstring(L"1"), unit_map));
			Assert::IsTrue(user_request::check_timeout_arg(std::wstring(L"2s"), unit_map));
			Assert::IsTrue(user_request::check_timeout_arg(std::wstring(L"3m"), unit_map));
			Assert::IsTrue(user_request::check_timeout_arg(std::wstring(L"4ms"), unit_map));
			Assert::IsTrue(user_request::check_timeout_arg(std::wstring(L"5h"), unit_map));
			Assert::IsTrue(user_request::check_timeout_arg(std::wstring(L"6d"), unit_map));

			Assert::IsTrue(user_request::check_timeout_arg(std::wstring(L"0"), unit_map));
			Assert::IsFalse(user_request::check_timeout_arg(std::wstring(L""), unit_map));
			Assert::IsFalse(user_request::check_timeout_arg(std::wstring(L" "), unit_map));
			Assert::IsFalse(user_request::check_timeout_arg(std::wstring(L"-10"), unit_map));

			Assert::IsTrue(user_request::check_timeout_arg(std::wstring(L"10.01ms"), unit_map));
			Assert::IsTrue(user_request::check_timeout_arg(std::wstring(L"199.99m"), unit_map));
			Assert::IsTrue(user_request::check_timeout_arg(std::wstring(L"2.50h"), unit_map));
			Assert::IsTrue(user_request::check_timeout_arg(std::wstring(L"2d"), unit_map));
			Assert::IsTrue(user_request::check_timeout_arg(std::wstring(L"2000s"), unit_map));

			Assert::IsFalse(user_request::check_timeout_arg(std::wstring(L"3.2222s"), unit_map));
			Assert::IsFalse(user_request::check_timeout_arg(std::wstring(L".5"), unit_map));
			Assert::IsFalse(user_request::check_timeout_arg(std::wstring(L".d"), unit_map));
			Assert::IsFalse(user_request::check_timeout_arg(std::wstring(L".0002d"), unit_map));
			Assert::IsFalse(user_request::check_timeout_arg(std::wstring(L"2.2!d"), unit_map));
			Assert::IsFalse(user_request::check_timeout_arg(std::wstring(L"2d30m"), unit_map));
			Assert::IsFalse(user_request::check_timeout_arg(std::wstring(L"timeout=20seconds"), unit_map));
		}

		TEST_METHOD(test_convert_timeout_arg_to_seconds)
		{
			Assert::AreEqual(0.1, user_request::convert_timeout_arg_to_seconds(std::wstring(L"100ms"), std::wstring(L"--timeout")));
			Assert::AreEqual(120.0, user_request::convert_timeout_arg_to_seconds(std::wstring(L"2m"), std::wstring(L"--timeout")));
			Assert::AreEqual(199.99, user_request::convert_timeout_arg_to_seconds(std::wstring(L"199.99"), std::wstring(L"--timeout")));
			Assert::AreEqual(9000.0, user_request::convert_timeout_arg_to_seconds(std::wstring(L"2.50h"), std::wstring(L"--timeout")));
			Assert::AreEqual(172800.0, user_request::convert_timeout_arg_to_seconds(std::wstring(L"2d"), std::wstring(L"--timeout")));
			Assert::AreEqual(2000.0, user_request::convert_timeout_arg_to_seconds(std::wstring(L"2000s"), std::wstring(L"--timeout")));
		}

		TEST_METHOD(test_check_symbol_arg)
		{
			Assert::IsTrue(user_request::check_symbol_arg(L"xmul", L"xmul"));
			Assert::IsTrue(user_request::check_symbol_arg(L"xmul", L"^x"));
			Assert::IsTrue(user_request::check_symbol_arg(L"xmul", L"^xm"));
			//Assert::IsTrue(user_request::check_symbol_arg(L"xmul", L"^xmul"));

			//Assert::IsTrue(user_request::check_symbol_arg(L"xmul", L"xmul$"));
			Assert::IsTrue(user_request::check_symbol_arg(L"xmul", L"mul$"));
			Assert::IsTrue(user_request::check_symbol_arg(L"xmul", L"ul$"));
			Assert::IsTrue(user_request::check_symbol_arg(L"xmul", L"l$"));

			Assert::IsTrue(user_request::check_symbol_arg(L"xmul", L"^xmul$"));
		}

		TEST_METHOD(test_check_symbol_arg_nok)
		{
			Assert::IsFalse(user_request::check_symbol_arg(L"xmul", L"^"));
			Assert::IsFalse(user_request::check_symbol_arg(L"xmul", L"$"));

			Assert::IsFalse(user_request::check_symbol_arg(L"xmul", L"^a"));
			Assert::IsFalse(user_request::check_symbol_arg(L"xmul", L"^xmul_"));
			Assert::IsFalse(user_request::check_symbol_arg(L"xmul", L"^ x"));

			Assert::IsFalse(user_request::check_symbol_arg(L"xmul", L"a$"));
			Assert::IsFalse(user_request::check_symbol_arg(L"xmul", L"_xmul$"));
			Assert::IsFalse(user_request::check_symbol_arg(L"xmul", L"l $"));

			Assert::IsFalse(user_request::check_symbol_arg(L"xmul", L"^xmu$"));
			Assert::IsFalse(user_request::check_symbol_arg(L"xmul", L"^ xmul $"));
			Assert::IsFalse(user_request::check_symbol_arg(L"xmul", L"^xmu$"));
			Assert::IsFalse(user_request::check_symbol_arg(L"xmul", L"^mul$"));
		}
	};
}
