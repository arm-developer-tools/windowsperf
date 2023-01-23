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
			std::vector<uint32_t> Output = {0, 1, 2, 4, 5, 6};
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
	};
}
