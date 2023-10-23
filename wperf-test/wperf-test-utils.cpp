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

#include "wperf/utils.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace wperftest
{
	TEST_CLASS(wperftest_utils)
	{
	public:

		TEST_METHOD(test_MultiByteFromWideString)
		{
			Assert::AreEqual(MultiByteFromWideString(L"core"), std::string("core"));
			Assert::AreEqual(MultiByteFromWideString(L"dsu"), std::string("dsu"));
			Assert::AreEqual(MultiByteFromWideString(L"/dsu/"), std::string("/dsu/"));
			Assert::AreEqual(MultiByteFromWideString(L"!@#$%^&*(){}:~\"<>?,./"), std::string("!@#$%^&*(){}:~\"<>?,./"));
			Assert::AreEqual(MultiByteFromWideString(L""), std::string());
			Assert::AreEqual(MultiByteFromWideString(0), std::string());
		}

		TEST_METHOD(test_IntToHexWideString)
		{
			Assert::AreEqual(IntToHexWideString(0, 4), std::wstring(L"0x0000"));
			Assert::AreEqual(IntToHexWideString(1, 4), std::wstring(L"0x0001"));
			Assert::AreEqual(IntToHexWideString(256, 4), std::wstring(L"0x0100"));
		}

		TEST_METHOD(test_IntToHexWideString_wchar_t)
		{
			wchar_t wchar = 0x123;
			Assert::AreEqual(IntToHexWideString(wchar, 4), std::wstring(L"0x0123"));
			Assert::AreEqual(IntToHexWideString(wchar, 10), std::wstring(L"0x0000000123"));

			uint16_t ui16 = 0x256;
			Assert::AreEqual(IntToHexWideString(ui16, 4), std::wstring(L"0x0256"));
			Assert::AreEqual(IntToHexWideString(ui16, 10), std::wstring(L"0x0000000256"));
		}

		TEST_METHOD(test_IntToHexWideStringNoPrefix)
		{
			Assert::AreEqual(IntToHexWideStringNoPrefix(0, 4), std::wstring(L"0000"));
			Assert::AreEqual(IntToHexWideStringNoPrefix(1, 4), std::wstring(L"0001"));
			Assert::AreEqual(IntToHexWideStringNoPrefix(256, 4), std::wstring(L"0100"));
		}

		TEST_METHOD(test_IntToHexWideStringNoPrefix_wchar_t)
		{
			wchar_t wchar = 0x123;
			Assert::AreEqual(IntToHexWideStringNoPrefix(wchar, 4), std::wstring(L"0123"));
			Assert::AreEqual(IntToHexWideStringNoPrefix(wchar, 10), std::wstring(L"0000000123"));

			uint16_t ui16 = 0x256;
			Assert::AreEqual(IntToHexWideStringNoPrefix(ui16, 4), std::wstring(L"0256"));
			Assert::AreEqual(IntToHexWideStringNoPrefix(ui16, 10), std::wstring(L"0000000256"));
		}


		TEST_METHOD(test_IntToDecWideString)
		{
			Assert::AreEqual(IntToDecWideString(0, 1), std::wstring(L"0"));
			Assert::AreEqual(IntToDecWideString(0, 2), std::wstring(L" 0"));
			Assert::AreEqual(IntToDecWideString(0, 3), std::wstring(L"  0"));
			Assert::AreEqual(IntToDecWideString(0, 4), std::wstring(L"   0"));
			Assert::AreEqual(IntToDecWideString(1, 1), std::wstring(L"1"));
			Assert::AreEqual(IntToDecWideString(1, 2), std::wstring(L" 1"));
			Assert::AreEqual(IntToDecWideString(1, 3), std::wstring(L"  1"));
			Assert::AreEqual(IntToDecWideString(1, 4), std::wstring(L"   1"));
			Assert::AreEqual(IntToDecWideString(1, 5), std::wstring(L"    1"));
			Assert::AreEqual(IntToDecWideString(256, 1), std::wstring(L"256"));
			Assert::AreEqual(IntToDecWideString(256, 2), std::wstring(L"256"));
			Assert::AreEqual(IntToDecWideString(256, 3), std::wstring(L"256"));
			Assert::AreEqual(IntToDecWideString(256, 4), std::wstring(L" 256"));
			Assert::AreEqual(IntToDecWideString(256, 5), std::wstring(L"  256"));
			Assert::AreEqual(IntToDecWideString(256, 6), std::wstring(L"   256"));
		}

		TEST_METHOD(test_DoubleToWideString)
		{
			Assert::AreEqual(DoubleToWideString(0.0, 2), std::wstring(L"0.00"));
			Assert::AreEqual(DoubleToWideString(1.1, 2), std::wstring(L"1.10"));
			Assert::AreEqual(DoubleToWideString(99.99, 2), std::wstring(L"99.99"));

			Assert::AreEqual(DoubleToWideString(0.0, 4), std::wstring(L"0.0000"));
			Assert::AreEqual(DoubleToWideString(1.1, 4), std::wstring(L"1.1000"));
			Assert::AreEqual(DoubleToWideString(99.99, 4), std::wstring(L"99.9900"));
		}

		TEST_METHOD(test_DoubleToWideStringExt)
		{
			Assert::AreEqual(DoubleToWideStringExt(0.0, 2, 6), std::wstring(L"  0.00"));
			Assert::AreEqual(DoubleToWideStringExt(1.1, 2, 6), std::wstring(L"  1.10"));
			Assert::AreEqual(DoubleToWideStringExt(99.99, 2, 6), std::wstring(L" 99.99"));

			Assert::AreEqual(DoubleToWideStringExt(0.0, 4, 8), std::wstring(L"  0.0000"));
			Assert::AreEqual(DoubleToWideStringExt(1.1, 4, 8), std::wstring(L"  1.1000"));
			Assert::AreEqual(DoubleToWideStringExt(99.99, 4, 8), std::wstring(L" 99.9900"));
		}

		TEST_METHOD(test_TokenizeWideStringOfInts_error)
		{
			std::vector<uint32_t> Output;
			Assert::IsFalse(TokenizeWideStringOfInts(L"a", L',', Output));
			Assert::IsFalse(TokenizeWideStringOfInts(L"0a", L',', Output));
			Assert::IsFalse(TokenizeWideStringOfInts(L"0,a", L',', Output));
			Assert::IsFalse(TokenizeWideStringOfInts(L"0,1,2a", L',', Output));
			Assert::IsFalse(TokenizeWideStringOfInts(L"0, 1, 2", L',', Output));
			Assert::IsFalse(TokenizeWideStringOfInts(L"0 , 1,2", L',', Output));
			Assert::IsFalse(TokenizeWideStringOfInts(L"0,1,2 ", L',', Output));
			Assert::IsFalse(TokenizeWideStringOfInts(L"0,-1,2", L',', Output));
		}

		TEST_METHOD(test_TokenizeWideStringOfInts_empty)
		{
			std::vector<uint32_t> Output;
			Assert::IsTrue(TokenizeWideStringOfInts(L"", L',', Output));
			Assert::AreEqual(Output.size(), (size_t)0);
		}

		TEST_METHOD(test_TokenizeWideStringOfInts_clear_output)
		{
			std::vector<uint32_t> Output = { 0, 1, 2, 4, 5, 6 };
			Assert::IsTrue(TokenizeWideStringOfInts(L"", L',', Output));
			Assert::AreEqual(Output.size(), (size_t)0);
		}

		TEST_METHOD(test_TokenizeWideStringOfInts_0)
		{
			std::vector<uint32_t> Output;
			Assert::IsTrue(TokenizeWideStringOfInts(L"0", L',', Output));
			Assert::AreEqual(Output.size(), (size_t)1);
			Assert::IsTrue(Output == std::vector<uint32_t>{0});
		}

		TEST_METHOD(test_TokenizeWideStringOfInts_012)
		{
			std::vector<uint32_t> Output;
			Assert::IsTrue(TokenizeWideStringOfInts(L"0,1,2", L',', Output));
			Assert::AreEqual(Output.size(), (size_t)3);
			Assert::IsTrue(Output == std::vector<uint32_t>{0, 1, 2});
		}

		TEST_METHOD(test_TokenizeWideStringOfInts_fibb)
		{
			std::vector<uint32_t> Output;
			Assert::IsTrue(TokenizeWideStringOfInts(L"0,1,1,2,3,5,8,13,21,34,55,89,144,233", L',', Output));
			Assert::AreEqual(Output.size(), (size_t)14);
			Assert::IsTrue(Output == std::vector<uint32_t>{0, 1, 1, 2, 3, 5, 8, 13, 21, 34, 55, 89, 144, 233});
		}

		TEST_METHOD(test_ReplaceFileExtension)
		{
			Assert::AreEqual(ReplaceFileExtension(std::wstring(L"filename"), std::wstring(L"xyz")), std::wstring(L"filename"));
			Assert::AreEqual(ReplaceFileExtension(std::wstring(L""), std::wstring(L"xyz")), std::wstring(L""));

			Assert::AreEqual(ReplaceFileExtension(std::wstring(L"filename.abc"), std::wstring(L"xyz")), std::wstring(L"filename.xyz"));
			Assert::AreEqual(ReplaceFileExtension(std::wstring(L"file.name.abc"), std::wstring(L"xyz")), std::wstring(L"file.name.xyz"));
			Assert::AreEqual(ReplaceFileExtension(std::wstring(L".file.name.abc"), std::wstring(L"xyz")), std::wstring(L".file.name.xyz"));
			Assert::AreEqual(ReplaceFileExtension(std::wstring(L".abc"), std::wstring(L"xyz")), std::wstring(L".xyz"));
		}

		TEST_METHOD(test_IntToDecWithCommas)
		{
			Assert::AreEqual(IntToDecWithCommas(1), std::wstring(L"1"));
			Assert::AreEqual(IntToDecWithCommas(12), std::wstring(L"12"));
			Assert::AreEqual(IntToDecWithCommas(123), std::wstring(L"123"));
			Assert::AreEqual(IntToDecWithCommas(1234), std::wstring(L"1,234"));
			Assert::AreEqual(IntToDecWithCommas(12345), std::wstring(L"12,345"));
			Assert::AreEqual(IntToDecWithCommas(123456), std::wstring(L"123,456"));
			Assert::AreEqual(IntToDecWithCommas(1234567), std::wstring(L"1,234,567"));
			Assert::AreEqual(IntToDecWithCommas(12345678), std::wstring(L"12,345,678"));
			Assert::AreEqual(IntToDecWithCommas(123456789), std::wstring(L"123,456,789"));
			Assert::AreEqual(IntToDecWithCommas(12345678910), std::wstring(L"12,345,678,910"));
			Assert::AreEqual(IntToDecWithCommas(1234567891011), std::wstring(L"1,234,567,891,011"));
			Assert::AreEqual(IntToDecWithCommas(123456789101112), std::wstring(L"123,456,789,101,112"));
			Assert::AreEqual(IntToDecWithCommas(12345678910111213), std::wstring(L"12,345,678,910,111,213"));
			Assert::AreEqual(IntToDecWithCommas(1234567891011121314), std::wstring(L"1,234,567,891,011,121,314"));
		}

		TEST_METHOD(test_IntToDecWithCommas_neg)
		{
			Assert::AreEqual(IntToDecWithCommas(-1), std::wstring(L"-1"));
			Assert::AreEqual(IntToDecWithCommas(-12), std::wstring(L"-12"));
			Assert::AreEqual(IntToDecWithCommas(-123), std::wstring(L"-123"));
			Assert::AreEqual(IntToDecWithCommas(-1234), std::wstring(L"-1,234"));
			Assert::AreEqual(IntToDecWithCommas(-12345), std::wstring(L"-12,345"));
			Assert::AreEqual(IntToDecWithCommas(-123456), std::wstring(L"-123,456"));
			Assert::AreEqual(IntToDecWithCommas(-1234567), std::wstring(L"-1,234,567"));
			Assert::AreEqual(IntToDecWithCommas(-12345678), std::wstring(L"-12,345,678"));
			Assert::AreEqual(IntToDecWithCommas(-123456789), std::wstring(L"-123,456,789"));
			Assert::AreEqual(IntToDecWithCommas(-12345678910), std::wstring(L"-12,345,678,910"));
			Assert::AreEqual(IntToDecWithCommas(-1234567891011), std::wstring(L"-1,234,567,891,011"));
			Assert::AreEqual(IntToDecWithCommas(-123456789101112), std::wstring(L"-123,456,789,101,112"));
			Assert::AreEqual(IntToDecWithCommas(-12345678910111213), std::wstring(L"-12,345,678,910,111,213"));
			Assert::AreEqual(IntToDecWithCommas(-1234567891011121314), std::wstring(L"-1,234,567,891,011,121,314"));
		}

		TEST_METHOD(test_IntToDecWithCommas_Ramanujan_Tau)
		{
			Assert::AreEqual(IntToDecWithCommas(1), std::wstring(L"1"));
			Assert::AreEqual(IntToDecWithCommas(-24), std::wstring(L"-24"));
			Assert::AreEqual(IntToDecWithCommas(252), std::wstring(L"252"));
			Assert::AreEqual(IntToDecWithCommas(-1472), std::wstring(L"-1,472"));
			Assert::AreEqual(IntToDecWithCommas(4830), std::wstring(L"4,830"));
			Assert::AreEqual(IntToDecWithCommas(-6048), std::wstring(L"-6,048"));
			Assert::AreEqual(IntToDecWithCommas(-16744), std::wstring(L"-16,744"));
			Assert::AreEqual(IntToDecWithCommas(84480), std::wstring(L"84,480"));
			Assert::AreEqual(IntToDecWithCommas(-113643), std::wstring(L"-113,643"));
			Assert::AreEqual(IntToDecWithCommas(-115920), std::wstring(L"-115,920"));
			Assert::AreEqual(IntToDecWithCommas(534612), std::wstring(L"534,612"));
			Assert::AreEqual(IntToDecWithCommas(-370944), std::wstring(L"-370,944"));
			Assert::AreEqual(IntToDecWithCommas(-577738), std::wstring(L"-577,738"));
			Assert::AreEqual(IntToDecWithCommas(401856), std::wstring(L"401,856"));
			Assert::AreEqual(IntToDecWithCommas(1217160), std::wstring(L"1,217,160"));
			Assert::AreEqual(IntToDecWithCommas(987136), std::wstring(L"987,136"));
			Assert::AreEqual(IntToDecWithCommas(-6905934), std::wstring(L"-6,905,934"));
			Assert::AreEqual(IntToDecWithCommas(2727432), std::wstring(L"2,727,432"));
			Assert::AreEqual(IntToDecWithCommas(10661420), std::wstring(L"10,661,420"));
			Assert::AreEqual(IntToDecWithCommas(-7109760), std::wstring(L"-7,109,760"));
		}

		TEST_METHOD(test_CaseInsensitiveWStringComparision)
		{
			Assert::IsTrue(CaseInsensitiveWStringComparision(std::wstring(L""), std::wstring(L"")));
			Assert::IsTrue(CaseInsensitiveWStringComparision(std::wstring(L",./"), std::wstring(L",./")));
			Assert::IsTrue(CaseInsensitiveWStringComparision(std::wstring(L"1234567890"), std::wstring(L"1234567890")));
			Assert::IsTrue(CaseInsensitiveWStringComparision(std::wstring(L"!@#$%^&*()_+"), std::wstring(L"!@#$%^&*()_+")));
			Assert::IsTrue(CaseInsensitiveWStringComparision(std::wstring(L"!@#$%^&*()_+a"), std::wstring(L"!@#$%^&*()_+A")));
			Assert::IsTrue(CaseInsensitiveWStringComparision(std::wstring(L"0x5f3759df"), std::wstring(L"0X5F3759DF")));
			Assert::IsTrue(CaseInsensitiveWStringComparision(std::wstring(L"   0x5f3759df"), std::wstring(L"   0X5F3759DF")));
			Assert::IsTrue(CaseInsensitiveWStringComparision(std::wstring(L"0x5f3759df   "), std::wstring(L"0X5F3759DF   ")));
			Assert::IsTrue(CaseInsensitiveWStringComparision(std::wstring(L"   0x5f3759df   "), std::wstring(L"   0X5F3759DF   ")));
			Assert::IsTrue(CaseInsensitiveWStringComparision(std::wstring(L"text"), std::wstring(L"TEXT")));
			Assert::IsTrue(CaseInsensitiveWStringComparision(std::wstring(L"x"), std::wstring(L"X")));
			Assert::IsTrue(CaseInsensitiveWStringComparision(std::wstring(L"It takes a touch of genius -- and a lot of courage -- to move in the opposite direction"),
				std::wstring(L"It takes a touch of genius -- and a lot of courage -- to move in the opposite direction")));
			Assert::IsTrue(CaseInsensitiveWStringComparision(std::wstring(L"Everything should be made as simple as possible, but not simpler."),
				std::wstring(L"Everything should be maDE AS SIMple as possIBLE, BUT NOt simpler.")));
		}

		TEST_METHOD(test_WStringToLower)
		{
			Assert::AreEqual(WStringToLower(L""), std::wstring(L""));
			Assert::AreEqual(WStringToLower(L"X"), std::wstring(L"x"));
			Assert::AreEqual(WStringToLower(L"/DSU/"), std::wstring(L"/dsu/"));
			Assert::AreEqual(WStringToLower(L"/core/"), std::wstring(L"/core/"));
			Assert::AreEqual(WStringToLower(L"The hardest thing in the world to understand is the income tax."),
				std::wstring(L"the hardest thing in the world to understand is the income tax."));
		}

		TEST_METHOD(test_WStringStartsWith)
		{
			Assert::IsTrue(WStringStartsWith(std::wstring(L"/dsu/l3d_cache"), std::wstring(L"")));
			Assert::IsTrue(WStringStartsWith(std::wstring(L"/dsu/l3d_cache"), std::wstring(L"/dsu/")));
			Assert::IsTrue(WStringStartsWith(std::wstring(L"/dsu/l3d_cache_refill"), std::wstring(L"/dsu/")));
			Assert::IsTrue(WStringStartsWith(std::wstring(L"/dmc_clkdiv2/rdwr"), std::wstring(L"/dmc_clkdiv2/")));
		}

		TEST_METHOD(test_CaseInsensitiveWStringStartsWith)
		{
			Assert::IsTrue(CaseInsensitiveWStringStartsWith(std::wstring(L"/DSU/l3d_cache"), std::wstring(L"")));
			Assert::IsTrue(CaseInsensitiveWStringStartsWith(std::wstring(L"/DSU/l3d_cache"), std::wstring(L"/dsu/")));
			Assert::IsTrue(CaseInsensitiveWStringStartsWith(std::wstring(L"/DMC_CLKDIV2/rdwr"), std::wstring(L"/dmc_clkdiv2/")));
		}

		TEST_METHOD(test_ReplaceTokenInWstring_ok)
		{
			std::string filename = "{aaa} {bbb} {ccc} {ddd} {eee}";

			Assert::IsTrue(ReplaceTokenInString(filename, "{aaa}", "111"));
			Assert::IsTrue(ReplaceTokenInString(filename, "{bbb}", "222"));
			Assert::IsTrue(ReplaceTokenInString(filename, "{ccc}", "333"));
			Assert::IsTrue(ReplaceTokenInString(filename, "{ddd}", "444"));
			Assert::IsTrue(ReplaceTokenInString(filename, "{eee}", "555"));
			Assert::AreEqual(filename, std::string("111 222 333 444 555"));
		}

		TEST_METHOD(test_ReplaceTokenInWstring_nok)
		{
			std::string filename = "{aaa} {bbb} {ccc} {ddd} {eee}";

			Assert::IsFalse(ReplaceTokenInString(filename, "{aaa ", "111"));
			Assert::IsFalse(ReplaceTokenInString(filename, "{bb}", "222"));
			Assert::IsFalse(ReplaceTokenInString(filename, " ccc}", "333"));
			Assert::IsFalse(ReplaceTokenInString(filename, " ddd ", "444"));
			Assert::IsFalse(ReplaceTokenInString(filename, "}eee{", "555"));
		}

		TEST_METHOD(test_ReplaceTokenInWstring_ttimestamp_class_ok)
		{
			std::string filename = "wperf_system_side_{timestamp}.{class}.csv";

			Assert::IsTrue(ReplaceTokenInString(filename, "{timestamp}", "2023_09_21_09_42_59"));
			Assert::IsTrue(ReplaceTokenInString(filename, "{class}", "core"));
			Assert::AreEqual(filename, std::string("wperf_system_side_2023_09_21_09_42_59.core.csv"));
		}

		TEST_METHOD(test_ReplaceTokenInWstring_ttimestamp_class_nok)
		{
			std::string filename = "wperf_system_side_{timestamp}.{class}.csv";

			Assert::IsFalse(ReplaceTokenInString(filename, "{foo}", "2023_09_21_09_42_59"));
			Assert::IsFalse(ReplaceTokenInString(filename, "{bar}", "core"));
			Assert::IsFalse(ReplaceTokenInString(filename, "{baz}", "text is here"));
			Assert::IsFalse(ReplaceTokenInString(filename, "{TIMESTAMP}", "text is here"));
			Assert::IsFalse(ReplaceTokenInString(filename, "{CLASS}", "text is here"));
		}
	};
}
