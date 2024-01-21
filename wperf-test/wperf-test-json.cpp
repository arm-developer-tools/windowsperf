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

#include <sstream>
#include "pch.h"
#include "CppUnitTest.h"

#include "wperf/json.h"
#include "wperf/output.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace wperftest
{
	template <typename CharType>
	struct UnitTestOutputTraits : public TableOutputTraits<CharType>
	{
		typedef typename std::conditional_t<std::is_same_v<CharType, char>, std::string, std::wstring> StringType;
		inline const static std::tuple<StringType> columns;
		inline const static std::tuple<CharType*> headers =
			std::make_tuple(LITERALCONSTANTS_GET("column_header"));
		inline const static int size = std::tuple_size_v<decltype(headers)>;
		inline const static CharType* key = LITERALCONSTANTS_GET("json_key");
	};

	TEST_CLASS(wperftest_json)
	{
	public:		
		TEST_METHOD(test_tablejson_char)
		{
			{
				TableJSON<UnitTestOutputTraits<char>, char> table;
				std::stringstream ss;
				ss << table;
				Assert::AreEqual(std::string("{}"), ss.str());
			}
			{
				TableJSON<UnitTestOutputTraits<char>, char> table;
				std::stringstream ss;
				std::vector<std::string> vals = { };
				table.SetKey("json_key");
				table.AddColumn("column_header");
				table.Insert(vals);
				ss << table;
				Assert::AreEqual(std::string("{\"json_key\":[]}"), ss.str());
			}
			{
				TableJSON<UnitTestOutputTraits<char>, char> table;
				std::stringstream ss;
				std::vector<std::string> vals = { std::string("item1") };
				table.SetKey("json_key");
				table.AddColumn("column_header");
				table.Insert(vals);
				ss << table;
				Assert::AreEqual(std::string("{\"json_key\":[{\"column_header\":\"item1\"}]}"), ss.str());
			}

		}

		TEST_METHOD(test_tablejson_wchar)
		{
			{
				TableJSON<UnitTestOutputTraits<wchar_t>, wchar_t> table;
				std::wstringstream ss;
				ss << table;
				Assert::AreEqual(std::wstring(L"{}"), ss.str());
			}
			{
				TableJSON<UnitTestOutputTraits<wchar_t>, wchar_t> table;
				std::wstringstream ss;
				std::vector<std::wstring> vals = { };
				table.SetKey(L"json_key");
				table.AddColumn(L"column_header");
				table.Insert(vals);
				ss << table;
				Assert::AreEqual(std::wstring(L"{\"json_key\":[]}"), ss.str());
			}
			{
				TableJSON<UnitTestOutputTraits<wchar_t>, wchar_t> table;
				std::wstringstream ss;
				std::vector<std::wstring> vals = { std::wstring(L"item1") };
				table.SetKey(L"json_key");
				table.AddColumn(L"column_header");
				table.Insert(vals);
				ss << table;
				Assert::AreEqual(std::wstring(L"{\"json_key\":[{\"column_header\":\"item1\"}]}"), ss.str());
			}

		}

		TEST_METHOD(test_jsonobject_char)
		{
			{
				JSONObject<int, false, true, char> obj;
				std::stringstream ss;
				ss << obj;
				Assert::AreEqual(std::string("{}"), ss.str());
			}
			{
				JSONObject<int, false, true, char> obj;
				obj.m_map["key1"] = 1;
				std::stringstream ss;
				ss << obj;
				Assert::AreEqual(std::string("{\"key1\":1}"), ss.str());
			}
			{
				JSONObject<double, false, true, char> obj;
				obj.m_map["key1"] = 1.1;
				std::stringstream ss;
				ss << obj;
				Assert::AreEqual(std::string("{\"key1\":1.1}"), ss.str());
			}
			{
				JSONObject<char, false, true, char> obj;
				obj.m_map["key1"] = 'a';
				std::stringstream ss;
				ss << obj;
				Assert::AreEqual(std::string("{\"key1\":\"a\"}"), ss.str());
			}
			{
				JSONObject<wchar_t, false, true, char> obj;
				obj.m_map["key1"] = 'a';
				std::stringstream ss;
				ss << obj;
				Assert::AreEqual(std::string("{\"key1\":\"97\"}"), ss.str());
			}
		}

		TEST_METHOD(test_jsonobject_wchar)
		{
			{
				JSONObject<int, false, true, wchar_t> obj;
				std::wstringstream ss;
				ss << obj;
				Assert::AreEqual(std::wstring(L"{}"), ss.str());
			}
			{
				JSONObject<int, false, true, wchar_t> obj;
				obj.m_map[L"key1"] = 1;
				std::wstringstream ss;
				ss << obj;
				Assert::AreEqual(std::wstring(L"{\"key1\":1}"), ss.str());
			}
			{
				JSONObject<double, false, true, wchar_t> obj;
				obj.m_map[L"key1"] = 1.1;
				std::wstringstream ss;
				ss << obj;
				Assert::AreEqual(std::wstring(L"{\"key1\":1.1}"), ss.str());
			}
			{
				JSONObject<char, false, true, wchar_t> obj;
				obj.m_map[L"key1"] = 'a';
				std::wstringstream ss;
				ss << obj;
				Assert::AreEqual(std::wstring(L"{\"key1\":\"a\"}"), ss.str());
			}
			{
				JSONObject<wchar_t, false, true, char> obj;
				obj.m_map["key1"] = 'a';
				std::stringstream ss;
				ss << obj;
				Assert::AreEqual(std::string("{\"key1\":\"97\"}"), ss.str());
			}
		}
	};
}
