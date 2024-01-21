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


#include <string>

#include "pch.h"
#include "CppUnitTest.h"

#include "wperf/pe_file.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace wperftest
{
	TEST_CLASS(wperftest_pe_file)
	{
	public:

		TEST_METHOD(test_gen_pdb_name_dll)
		{
			Assert::AreEqual(gen_pdb_name(L"ADVAPI32.dll"), std::wstring(L"ADVAPI32.pdb"));
			Assert::AreEqual(gen_pdb_name(L"KERNEL32.DLL"), std::wstring(L"KERNEL32.pdb"));
			Assert::AreEqual(gen_pdb_name(L"KERNELBASE.dll"), std::wstring(L"KERNELBASE.pdb"));
			Assert::AreEqual(gen_pdb_name(L"RPCRT4.dll"), std::wstring(L"RPCRT4.pdb"));
			Assert::AreEqual(gen_pdb_name(L"VCRUNTIME140.dll"), std::wstring(L"VCRUNTIME140.pdb"));
			Assert::AreEqual(gen_pdb_name(L"VCRUNTIME140D.dll"), std::wstring(L"VCRUNTIME140D.pdb"));
			Assert::AreEqual(gen_pdb_name(L"VERSION.dll"), std::wstring(L"VERSION.pdb"));
			Assert::AreEqual(gen_pdb_name(L"WS2_32.dll"), std::wstring(L"WS2_32.pdb"));
			Assert::AreEqual(gen_pdb_name(L"msvcrt.dll"), std::wstring(L"msvcrt.pdb"));
			Assert::AreEqual(gen_pdb_name(L"ntdll.dll"), std::wstring(L"ntdll.pdb"));
			Assert::AreEqual(gen_pdb_name(L"python312_d.dll"), std::wstring(L"python312_d.pdb"));
		}

		TEST_METHOD(test_gen_pdb_name_exe)
		{
			Assert::AreEqual(gen_pdb_name(L"python_d.exe"), std::wstring(L"python_d.pdb"));
			Assert::AreEqual(gen_pdb_name(L"wperf.exe"), std::wstring(L"wperf.pdb"));
		}

		TEST_METHOD(test_gen_pdb_name_dll_path)
		{
			Assert::AreEqual(gen_pdb_name(L"C:\\Windows\\System32\\ADVAPI32.dll"), std::wstring(L"C:\\Windows\\System32\\ADVAPI32.pdb"));
			Assert::AreEqual(gen_pdb_name(L"C:\\Windows\\System32\\KERNEL32.DLL"), std::wstring(L"C:\\Windows\\System32\\KERNEL32.pdb"));
			Assert::AreEqual(gen_pdb_name(L"C:\\Windows\\System32\\KERNELBASE.dll"), std::wstring(L"C:\\Windows\\System32\\KERNELBASE.pdb"));
			Assert::AreEqual(gen_pdb_name(L"C:\\Windows\\System32\\RPCRT4.dll"), std::wstring(L"C:\\Windows\\System32\\RPCRT4.pdb"));
			Assert::AreEqual(gen_pdb_name(L"C:\\Windows\\SYSTEM32\\VCRUNTIME140D.dll"), std::wstring(L"C:\\Windows\\SYSTEM32\\VCRUNTIME140D.pdb"));
			Assert::AreEqual(gen_pdb_name(L"C:\\Windows\\SYSTEM32\\VERSION.dll"), std::wstring(L"C:\\Windows\\SYSTEM32\\VERSION.pdb"));
			Assert::AreEqual(gen_pdb_name(L"C:\\Windows\\System32\\msvcrt.dll"), std::wstring(L"C:\\Windows\\System32\\msvcrt.pdb"));
			Assert::AreEqual(gen_pdb_name(L"C:\\Windows\\SYSTEM32\\ntdll.dll"), std::wstring(L"C:\\Windows\\SYSTEM32\\ntdll.pdb"));
		}

		TEST_METHOD(test_gen_pdb_name_multiple_dll_dots)
		{
			Assert::AreEqual(gen_pdb_name(L"api-ms-win-crt-runtime-l1-1.2.3.dll"), std::wstring(L"api-ms-win-crt-runtime-l1-1.2.3.pdb"));
			Assert::AreEqual(gen_pdb_name(L".lots.of.dots.in.filename....dll"), std::wstring(L".lots.of.dots.in.filename....pdb"));
		}
	};
}
