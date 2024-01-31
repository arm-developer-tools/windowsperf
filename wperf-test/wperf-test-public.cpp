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
#include <Guiddef.h>
#include "wperf-common/public.h"

#include <string>
#include <vector>


using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace wperftest
{
	TEST_CLASS(wperftest_common_public_ver_macros)
	{
	public:

		TEST_METHOD(test_public_ver_macros)
		{
			// Smoke test for public.h version macros
			Assert::IsTrue(MAJOR > 0);		// We are beyond ver. 1.x.y
			Assert::IsTrue(MINOR >= 0);
			Assert::IsTrue(PATCH >= 0);
		}

		TEST_METHOD(test_public_ver_WPERF_VER_PRODUCTVERSION_macro)
		{
			// WPERF_VER_PRODUCTVERSION generates comma separated list of integers
			std::vector<int> v = { WPERF_VER_PRODUCTVERSION(MAJOR,MINOR,PATCH) };	// E.g. 3,4,0,0

			Assert::IsTrue(v.size() == 4);
			Assert::IsTrue(v[0] == MAJOR);
			Assert::IsTrue(v[1] == MINOR);
			Assert::IsTrue(v[2] == PATCH);
			Assert::IsTrue(v[3] == 0);	// Always ZERO
		}

		TEST_METHOD(test_public_ver_WPERF_VER_PRODUCTVERSION_macro_012)
		{
			std::vector<int> v = { WPERF_VER_PRODUCTVERSION(7, 1, 2) };

			Assert::IsTrue(v.size() == 4);
			Assert::IsTrue(v[0] == 7);	// Use `7` as zero is trailing number
			Assert::IsTrue(v[1] == 1);
			Assert::IsTrue(v[2] == 2);
			Assert::IsTrue(v[3] == 0);	// Always ZERO
		}

		TEST_METHOD(test_public_ver_WPERF_VER_PRODUCTVERSION_macro_to_str)
		{
			// WPERF_VER_PRODUCTVERSION generates comma separated list of integers
			// This checks with conversion to STRING
			std::string s = WPERF_XSTRING(WPERF_VER_PRODUCTVERSION(MAJOR, MINOR, PATCH));

			std::ostringstream oss;
			oss << MAJOR << "," << MINOR << "," << PATCH << ",0";	// E.g. "3,4,0,0"

			Assert::AreEqual(s, oss.str());
		}

		TEST_METHOD(test_public_ver_WPERF_VER_PRODUCTVERSION_macro_to_str_712)
		{
			std::string s = WPERF_XSTRING(WPERF_VER_PRODUCTVERSION(7, 1, 2));

			Assert::AreEqual(s, std::string("7,1,2,0"));
		}

		TEST_METHOD(test_public_ver_WPERF_VER_PRODUCTVERSION_STR_macro)
		{
			std::string s = WPERF_VER_PRODUCTVERSION_STR(WPERF_XSTRING(MAJOR), WPERF_XSTRING(MINOR), WPERF_XSTRING(PATCH));		// E.g. "3.4.0.0"

			std::ostringstream oss;
			oss << MAJOR << "." << MINOR << "." << PATCH << ".0";	// E.g. "3.4.0.0"

			Assert::AreEqual(s, oss.str());
		}

		TEST_METHOD(test_public_ver_WPERF_VER_PRODUCTVERSION_STR_macro_712)
		{
			std::string s = WPERF_VER_PRODUCTVERSION_STR(WPERF_XSTRING(7), WPERF_XSTRING(1), WPERF_XSTRING(2));

			Assert::AreEqual(s, std::string("7.1.2.0"));
		}

		TEST_METHOD(test_public_ver_Resource_rc_integration)
		{
			{
#define VER_PRODUCTVERSION      WPERF_VER_PRODUCTVERSION(3,4,5)
				std::vector<int> v = { VER_PRODUCTVERSION };

				Assert::IsTrue(v.size() == 4);
				Assert::IsTrue(v[0] == 3);
				Assert::IsTrue(v[1] == 4);
				Assert::IsTrue(v[2] == 5);
				Assert::IsTrue(v[3] == 0);	// Always ZERO
#undef VER_PRODUCTVERSION
			}

			{
#define VER_PRODUCTVERSION_STR	WPERF_VER_PRODUCTVERSION_STR(WPERF_XSTRING(7), WPERF_XSTRING(8), WPERF_XSTRING(9))
				std::string s = VER_PRODUCTVERSION_STR;

				Assert::AreEqual(s, std::string("7.8.9.0"));
#undef VER_PRODUCTVERSION_STR
			}
		}

	};
}
