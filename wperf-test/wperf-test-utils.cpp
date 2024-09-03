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

#include "wperf/utils.h"

#include <iostream>
#include <string>
#include <unordered_map>
#include <numeric>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace wperftest
{
	TEST_CLASS(wperftest_utils)
	{
	public:

		TEST_METHOD(test_GetFullFilePath)
		{
			Assert::AreEqual(GetFullFilePath(std::wstring(L"c:\\Workspace"),
				std::wstring(L"filename.csv")),
				std::wstring(L"c:\\Workspace\\filename.csv"));
			Assert::AreEqual(GetFullFilePath(std::wstring(L"c:\\Program Files (x86)\\InstallShield Installation Information\\{04201224-2B34-4EE7-862B-B7BBF89DB3AB}"),
				std::wstring(L"setup.exe")),
				std::wstring(L"c:\\Program Files (x86)\\InstallShield Installation Information\\{04201224-2B34-4EE7-862B-B7BBF89DB3AB}\\setup.exe"));
		}

		TEST_METHOD(test_WStringJoin_corner_empty_vect)
		{
			{
				std::vector<std::wstring> vect = {  };
				Assert::AreEqual(WStringJoin(vect, L""), std::wstring(L""));
			}

			{
				std::vector<std::wstring> vect = {  };
				Assert::AreEqual(WStringJoin(vect, L","), std::wstring(L""));
			}
		}

		TEST_METHOD(test_WStringJoin_corner_empty_sep)
		{
			{
				std::vector<std::wstring> vect = { L"abc", L"DEF" };
				Assert::AreEqual(WStringJoin(vect, L""), std::wstring(L"abcDEF"));
			}

			{
				std::vector<std::wstring> vect = { L"", L"b", L"", L"c", L"d" };
				Assert::AreEqual(WStringJoin(vect, L""), std::wstring(L"bcd"));
			}
		}

		TEST_METHOD(test_WStringJoin_corner_vect_1)
		{
			{
				std::vector<std::wstring> vect = { L"abc" };
				Assert::AreEqual(WStringJoin(vect, L""), std::wstring(L"abc"));
			}

			{
				std::vector<std::wstring> vect = { L"DEF" };
				Assert::AreEqual(WStringJoin(vect, L","), std::wstring(L"DEF"));
			}

			{
				std::vector<std::wstring> vect = { L"ghi" };
				Assert::AreEqual(WStringJoin(vect, L"    "), std::wstring(L"ghi"));
			}
		}

		TEST_METHOD(test_WStringJoin_comma)
		{
			{
				std::vector<std::wstring> vect = { L"a", L"b" };
				Assert::AreEqual(WStringJoin(vect, L", "), std::wstring(L"a, b"));
			}

			{
				std::vector<std::wstring> vect = { L"a", L"b", L"b", L"c", L"d" };
				Assert::AreEqual(WStringJoin(vect, L";"), std::wstring(L"a;b;b;c;d"));
			}

			{
				std::vector<std::wstring> vect = { L"load_filter", L"store_filter", L"branch_filter", L"ld", L"st", L"b" };
				Assert::AreEqual(WStringJoin(vect, L","), std::wstring(L"load_filter,store_filter,branch_filter,ld,st,b"));
			}
		}

		TEST_METHOD(test_TrimWideString)
		{
			Assert::AreEqual(std::wstring(L"abc"), TrimWideString(L"    abc"));
			Assert::AreEqual(std::wstring(L"abc"), TrimWideString(L"    abc     "));
			Assert::AreEqual(std::wstring(L"abc"), TrimWideString(L"abc      "));
			Assert::AreEqual(std::wstring(L""), TrimWideString(L""));
			Assert::AreEqual(std::wstring(L"abc def"), TrimWideString(L"    abc def    "));
			Assert::AreEqual(std::wstring(L"abc def"), TrimWideString(L"    abc def"));
		}

		TEST_METHOD(test_MultiByteFromWideString)
		{
			Assert::AreEqual(MultiByteFromWideString(L"core"), std::string("core"));
			Assert::AreEqual(MultiByteFromWideString(L"dsu"), std::string("dsu"));
			Assert::AreEqual(MultiByteFromWideString(L"/dsu/"), std::string("/dsu/"));
			Assert::AreEqual(MultiByteFromWideString(L"!@#$%^&*(){}:~\"<>?,./"), std::string("!@#$%^&*(){}:~\"<>?,./"));
			Assert::AreEqual(MultiByteFromWideString(L""), std::string());
			Assert::AreEqual(MultiByteFromWideString(0), std::string());
		}

		TEST_METHOD(test_WideStringFromMultiByte)
		{
			Assert::AreEqual(WideStringFromMultiByte("core"), std::wstring(L"core"));
			Assert::AreEqual(WideStringFromMultiByte("dsu"), std::wstring(L"dsu"));
			Assert::AreEqual(WideStringFromMultiByte("/dsu/"), std::wstring(L"/dsu/"));
			Assert::AreEqual(WideStringFromMultiByte("!@#$%^&*(){}:~\"<>?,./"), std::wstring(L"!@#$%^&*(){}:~\"<>?,./"));
			Assert::AreEqual(WideStringFromMultiByte(""), std::wstring());
			Assert::AreEqual(WideStringFromMultiByte(0), std::wstring());
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

		TEST_METHOD(test_TokenizeWideStringOfStrings_empty)
		{
			{
				std::vector<std::wstring> Output;
				TokenizeWideStringOfStrings(L"", L',', Output);
				Assert::AreEqual(static_cast<std::vector<std::wstring>::size_type>(0), Output.size());
			}

			{
				std::vector<std::wstring> Output;
				TokenizeWideStringOfStrings(L" ", L' ', Output);
				Assert::AreEqual(static_cast<std::vector<std::wstring>::size_type>(0), Output.size());
			}

			{
				std::vector<std::wstring> Output;
				TokenizeWideStringOfStrings(L"                  ", L' ', Output);
				Assert::AreEqual(static_cast<std::vector<std::wstring>::size_type>(0), Output.size());
			}

			{
				std::vector<std::wstring> Output;
				TokenizeWideStringOfStrings(L"////", L'/', Output);
				Assert::AreEqual(static_cast<std::vector<std::wstring>::size_type>(0), Output.size());
			}
		}

		void tokenizeWideStringOfStrings_test_helper(const std::wstring& str, const std::vector<std::wstring>& arr, const wchar_t delim)
		{
			std::vector<std::wstring> Output;
			std::vector<std::wstring> Reference = arr;
			TokenizeWideStringOfStrings(str, delim, Output);
			Assert::AreEqual(Reference.size(), Output.size(), std::wstring(L"Output and Reference differ in size. (" + str + L")").c_str());
			for (auto i = 0; i < Reference.size(); i++)
			{
				Assert::IsTrue(Output[i] == Reference[i],
					std::wstring(L"Got (" + Output[i] + L") expected (" + Reference[i] + L")").c_str());
			}
		}

		TEST_METHOD(test_TokenizeWideStringOfStrings)
		{
			tokenizeWideStringOfStrings_test_helper(L"a b c", { L"a", L"b", L"c" }, L' ');
			tokenizeWideStringOfStrings_test_helper(L"            a b                c           ", { L"a", L"b", L"c" }, L' ');
			tokenizeWideStringOfStrings_test_helper(L"a b c def         ", { L"a", L"b", L"c", L"def"}, L' ');
			tokenizeWideStringOfStrings_test_helper(L"                            b                           ", { L"b" }, L' ');
			tokenizeWideStringOfStrings_test_helper(L"a b/c d e", { L"a", L"b/c", L"d", L"e"}, L' ');
			tokenizeWideStringOfStrings_test_helper(L"a b c", { L"a b c" }, L'*');
			tokenizeWideStringOfStrings_test_helper(L"a*b*c", { L"a", L"b", L"c" }, L'*');
			tokenizeWideStringOfStrings_test_helper(L"***************a b**********c***", { L"a b", L"c" }, L'*');
			tokenizeWideStringOfStrings_test_helper(L"a*b*c*def***********", { L"a", L"b", L"c", L"def" }, L'*');
			tokenizeWideStringOfStrings_test_helper(L"***************************b************************", { L"b" }, L'*');
			tokenizeWideStringOfStrings_test_helper(L"a*b/c*d*e", { L"a", L"b/c", L"d", L"e" }, L'*');
		
		}


		std::vector<uint32_t> generateSequence(const uint32_t& start, const uint32_t& end)
		{
			std::vector<uint32_t> sequence(end - start + 1);
			std::iota(sequence.begin(), sequence.end(), start);
			return sequence;
		}


		void tokenizeWideStringofInts_test_helper(const std::wstring& input, const std::vector<uint32_t>& expectedOutput, bool expectedResult)
		{
			std::vector<uint32_t> Output;
			bool result = TokenizeWideStringOfInts(input, L',', Output);

			Assert::AreEqual(result, expectedResult);
			if (expectedResult) {
				Assert::AreEqual(Output.size(), expectedOutput.size());
				Assert::IsTrue(Output == expectedOutput);
			}
		}

		TEST_METHOD(test_TokenizeWideStringOfInts)
		{
			// Test unexpected inputs
			tokenizeWideStringofInts_test_helper(L"a", {}, false);
			tokenizeWideStringofInts_test_helper(L"0a", {}, false);
			tokenizeWideStringofInts_test_helper(L"0,a", {}, false);
			tokenizeWideStringofInts_test_helper(L"0,1,2a", {}, false);
			tokenizeWideStringofInts_test_helper(L"0, 1, 2", {}, false);
			tokenizeWideStringofInts_test_helper(L"0 , 1,2", {}, false);
			tokenizeWideStringofInts_test_helper(L"0,1,2 ", {}, false);
			tokenizeWideStringofInts_test_helper(L"0,-1,2", {}, false);

			// Test correct cases
			tokenizeWideStringofInts_test_helper(L"", {}, true);
			tokenizeWideStringofInts_test_helper(L"0", generateSequence(0, 0), true);
			tokenizeWideStringofInts_test_helper(L"0,1,2", generateSequence(0, 2), true);
			tokenizeWideStringofInts_test_helper(L"5-10,11-15,16-20", generateSequence(5, 20), true);
			tokenizeWideStringofInts_test_helper(L"30-20", generateSequence(20, 30), true);
			tokenizeWideStringofInts_test_helper(L"0,1,1,2,3,5,8,13,21,34,55,89,144,233", { 0, 1, 1, 2, 3, 5, 8, 13, 21, 34, 55, 89, 144, 233 }, true);
			tokenizeWideStringofInts_test_helper(L"0-4,4-1,0,0-1,2-4", { 0, 1, 2, 3, 4, 1, 2, 3, 4, 0, 0, 1, 2, 3, 4 }, true);
			tokenizeWideStringofInts_test_helper(L"1,2,3,4,5-24,25", generateSequence(1, 25), true);
			tokenizeWideStringofInts_test_helper(L"0,1-1,2,3-5,8,21-13", { 0, 1, 2, 3, 4, 5, 8,13, 14, 15, 16, 17, 18, 19, 20, 21 }, true);
			tokenizeWideStringofInts_test_helper(L"3,14-17,99-101,7,27-23,999-1001", { 3,14,15,16,17,99,100,101,7,23,24,25,26,27,999,1000,1001 }, true);
		}

		TEST_METHOD(test_TokenizeWideStringOfInts_clear_output)
		{
			std::vector<uint32_t> Output = { 0, 1, 2, 4, 5, 6 };
			Assert::IsTrue(TokenizeWideStringOfInts(L"", L',', Output));
			Assert::AreEqual(Output.size(), (size_t)0);
		}



		void tokenizeWideStringofIntRange_test_helper(const std::wstring& input, const std::vector<uint32_t>& expectedOutput, bool expectedResult)
		{
			std::vector<uint32_t> Output;
			bool result = TokenizeWideStringofIntRange(input, L'-', Output);

			Assert::AreEqual(result, expectedResult);
			Assert::AreEqual(Output.size(), expectedOutput.size());
			Assert::IsTrue(Output == expectedOutput);
		}

		/*Call the following tests with tokenizeWideStringofIntRange_test_helper(wstring testInput, std::vector<uint32_t> expectedOutput, bool expectedResult) */
		TEST_METHOD(test_TokenizeWideStringofIntRange)
		{
			// Test basic usage
			tokenizeWideStringofIntRange_test_helper(L"0-3", generateSequence(0,3), true);
			tokenizeWideStringofIntRange_test_helper(L"0-0", generateSequence(0,0), true);
			tokenizeWideStringofIntRange_test_helper(L"4-0", generateSequence(0,4), true);
			tokenizeWideStringofIntRange_test_helper(L"9-2", generateSequence(2,9), true);

			// Test longer ranges
			tokenizeWideStringofIntRange_test_helper(L"0-255", generateSequence(0, 255), true);
			tokenizeWideStringofIntRange_test_helper(L"57-13", generateSequence(13,57), true);
			tokenizeWideStringofIntRange_test_helper(L"941-28", generateSequence(28, 941), true);
			tokenizeWideStringofIntRange_test_helper(L"966-999", generateSequence(966, 999), true);

			//Test bad input
			tokenizeWideStringofIntRange_test_helper(L"0--3", {}, false);
			tokenizeWideStringofIntRange_test_helper(L"36- 3", {}, false);
			tokenizeWideStringofIntRange_test_helper(L"9 -342", {}, false);
			tokenizeWideStringofIntRange_test_helper(L"-22222-3", {}, false);
			tokenizeWideStringofIntRange_test_helper(L"-68--6", {}, false);
			tokenizeWideStringofIntRange_test_helper(L"-391 - 01", {}, false);
			tokenizeWideStringofIntRange_test_helper(L"hello-Tokenizer", {}, false);
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
			Assert::IsTrue(CaseInsensitiveWStringStartsWith(std::wstring(L"/dsu/l3d_cache"), std::wstring(L"")));
			Assert::IsTrue(CaseInsensitiveWStringStartsWith(std::wstring(L"/dsu/l3d_cache"), std::wstring(L"/dsu/")));
			Assert::IsTrue(CaseInsensitiveWStringStartsWith(std::wstring(L"/dmc_clkdiv2/rdwr"), std::wstring(L"/dmc_clkdiv2/")));

			Assert::IsTrue(CaseInsensitiveWStringStartsWith(std::wstring(L"/DSU/l3d_cache"), std::wstring(L"")));
			Assert::IsTrue(CaseInsensitiveWStringStartsWith(std::wstring(L"/DSU/l3d_cache"), std::wstring(L"/dsu/")));
			Assert::IsTrue(CaseInsensitiveWStringStartsWith(std::wstring(L"/DMC_CLKDIV2/rdwr"), std::wstring(L"/dmc_clkdiv2/")));
		}

		TEST_METHOD(test_CaseInsensitiveWStringStartsWith_nok)
		{
			Assert::IsFalse(CaseInsensitiveWStringStartsWith(std::wstring(L"/dsu/l3d_cache"), std::wstring(L"l3d_cache")));
			Assert::IsFalse(CaseInsensitiveWStringStartsWith(std::wstring(L"/dsu/l3d_cache"), std::wstring(L"/dsu_")));
			Assert::IsFalse(CaseInsensitiveWStringStartsWith(std::wstring(L"/dmc_clkdiv2/rdwr"), std::wstring(L"rdwr")));
		}

		TEST_METHOD(test_WStringEndsWith)
		{
			Assert::IsTrue(WStringEndsWith(std::wstring(L"/dsu/l3d_cache"), std::wstring(L"")));
			Assert::IsTrue(WStringEndsWith(std::wstring(L"/dsu/l3d_cache"), std::wstring(L"/l3d_cache")));
			Assert::IsTrue(WStringEndsWith(std::wstring(L"/dsu/l3d_cache_refill"), std::wstring(L"refill")));
			Assert::IsTrue(WStringEndsWith(std::wstring(L"/dmc_clkdiv2/rdwr"), std::wstring(L"/rdwr")));
		}

		TEST_METHOD(test_CaseInsensitiveWStringEndsWith)
		{
			Assert::IsTrue(CaseInsensitiveWStringEndsWith(std::wstring(L"/dsu/l3d_cache"), std::wstring(L"")));
			Assert::IsTrue(CaseInsensitiveWStringEndsWith(std::wstring(L"/dsu/l3d_cache"), std::wstring(L"/l3d_cache")));
			Assert::IsTrue(CaseInsensitiveWStringEndsWith(std::wstring(L"/dsu/l3d_cache_refill"), std::wstring(L"refill")));
			Assert::IsTrue(CaseInsensitiveWStringEndsWith(std::wstring(L"/dmc_clkdiv2/rdwr"), std::wstring(L"/rdwr")));

			Assert::IsTrue(CaseInsensitiveWStringEndsWith(std::wstring(L"/DSU/L3D_CACHE"), std::wstring(L"")));
			Assert::IsTrue(CaseInsensitiveWStringEndsWith(std::wstring(L"/DSU/L3D_CACHE"), std::wstring(L"/l3d_cache")));
			Assert::IsTrue(CaseInsensitiveWStringEndsWith(std::wstring(L"/DSU/L3D_CACHE_REFILL"), std::wstring(L"refill")));
			Assert::IsTrue(CaseInsensitiveWStringEndsWith(std::wstring(L"/DMC_CLKDIV2/RDWR"), std::wstring(L"/rdwr")));
		}

		TEST_METHOD(test_CaseInsensitiveWStringEndsWith_nok)
		{
			Assert::IsFalse(CaseInsensitiveWStringEndsWith(std::wstring(L"/dsu/l3d_cache"), std::wstring(L"_l3d_cache")));
			Assert::IsFalse(CaseInsensitiveWStringEndsWith(std::wstring(L"/dsu/l3d_cache_refill"), std::wstring(L"/dsu")));
		}

		TEST_METHOD(test_ReplaceAllTokensInWString)
		{
			{
				std::wstring input = L"";
				ReplaceAllTokensInWString(input, L"a", L"_");
				Assert::AreEqual(input, std::wstring(L""));
			}

			{
				std::wstring input = L"abc";
				ReplaceAllTokensInWString(input, L"d", L"_");
				Assert::AreEqual(input, std::wstring(L"abc"));
			}

			{
				std::wstring input = L"aaa";
				ReplaceAllTokensInWString(input, L"a", L"__");
				Assert::AreEqual(input, std::wstring(L"______"));
			}

			{
				std::wstring input = L"aba";
				ReplaceAllTokensInWString(input, L"a", L"_");
				Assert::AreEqual(input, std::wstring(L"_b_"));
			}

			{
				std::wstring input = L"load_percentage,store_percentage,integer_dp_percentage,simd_percentage,scalar_fp_percentage,branch_percentage,crypto_percentage";
				ReplaceAllTokensInWString(input, L",", L", ");
				Assert::AreEqual(input, std::wstring(L"load_percentage, store_percentage, integer_dp_percentage, simd_percentage, scalar_fp_percentage, branch_percentage, crypto_percentage"));
			}
		}

		TEST_METHOD(test_ReplaceAllTokensInWString_complex)
		{
			{
				std::wstring input = L"abcabc";
				ReplaceAllTokensInWString(input, L"abc", L"cba");
				Assert::AreEqual(input, std::wstring(L"cbacba"));
			}

			{
				std::wstring input = L"!@#!@#";
				ReplaceAllTokensInWString(input, L"!@#", L"#@!");
				Assert::AreEqual(input, std::wstring(L"#@!#@!"));
			}

			{
				std::wstring input = L"aaa";
				ReplaceAllTokensInWString(input, L"a", L"aa");
				Assert::AreEqual(input, std::wstring(L"aaaaaa"));
			}

			{
				std::wstring input = L"WORD.WORD";
				ReplaceAllTokensInWString(input, L"WORD.", L"WORD.WORD.");
				Assert::AreEqual(input, std::wstring(L"WORD.WORD.WORD"));
			}
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

		TEST_METHOD(test_ReplaceAllTokensInString_string)
		{
			{
				std::string input = "\n";
				std::string output = ReplaceAllTokensInString(input, std::string("\n"), std::string("\\n"));
				Assert::AreEqual(output, std::string("\\n"));
			}

			{
				std::string input = "\n\n";
				std::string output = ReplaceAllTokensInString(input, std::string("\n"), std::string("\\n"));
				Assert::AreEqual(output, std::string("\\n\\n"));
			}

			{
				std::string input = "test string with new line at the end\n";
				std::string output = ReplaceAllTokensInString(input, std::string("\n"), std::string("\\n"));
				Assert::AreEqual(output, std::string("test string with new line at the end\\n"));
			}

			{
				std::string input = "The following cache operations are not counted:\n\n1. Invalidations which do not result in data being transferred out of the L1 (such as evictions of clean data),\n2.";
				std::string output = ReplaceAllTokensInString(input, std::string("\n"), std::string("\\n"));
				Assert::AreEqual(output, std::string("The following cache operations are not counted:\\n\\n1. Invalidations which do not result in data being transferred out of the L1 (such as evictions of clean data),\\n2."));
			}
		}

		TEST_METHOD(test_ReplaceAllTokensInString_string_complex)
		{
			{
				std::string input = "abcabc";
				std::string output = ReplaceAllTokensInString(input, std::string("abc"), std::string("cba"));
				Assert::AreEqual(output, std::string("cbacba"));
			}

			{
				std::string input = "aaa";
				std::string output = ReplaceAllTokensInString(input, std::string("a"), std::string("aa"));
				Assert::AreEqual(output, std::string("aaaaaa"));
			}

			{
				std::string input = ".......aaa";
				std::string output = ReplaceAllTokensInString(input, std::string("a"), std::string("aa"));
				Assert::AreEqual(output, std::string(".......aaaaaa"));
			}

			{
				std::string input = "aaa.......";
				std::string output = ReplaceAllTokensInString(input, std::string("a"), std::string("aa"));
				Assert::AreEqual(output, std::string("aaaaaa......."));
			}

			{
				std::string input = ".......aaa.......";
				std::string output = ReplaceAllTokensInString(input, std::string("a"), std::string("aa"));
				Assert::AreEqual(output, std::string(".......aaaaaa......."));
			}

			{
				std::string input = ".......aaa.aaa.......";
				std::string output = ReplaceAllTokensInString(input, std::string("a"), std::string("aa"));
				Assert::AreEqual(output, std::string(".......aaaaaa.aaaaaa......."));
			}

			{
				std::string input = "WORD.WORD";
				std::string output = ReplaceAllTokensInString(input, std::string("WORD."), std::string("WORD.WORD."));
				Assert::AreEqual(output, std::string("WORD.WORD.WORD"));
			}
		}

		TEST_METHOD(test_ReplaceAllTokensInString_wstring)
		{
			{
				std::wstring input = L"\n";
				std::wstring output = ReplaceAllTokensInString(input, std::wstring(L"\n"), std::wstring(L"\\n"));
				Assert::AreEqual(output, std::wstring(L"\\n"));
			}

			{
				std::wstring input = L"\n\n";
				std::wstring output = ReplaceAllTokensInString(input, std::wstring(L"\n"), std::wstring(L"\\n"));
				Assert::AreEqual(output, std::wstring(L"\\n\\n"));
			}

			{
				std::wstring input = L"test string with new line at the end\n";
				std::wstring output = ReplaceAllTokensInString(input, std::wstring(L"\n"), std::wstring(L"\\n"));
				Assert::AreEqual(output, std::wstring(L"test string with new line at the end\\n"));
			}

			{
				std::wstring input = L"The following cache operations are not counted:\n\n1. Invalidations which do not result in data being transferred out of the L1 (such as evictions of clean data),\n2.";
				std::wstring output = ReplaceAllTokensInString(input, std::wstring(L"\n"), std::wstring(L"\\n"));
				Assert::AreEqual(output, std::wstring(L"The following cache operations are not counted:\\n\\n1. Invalidations which do not result in data being transferred out of the L1 (such as evictions of clean data),\\n2."));
			}
		}

		TEST_METHOD(test_ReplaceAllTokensInString_wstring_complex)
		{
			{
				std::wstring input = L"abcabc";
				std::wstring output = ReplaceAllTokensInString(input, std::wstring(L"abc"), std::wstring(L"cba"));
				Assert::AreEqual(output, std::wstring(L"cbacba"));
			}

			{
				std::wstring input = L"aaa";
				std::wstring output = ReplaceAllTokensInString(input, std::wstring(L"a"), std::wstring(L"aa"));
				Assert::AreEqual(output, std::wstring(L"aaaaaa"));
			}

			{
				std::wstring input = L".......aaa";
				std::wstring output = ReplaceAllTokensInString(input, std::wstring(L"a"), std::wstring(L"aa"));
				Assert::AreEqual(output, std::wstring(L".......aaaaaa"));
			}

			{
				std::wstring input = L"aaa.......";
				std::wstring output = ReplaceAllTokensInString(input, std::wstring(L"a"), std::wstring(L"aa"));
				Assert::AreEqual(output, std::wstring(L"aaaaaa......."));
			}

			{
				std::wstring input = L".......aaa.......";
				std::wstring output = ReplaceAllTokensInString(input, std::wstring(L"a"), std::wstring(L"aa"));
				Assert::AreEqual(output, std::wstring(L".......aaaaaa......."));
			}

			{
				std::wstring input = L".......aaa.aaa.......";
				std::wstring output = ReplaceAllTokensInString(input, std::wstring(L"a"), std::wstring(L"aa"));
				Assert::AreEqual(output, std::wstring(L".......aaaaaa.aaaaaa......."));
			}

			{
				std::wstring input = L"WORD.WORD";
				std::wstring output = ReplaceAllTokensInString(input, std::wstring(L"WORD."), std::wstring(L"WORD.WORD."));
				Assert::AreEqual(output, std::wstring(L"WORD.WORD.WORD"));
			}
		}

		TEST_METHOD(test_TokenizeWideStringOfStringsDelim)
		{
			{
				std::wstring input = L"a";
				std::vector<std::wstring> tokens;
				TokenizeWideStringOfStringsDelim(input, L'|', tokens);

				Assert::IsTrue(tokens.size() == 1);
			}

			{
				std::wstring input = L"";
				std::vector<std::wstring> tokens;
				TokenizeWideStringOfStringsDelim(input, L'|', tokens);

				Assert::IsTrue(tokens.size() == 0);
			}
		}

		TEST_METHOD(test_TokenizeWideStringOfStringsDelim_abc)
		{
			{
				std::wstring input = L"x__#__y";
				std::vector<std::wstring> tokens;
				TokenizeWideStringOfStringsDelim(input, L'#', tokens);

				Assert::IsTrue(tokens.size() == 2);
				Assert::AreEqual(tokens[0], std::wstring(L"x__#"));
				Assert::AreEqual(tokens[1], std::wstring(L"__y"));
			}

			{
				std::wstring input = L"a_|_b_|_c";
				std::vector<std::wstring> tokens;
				TokenizeWideStringOfStringsDelim(input, L'|', tokens);

				Assert::IsTrue(tokens.size() == 3);
				Assert::AreEqual(tokens[0], std::wstring(L"a_|"));
				Assert::AreEqual(tokens[1], std::wstring(L"_b_|"));
				Assert::AreEqual(tokens[2], std::wstring(L"_c"));
			}

			{
				std::wstring input = L"a,b,c,d,e,f,g";
				std::vector<std::wstring> tokens;
				TokenizeWideStringOfStringsDelim(input, L',', tokens);

				Assert::IsTrue(tokens.size() == 7);
				Assert::AreEqual(tokens[0], std::wstring(L"a,"));
				Assert::AreEqual(tokens[1], std::wstring(L"b,"));
				Assert::AreEqual(tokens[2], std::wstring(L"c,"));
				Assert::AreEqual(tokens[3], std::wstring(L"d,"));
				Assert::AreEqual(tokens[4], std::wstring(L"e,"));
				Assert::AreEqual(tokens[5], std::wstring(L"f,"));
				Assert::AreEqual(tokens[6], std::wstring(L"g"));
			}
		}

		TEST_METHOD(test_ConvertTimeUnitToSeconds)
		{
			const std::unordered_map<std::wstring, double> unitMap = { {L"s", 1}, { L"m", 60 }, {L"ms", 0.001}, {L"h", 3600}, {L"d" , 86400}};

			Assert::AreEqual(ConvertNumberWithUnit(double(0.311), std::wstring(L"ms"), unitMap), double(0.000311));
			Assert::AreEqual(ConvertNumberWithUnit(double(10.24), std::wstring(L"s"), unitMap), double(10.24));
			Assert::AreEqual(ConvertNumberWithUnit(double(60), std::wstring(L"d"), unitMap), double(5184000));
			Assert::AreEqual(ConvertNumberWithUnit(double(2.5), std::wstring(L"h"), unitMap), double(9000));
		}
	};
}
