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

#include <algorithm>
#include <functional>
#include <sstream>
#include "pch.h"
#include "CppUnitTest.h"

#include <windows.h>
#include "wperf/events.h"
#include "wperf/parsers.h"
#include "wperf/exception.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace wperftest
{
	TEST_CLASS(wperftest_parsers_events_feat_spe)
	{
	public:

		TEST_METHOD(parse_events_str_for_feat_spe_incorrect_input)
		{
			std::map<std::wstring, bool> flags;
			Assert::IsFalse(parse_events_str_for_feat_spe(std::wstring(L"arm_spe_0/"), flags));
			Assert::IsFalse(parse_events_str_for_feat_spe(std::wstring(L"arm_spe_0"), flags));
			Assert::IsFalse(parse_events_str_for_feat_spe(std::wstring(L"arm_spe_/"), flags));
			Assert::IsFalse(parse_events_str_for_feat_spe(std::wstring(L"arm_sp"), flags));
			Assert::IsFalse(parse_events_str_for_feat_spe(std::wstring(L"arm_"), flags));
			Assert::IsFalse(parse_events_str_for_feat_spe(std::wstring(L"_"), flags));
			Assert::IsFalse(parse_events_str_for_feat_spe(std::wstring(L" "), flags));
			Assert::IsFalse(parse_events_str_for_feat_spe(std::wstring(L""), flags));
		}

		TEST_METHOD(parse_events_str_for_feat_spe_incorrect_input_throw)
		{
			auto wrapper_filter_eq_2 = [=]() {
				std::map<std::wstring, bool> flags;
				Assert::IsFalse(parse_events_str_for_feat_spe(std::wstring(L"arm_spe_0/branch_filter=2/"), flags));
				};
			Assert::ExpectException<fatal_exception>(wrapper_filter_eq_2);

			auto wrapper_filter_name_empty = [=]() {
				std::map<std::wstring, bool> flags;
				Assert::IsFalse(parse_events_str_for_feat_spe(std::wstring(L"arm_spe_0/=1/"), flags));
				};
			Assert::ExpectException<fatal_exception>(wrapper_filter_name_empty);

			auto wrapper_filter_just_eq = [=]() {
				std::map<std::wstring, bool> flags;
				Assert::IsFalse(parse_events_str_for_feat_spe(std::wstring(L"arm_spe_0/=/"), flags));
				};
			Assert::ExpectException<fatal_exception>(wrapper_filter_just_eq);

			auto wrapper_filter_eq_letter = [=]() {
				std::map<std::wstring, bool> flags;
				Assert::IsFalse(parse_events_str_for_feat_spe(std::wstring(L"arm_spe_0/jitter=x/"), flags));
				};
			Assert::ExpectException<fatal_exception>(wrapper_filter_eq_letter);
		}

		TEST_METHOD(parse_events_str_for_feat_spe_branch_filter_1)
		{
			std::wstring events_str = L"arm_spe_0/branch_filter=1/";
			std::map<std::wstring, bool> flags;

			bool ret = parse_events_str_for_feat_spe(events_str, flags);

			Assert::IsTrue(ret);
			Assert::IsTrue(flags.size() == 1);
			Assert::IsTrue(flags.count(L"branch_filter"));
			Assert::IsTrue(flags[L"branch_filter"] == true);
		}

		TEST_METHOD(parse_events_str_for_feat_spe_branch_filter_0)
		{
			std::wstring events_str = L"arm_spe_0/branch_filter=0/";
			std::map<std::wstring, bool> flags;

			bool ret = parse_events_str_for_feat_spe(events_str, flags);

			Assert::IsTrue(ret);
			Assert::IsTrue(flags.size() == 1);
			Assert::IsTrue(flags.count(L"branch_filter"));
			Assert::IsTrue(flags[L"branch_filter"] == false);
		}

		TEST_METHOD(parse_events_str_for_feat_spe_2_filters_10)
		{
			std::wstring events_str = L"arm_spe_0/branch_filter=1,jitter=0/";
			std::map<std::wstring, bool> flags;

			bool ret = parse_events_str_for_feat_spe(events_str, flags);

			Assert::IsTrue(ret);
			Assert::IsTrue(flags.size() == 2);
			Assert::IsTrue(flags.count(L"branch_filter"));
			Assert::IsTrue(flags.count(L"jitter"));
			Assert::IsTrue(flags[L"branch_filter"] == true);
			Assert::IsTrue(flags[L"jitter"] == false);
		}

		TEST_METHOD(parse_events_str_for_feat_spe_2_filters_01)
		{
			std::wstring events_str = L"arm_spe_0/branch_filter=0,jitter=1/";
			std::map<std::wstring, bool> flags;

			bool ret = parse_events_str_for_feat_spe(events_str, flags);

			Assert::IsTrue(ret);
			Assert::IsTrue(flags.size() == 2);
			Assert::IsTrue(flags.count(L"branch_filter"));
			Assert::IsTrue(flags.count(L"jitter"));
			Assert::IsTrue(flags[L"branch_filter"] == false);
			Assert::IsTrue(flags[L"jitter"] == true);
		}

		TEST_METHOD(parse_events_str_for_feat_spe_2_filters_11)
		{
			std::wstring events_str = L"arm_spe_0/branch_filter=1,jitter=1/";
			std::map<std::wstring, bool> flags;

			bool ret = parse_events_str_for_feat_spe(events_str, flags);

			Assert::IsTrue(ret);
			Assert::IsTrue(flags.size() == 2);
			Assert::IsTrue(flags.count(L"branch_filter"));
			Assert::IsTrue(flags.count(L"jitter"));
			Assert::IsTrue(flags[L"branch_filter"] == true);
			Assert::IsTrue(flags[L"jitter"] == true);
		}

		TEST_METHOD(parse_events_str_for_feat_spe_2_filters_00)
		{
			std::wstring events_str = L"arm_spe_0/branch_filter=0,jitter=0/";
			std::map<std::wstring, bool> flags;

			bool ret = parse_events_str_for_feat_spe(events_str, flags);

			Assert::IsTrue(ret);
			Assert::IsTrue(flags.size() == 2);
			Assert::IsTrue(flags.count(L"branch_filter"));
			Assert::IsTrue(flags.count(L"jitter"));
			Assert::IsTrue(flags[L"branch_filter"] == false);
			Assert::IsTrue(flags[L"jitter"] == false);
		}

		/* Example usage could include:

		PMSFCR_EL1.FT enables filtering by operation type. When enabled PMSFCR_EL1.{ST, LD, B} define the
		collected types:
		* ST enables collection of store sampled operations, including all atomic operations.
		* LD enables collection of load sampled operations, including atomic operations that return a value to 
		  register.
		* B enables collection of branch sampled operations, including direct and indirect branches and exception
		  returns.		
		*/

		TEST_METHOD(parse_events_str_for_feat_spe_2_filters_010)
		{
			std::wstring events_str = L"arm_spe_0/ld=0,st=1,b=0/";
			std::map<std::wstring, bool> flags;

			bool ret = parse_events_str_for_feat_spe(events_str, flags);

			Assert::IsTrue(ret);
			Assert::IsTrue(flags.size() == 3);
			Assert::IsTrue(flags.count(L"ld"));
			Assert::IsTrue(flags.count(L"st"));
			Assert::IsTrue(flags.count(L"b"));
			Assert::IsTrue(flags[L"ld"] == false);
			Assert::IsTrue(flags[L"st"] == true);
			Assert::IsTrue(flags[L"b"] == false);
		}
	};

	/****************************************************************************/

	TEST_CLASS(wperftest_parsers_events_sampling)
	{
	public:
		TEST_METHOD(test_parse_events_str_for_sample_1)
		{
			std::vector<struct evt_sample_src> ioctl_events_sample;	// Sorted by index !
			std::map<uint32_t, uint32_t> sampling_inverval;		// [event_idx] => frequency

			parse_events_str_for_sample(L"vfp_spec:10000", ioctl_events_sample, sampling_inverval);

			Assert::IsTrue(sampling_inverval.size() == 1);
			Assert::IsTrue(sampling_inverval.count(0x0075) == 1);

			Assert::IsTrue(sampling_inverval[0x0075] == 10000);

			Assert::IsTrue(ioctl_events_sample.size() == 1);	// Sorted by index !

			Assert::IsTrue(ioctl_events_sample[0].index == uint32_t(0x0075));			// vfp_spec
			Assert::IsTrue(ioctl_events_sample[0].interval == uint32_t(10000));
		}

		TEST_METHOD(test_parse_events_str_for_sample_2)
		{
			std::vector<struct evt_sample_src> ioctl_events_sample;	// Sorted by index !
			std::map<uint32_t, uint32_t> sampling_inverval;		// [event_idx] => frequency

			parse_events_str_for_sample(L"vfp_spec:12345,ld_spec:67890", ioctl_events_sample, sampling_inverval);

			Assert::IsTrue(sampling_inverval.size() == 2);
			Assert::IsTrue(sampling_inverval.count(0x0075) == 1);
			Assert::IsTrue(sampling_inverval.count(0x0070) == 1);

			Assert::IsTrue(sampling_inverval[0x0070] == 67890);
			Assert::IsTrue(sampling_inverval[0x0075] == 12345);

			Assert::IsTrue(ioctl_events_sample.size() == 2);	// Sorted by index !

			Assert::IsTrue(ioctl_events_sample[0].index == uint32_t(0x0070));			// ld_spec
			Assert::IsTrue(ioctl_events_sample[0].interval == uint32_t(67890));

			Assert::IsTrue(ioctl_events_sample[1].index == uint32_t(0x0075));			// vfp_spec
			Assert::IsTrue(ioctl_events_sample[1].interval == uint32_t(12345));
		}

		TEST_METHOD(test_parse_events_str_for_sample_3)
		{
			std::vector<struct evt_sample_src> ioctl_events_sample;	// Sorted by index !
			std::map<uint32_t, uint32_t> sampling_inverval;		// [event_idx] => frequency

			parse_events_str_for_sample(L"vfp_spec:12345,ld_spec:67890,cpu_cycles:1234321", ioctl_events_sample, sampling_inverval);

			Assert::IsTrue(sampling_inverval.size() == 3);
			Assert::IsTrue(sampling_inverval.count(0x0075) == 1);
			Assert::IsTrue(sampling_inverval.count(0x0070) == 1);
			Assert::IsTrue(sampling_inverval.count(0x0011) == 1);

			Assert::IsTrue(sampling_inverval[0x0075] == 12345);
			Assert::IsTrue(sampling_inverval[0x0070] == 67890);
			Assert::IsTrue(sampling_inverval[0x0011] == 1234321);

			Assert::IsTrue(ioctl_events_sample.size() == 3);	// Sorted by index !

			Assert::IsTrue(ioctl_events_sample[0].index == uint32_t(0x0070));			// ld_spec
			Assert::IsTrue(ioctl_events_sample[0].interval == uint32_t(67890));

			Assert::IsTrue(ioctl_events_sample[1].index == uint32_t(0x0075));			// vfp_spec
			Assert::IsTrue(ioctl_events_sample[1].interval == uint32_t(12345));

			Assert::IsTrue(ioctl_events_sample[2].index == uint32_t(CYCLE_EVT_IDX));	// cpu_cycles
			Assert::IsTrue(ioctl_events_sample[2].interval == uint32_t(1234321));
		}

		TEST_METHOD(test_parse_events_str_for_sample_3_default_interval_ld_spec)
		{
			std::vector<struct evt_sample_src> ioctl_events_sample;	// Sorted by index !
			std::map<uint32_t, uint32_t> sampling_inverval;		// [event_idx] => frequency

			parse_events_str_for_sample(L"vfp_spec:12345,ld_spec,cpu_cycles:1234321", ioctl_events_sample, sampling_inverval);

			Assert::IsTrue(sampling_inverval.size() == 3);
			Assert::IsTrue(sampling_inverval.count(0x0075) == 1);
			Assert::IsTrue(sampling_inverval.count(0x0070) == 1);
			Assert::IsTrue(sampling_inverval.count(0x0011) == 1);

			Assert::IsTrue(sampling_inverval[0x0075] == 12345);
			Assert::IsTrue(sampling_inverval[0x0070] == PARSE_INTERVAL_DEFAULT);
			Assert::IsTrue(sampling_inverval[0x0011] == 1234321);

			Assert::IsTrue(ioctl_events_sample.size() == 3);	// Sorted by index !

			Assert::IsTrue(ioctl_events_sample[0].index == uint32_t(0x0070));			// ld_spec
			Assert::IsTrue(ioctl_events_sample[0].interval == uint32_t(PARSE_INTERVAL_DEFAULT));

			Assert::IsTrue(ioctl_events_sample[1].index == uint32_t(0x0075));			// vfp_spec
			Assert::IsTrue(ioctl_events_sample[1].interval == uint32_t(12345));

			Assert::IsTrue(ioctl_events_sample[2].index == uint32_t(CYCLE_EVT_IDX));	// cpu_cycles
			Assert::IsTrue(ioctl_events_sample[2].interval == uint32_t(1234321));
		}
	};

	/****************************************************************************/

	TEST_CLASS(wperftest_parsers_raw_events_sampling)
	{
	public:
		TEST_METHOD(test_parse_raw_events_str_for_sample_rf)
		{
			std::vector<struct evt_sample_src> ioctl_events_sample;
			std::map<uint32_t, uint32_t> sampling_inverval;

			parse_events_str_for_sample(L"rf", ioctl_events_sample, sampling_inverval);

			Assert::AreEqual(ioctl_events_sample.size(), (size_t)1);
			Assert::AreEqual(ioctl_events_sample[0].index, (uint32_t)0x0F);
			Assert::AreEqual(ioctl_events_sample[0].interval, PARSE_INTERVAL_DEFAULT);
		}

		TEST_METHOD(test_parse_raw_events_str_for_sample_rf_interval)
		{
			std::vector<struct evt_sample_src> ioctl_events_sample;
			std::map<uint32_t, uint32_t> sampling_inverval;

			parse_events_str_for_sample(L"rf:12345", ioctl_events_sample, sampling_inverval);

			Assert::IsTrue(ioctl_events_sample.size() ==  size_t(1));
			Assert::IsTrue(ioctl_events_sample[0].index == uint32_t(0x0F));
			Assert::IsTrue(ioctl_events_sample[0].interval == uint32_t(12345));
		}

		TEST_METHOD(test_parse_raw_events_str_for_sample_r1b)
		{
			std::vector<struct evt_sample_src> ioctl_events_sample;
			std::map<uint32_t, uint32_t> sampling_inverval;

			parse_events_str_for_sample(L"r1b", ioctl_events_sample, sampling_inverval);

			Assert::AreEqual(ioctl_events_sample.size(), (size_t)1);
			Assert::AreEqual(ioctl_events_sample[0].index, (uint32_t)0x1B);
			Assert::AreEqual(ioctl_events_sample[0].interval, PARSE_INTERVAL_DEFAULT);
		}

		TEST_METHOD(test_parse_raw_events_str_for_sample_r12_rab)
		{
			std::vector<struct evt_sample_src> ioctl_events_sample;
			std::map<uint32_t, uint32_t> sampling_inverval;

			parse_events_str_for_sample(L"r12,rab", ioctl_events_sample, sampling_inverval);

			Assert::AreEqual(ioctl_events_sample.size(), (size_t)2);
			Assert::AreEqual(ioctl_events_sample[0].index, (uint32_t)0x12);
			Assert::AreEqual(ioctl_events_sample[0].interval, PARSE_INTERVAL_DEFAULT);

			Assert::AreEqual(ioctl_events_sample[1].index, (uint32_t)0xAB);
			Assert::AreEqual(ioctl_events_sample[1].interval, PARSE_INTERVAL_DEFAULT);
		}

		TEST_METHOD(test_parse_raw_events_str_for_sample_r12_rab_interval)
		{
			std::vector<struct evt_sample_src> ioctl_events_sample;
			std::map<uint32_t, uint32_t> sampling_inverval;

			parse_events_str_for_sample(L"r12:12345,rab:67890", ioctl_events_sample, sampling_inverval);

			Assert::AreEqual(ioctl_events_sample.size(), (size_t)2);
			Assert::AreEqual(ioctl_events_sample[0].index, (uint32_t)0x12);
			Assert::AreEqual(ioctl_events_sample[0].interval, uint32_t(12345));

			Assert::AreEqual(ioctl_events_sample[1].index, (uint32_t)0xAB);
			Assert::AreEqual(ioctl_events_sample[1].interval, uint32_t(67890));
		}

		TEST_METHOD(test_parse_raw_events_str_for_sample_r_7_events)
		{
			std::vector<struct evt_sample_src> ioctl_events_sample;
			std::map<uint32_t, uint32_t> sampling_interval;

			parse_events_str_for_sample(L"ra,r3c,rdab,rdead,r1a2b,rfedc,r1234", ioctl_events_sample, sampling_interval);

			Assert::AreEqual(ioctl_events_sample.size(), (size_t)7);

			Assert::AreEqual(ioctl_events_sample[0].index, (uint32_t)0xA);
			Assert::AreEqual(ioctl_events_sample[0].interval, PARSE_INTERVAL_DEFAULT);

			Assert::AreEqual(ioctl_events_sample[1].index, (uint32_t)0x3C);
			Assert::AreEqual(ioctl_events_sample[1].interval, PARSE_INTERVAL_DEFAULT);

			Assert::AreEqual(ioctl_events_sample[2].index, (uint32_t)0xDAB);
			Assert::AreEqual(ioctl_events_sample[2].interval, PARSE_INTERVAL_DEFAULT);

			Assert::AreEqual(ioctl_events_sample[3].index, (uint32_t)0x1234);
			Assert::AreEqual(ioctl_events_sample[3].interval, PARSE_INTERVAL_DEFAULT);

			Assert::AreEqual(ioctl_events_sample[4].index, (uint32_t)0x1A2B);
			Assert::AreEqual(ioctl_events_sample[4].interval, PARSE_INTERVAL_DEFAULT);

			Assert::AreEqual(ioctl_events_sample[5].index, (uint32_t)0xDEAD);
			Assert::AreEqual(ioctl_events_sample[5].interval, PARSE_INTERVAL_DEFAULT);

			Assert::AreEqual(ioctl_events_sample[6].index, (uint32_t)0xFEDC);
			Assert::AreEqual(ioctl_events_sample[6].interval, PARSE_INTERVAL_DEFAULT);
		}

		TEST_METHOD(test_parse_raw_events_str_for_sample_r_7_events_interval)
		{
			std::vector<struct evt_sample_src> ioctl_events_sample;
			std::map<uint32_t, uint32_t> sampling_interval;

			parse_events_str_for_sample(L"ra,r3c:10000,rdab,rdead:200000,r1a2b,rfedc:3000000,r1234", ioctl_events_sample, sampling_interval);

			Assert::AreEqual(ioctl_events_sample.size(), (size_t)7);

			Assert::AreEqual(ioctl_events_sample[0].index, (uint32_t)0xA);
			Assert::AreEqual(ioctl_events_sample[0].interval, PARSE_INTERVAL_DEFAULT);

			Assert::AreEqual(ioctl_events_sample[1].index, (uint32_t)0x3C);
			Assert::AreEqual(ioctl_events_sample[1].interval, uint32_t(10000));

			Assert::AreEqual(ioctl_events_sample[2].index, (uint32_t)0xDAB);
			Assert::AreEqual(ioctl_events_sample[2].interval, PARSE_INTERVAL_DEFAULT);

			Assert::AreEqual(ioctl_events_sample[3].index, (uint32_t)0x1234);
			Assert::AreEqual(ioctl_events_sample[3].interval, PARSE_INTERVAL_DEFAULT);

			Assert::AreEqual(ioctl_events_sample[4].index, (uint32_t)0x1A2B);
			Assert::AreEqual(ioctl_events_sample[4].interval, PARSE_INTERVAL_DEFAULT);

			Assert::AreEqual(ioctl_events_sample[5].index, (uint32_t)0xDEAD);
			Assert::AreEqual(ioctl_events_sample[5].interval, uint32_t(200000));

			Assert::AreEqual(ioctl_events_sample[6].index, (uint32_t)0xFEDC);
			Assert::AreEqual(ioctl_events_sample[6].interval, uint32_t(3000000));
		}

		TEST_METHOD(test_parse_raw_events_str_for_sample_misc)
		{
			{
				std::vector<struct evt_sample_src> ioctl_events_sample;
				std::map<uint32_t, uint32_t> sampling_interval;
				parse_events_str_for_sample(L"rffff", ioctl_events_sample, sampling_interval);
			}

			{
				auto wrapper_extra_large_num = [=]() {
					std::vector<struct evt_sample_src> ioctl_events_sample;
					std::map<uint32_t, uint32_t> sampling_interval;
					parse_events_str_for_sample(L"r10000", ioctl_events_sample, sampling_interval);
				};
				Assert::ExpectException<fatal_exception>(wrapper_extra_large_num);
			}

			{
				auto wrapper_extra_large_num_2 = [=]() {
					std::vector<struct evt_sample_src> ioctl_events_sample;
					std::map<uint32_t, uint32_t> sampling_interval;
					parse_events_str_for_sample(L"r1,r10000", ioctl_events_sample, sampling_interval);
				};
				Assert::ExpectException<fatal_exception>(wrapper_extra_large_num_2);
			}
		}
	};

	/****************************************************************************/

	TEST_CLASS(wperftest_parsers_extra_events)
	{
	public:

		TEST_METHOD(test_parse_events_extra_1_event_OK)
		{
			std::map<enum evt_class, std::vector<struct extra_event>> extra_core_events;
			parse_events_extra(L"name1:0x1234", extra_core_events);

			Assert::AreEqual(extra_core_events[EVT_CORE].size(), (size_t)1);

			Assert::IsTrue(extra_core_events[EVT_CORE][0].name == L"name1");
			Assert::IsTrue(extra_core_events[EVT_CORE][0].hdr.evt_class == EVT_CORE);
			Assert::IsTrue(extra_core_events[EVT_CORE][0].hdr.num == 0x1234);
		}

		TEST_METHOD(test_parse_events_extra_1_event_max_val_OK)
		{
			std::map<enum evt_class, std::vector<struct extra_event>> extra_core_events;
			parse_events_extra(L"name1:0xffff", extra_core_events);

			Assert::AreEqual(extra_core_events[EVT_CORE].size(), (size_t)1);

			Assert::IsTrue(extra_core_events[EVT_CORE][0].name == L"name1");
			Assert::IsTrue(extra_core_events[EVT_CORE][0].hdr.evt_class == EVT_CORE);
			Assert::IsTrue(extra_core_events[EVT_CORE][0].hdr.num == 0xffff);
		}

		TEST_METHOD(test_parse_events_extra_2_events_OK)
		{
			std::map<enum evt_class, std::vector<struct extra_event>> extra_core_events;
			parse_events_extra(L"name1:0x1234,name2:0xabcd", extra_core_events);

			Assert::AreEqual(extra_core_events[EVT_CORE].size(), (size_t)2);

			Assert::IsTrue(extra_core_events[EVT_CORE][0].name == L"name1");
			Assert::IsTrue(extra_core_events[EVT_CORE][0].hdr.evt_class == EVT_CORE);
			Assert::IsTrue(extra_core_events[EVT_CORE][0].hdr.num == 0x1234);

			Assert::IsTrue(extra_core_events[EVT_CORE][1].name == L"name2");
			Assert::IsTrue(extra_core_events[EVT_CORE][1].hdr.evt_class == EVT_CORE);
			Assert::IsTrue(extra_core_events[EVT_CORE][1].hdr.num == 0xabcd);
		}

		TEST_METHOD(test_parse_events_extra_3_events_OK)
		{
			std::map<enum evt_class, std::vector<struct extra_event>> extra_core_events;
			parse_events_extra(L"first:0x12,second:FF,third:12a", extra_core_events);

			Assert::AreEqual(extra_core_events[EVT_CORE].size(), (size_t)3);

			Assert::IsTrue(extra_core_events[EVT_CORE][0].name == L"first");
			Assert::IsTrue(extra_core_events[EVT_CORE][0].hdr.evt_class == EVT_CORE);
			Assert::IsTrue(extra_core_events[EVT_CORE][0].hdr.num == 0x12);

			Assert::IsTrue(extra_core_events[EVT_CORE][1].name == L"second");
			Assert::IsTrue(extra_core_events[EVT_CORE][1].hdr.evt_class == EVT_CORE);
			Assert::IsTrue(extra_core_events[EVT_CORE][1].hdr.num == 0xff);

			Assert::IsTrue(extra_core_events[EVT_CORE][2].name == L"third");
			Assert::IsTrue(extra_core_events[EVT_CORE][2].hdr.evt_class == EVT_CORE);
			Assert::IsTrue(extra_core_events[EVT_CORE][2].hdr.num == 0x12a);
		}

		TEST_METHOD(test_parse_events_extra_whitespace_NOK)
		{
			{
				auto wrapper_whitespace_in_name = [=]() {
					std::map<enum evt_class, std::vector<struct extra_event>> extra_core_events;
					parse_events_extra(L"first:0x12,second:FF, third:12a", extra_core_events);;
				};
				Assert::ExpectException<fatal_exception>(wrapper_whitespace_in_name);
			}

			{
				auto wrapper_whitespace_in_num = [=]() {
					std::map<enum evt_class, std::vector<struct extra_event>> extra_core_events;
					parse_events_extra(L"first: 0x12,second:FF,third:12a", extra_core_events);;
				};
				Assert::ExpectException<fatal_exception>(wrapper_whitespace_in_num);
			}

			{
				auto wrapper_whitespace_tab = [=]() {
					std::map<enum evt_class, std::vector<struct extra_event>> extra_core_events;
					parse_events_extra(L"first:0x12,second:FF,third:12a\t", extra_core_events);;
				};
				Assert::ExpectException<fatal_exception>(wrapper_whitespace_tab);
			}
		}

		TEST_METHOD(test_parse_events_extra_delim_NOK)
		{
			{
				auto wrapper_no_delim_in_name = [=]() {
					std::map<enum evt_class, std::vector<struct extra_event>> extra_core_events;
					parse_events_extra(L"first0x12,second:FF,third:12a", extra_core_events);;
				};
				Assert::ExpectException<fatal_exception>(wrapper_no_delim_in_name);
			}

			{
				auto wrapper_no_delim_in_num = [=]() {
					std::map<enum evt_class, std::vector<struct extra_event>> extra_core_events;
					parse_events_extra(L"first:0x12,second:FF,0x1234", extra_core_events);;
				};
				Assert::ExpectException<fatal_exception>(wrapper_no_delim_in_num);
			}
		}

		TEST_METHOD(test_parse_events_extra_format_misc_NOK)
		{
			{
				auto wrapper_empty_str = [=]() {
					std::map<enum evt_class, std::vector<struct extra_event>> extra_core_events;
					parse_events_extra(L"", extra_core_events);;
				};
				Assert::ExpectException<fatal_exception>(wrapper_empty_str);
			}

			{
				auto wrapper_extra_delim = [=]() {
					std::map<enum evt_class, std::vector<struct extra_event>> extra_core_events;
					parse_events_extra(L"first:0x12:", extra_core_events);;
				};
				Assert::ExpectException<fatal_exception>(wrapper_extra_delim);
			}

			{
				auto wrapper_extra_large_num = [=]() {
					std::map<enum evt_class, std::vector<struct extra_event>> extra_core_events;
					parse_events_extra(L"first:0xffffaaaa", extra_core_events);;
				};
				Assert::ExpectException<fatal_exception>(wrapper_extra_large_num);
			}

			{
				auto wrapper_extra_large_num_2 = [=]() {
					std::map<enum evt_class, std::vector<struct extra_event>> extra_core_events;
					parse_events_extra(L"name1:0x1000,name2:0x10000", extra_core_events);;
				};
				Assert::ExpectException<fatal_exception>(wrapper_extra_large_num_2);
			}
		}
	};

	/****************************************************************************/

	TEST_CLASS(wperftest_parsers_counting)
	{
		public:
			TEST_METHOD(test_parse_events_str_EVT_CORE_rf_in_group)
		{
			std::map<enum evt_class, std::deque<struct evt_noted>> events;
			std::map<enum evt_class, std::vector<struct evt_noted>> groups;
			std::wstring note;
			struct pmu_device_cfg pmu_cfg = { 0 };

			pmu_cfg.gpc_nums[EVT_CORE] = 6;	// parse_events_str only uses gpc_nums[]

			parse_events_str(L"{rf}", events, groups, note, pmu_cfg);

			Assert::AreEqual(events[EVT_CORE].size(), (size_t)1);
			Assert::AreEqual(events[EVT_CORE][0].index, (uint16_t)0xF);
		}

		TEST_METHOD(test_parse_events_str_EVT_CORE_2_raw_events_in_group)
		{
			std::map<enum evt_class, std::deque<struct evt_noted>> events;
			std::map<enum evt_class, std::vector<struct evt_noted>> groups;
			std::wstring note;
			struct pmu_device_cfg pmu_cfg = { 0 };

			pmu_cfg.gpc_nums[EVT_CORE] = 6;	// parse_events_str only uses gpc_nums[]

			parse_events_str(L"{r1b,r75}", events, groups, note, pmu_cfg);

			Assert::AreEqual(groups[EVT_CORE].size(), (size_t)3);

			Assert::AreEqual(groups[EVT_CORE][0].index, (uint16_t)0x2);
			Assert::AreEqual(groups[EVT_CORE][1].index, (uint16_t)0x1B);
			Assert::AreEqual(groups[EVT_CORE][2].index, (uint16_t)0x75);

			Assert::IsTrue(groups[EVT_CORE][0].type == EVT_HDR);
			Assert::IsTrue(groups[EVT_CORE][1].type == EVT_GROUPED);
			Assert::IsTrue(groups[EVT_CORE][2].type == EVT_GROUPED);
		}

		TEST_METHOD(test_parse_events_str_EVT_CORE_raw_events_in_group_4)
		{
			std::map<enum evt_class, std::deque<struct evt_noted>> events;
			std::map<enum evt_class, std::vector<struct evt_noted>> groups;
			std::wstring note;
			struct pmu_device_cfg pmu_cfg = { 0 };

			pmu_cfg.gpc_nums[EVT_CORE] = 6;	// parse_events_str only uses gpc_nums[]

			parse_events_str(L"{r1b,r75},r74,r73,r70,r71", events, groups, note, pmu_cfg);

			Assert::AreEqual(events[EVT_CORE].size(), (size_t)4);

			Assert::AreEqual(events[EVT_CORE][0].index, (uint16_t)0x74);
			Assert::AreEqual(events[EVT_CORE][1].index, (uint16_t)0x73);
			Assert::AreEqual(events[EVT_CORE][2].index, (uint16_t)0x70);
			Assert::AreEqual(events[EVT_CORE][3].index, (uint16_t)0x71);

			Assert::AreEqual(groups[EVT_CORE].size(), (size_t)3);

			Assert::AreEqual(groups[EVT_CORE][0].index, (uint16_t)0x2);
			Assert::AreEqual(groups[EVT_CORE][1].index, (uint16_t)0x1B);
			Assert::AreEqual(groups[EVT_CORE][2].index, (uint16_t)0x75);

			Assert::IsTrue(groups[EVT_CORE][0].type == EVT_HDR);
			Assert::IsTrue(groups[EVT_CORE][1].type == EVT_GROUPED);
			Assert::IsTrue(groups[EVT_CORE][2].type == EVT_GROUPED);
		}

		/****************************************************************************/

		TEST_METHOD(test_parse_events_str_EVT_CORE_rf)
		{
			std::map<enum evt_class, std::deque<struct evt_noted>> events;
			std::map<enum evt_class, std::vector<struct evt_noted>> groups;
			std::wstring note;
			struct pmu_device_cfg pmu_cfg = { 0 };

			pmu_cfg.gpc_nums[EVT_CORE] = 6;	// parse_events_str only uses gpc_nums[]

			parse_events_str(L"rf", events, groups, note, pmu_cfg);

			Assert::AreEqual(events[EVT_CORE].size(), (size_t)1);
			Assert::AreEqual(events[EVT_CORE][0].index, (uint16_t)0xF);
		}

		TEST_METHOD(test_parse_events_str_EVT_CORE_r1b)
		{
			std::map<enum evt_class, std::deque<struct evt_noted>> events;
			std::map<enum evt_class, std::vector<struct evt_noted>> groups;
			std::wstring note;
			struct pmu_device_cfg pmu_cfg = { 0 };

			pmu_cfg.gpc_nums[EVT_CORE] = 6;	// parse_events_str only uses gpc_nums[]

			parse_events_str(L"r1b", events, groups, note, pmu_cfg);

			Assert::AreEqual(events[EVT_CORE].size(), (size_t)1);
			Assert::AreEqual(events[EVT_CORE][0].index, (uint16_t)0x1B);
		}

		TEST_METHOD(test_parse_events_str_raw_event_max_num)
		{
			std::map<enum evt_class, std::deque<struct evt_noted>> events;
			std::map<enum evt_class, std::vector<struct evt_noted>> groups;
			std::wstring note;
			struct pmu_device_cfg pmu_cfg = { 0 };

			pmu_cfg.gpc_nums[EVT_CORE] = 6;	// parse_events_str only uses gpc_nums[]

			parse_events_str(L"rffff", events, groups, note, pmu_cfg);

			Assert::AreEqual(events[EVT_CORE].size(), (size_t)1);
			Assert::AreEqual(events[EVT_CORE][0].index, (uint16_t)0xFFFF);
		}

		TEST_METHOD(test_parse_events_str_EVT_CORE_r_7_events)
		{
			std::map<enum evt_class, std::deque<struct evt_noted>> events;
			std::map<enum evt_class, std::vector<struct evt_noted>> groups;
			std::wstring note;
			struct pmu_device_cfg pmu_cfg = { 0 };

			pmu_cfg.gpc_nums[EVT_CORE] = 6;	// parse_events_str only uses gpc_nums[]

			parse_events_str(L"ra,r3c,rdab,rdead,r1a2b,rfedc,r1234", events, groups, note, pmu_cfg);

			Assert::AreEqual(events[EVT_CORE].size(), (size_t)7);

			Assert::AreEqual(events[EVT_CORE][0].index, (uint16_t)0xA);
			Assert::AreEqual(events[EVT_CORE][1].index, (uint16_t)0x3C);
			Assert::AreEqual(events[EVT_CORE][2].index, (uint16_t)0xDAB);
			Assert::AreEqual(events[EVT_CORE][3].index, (uint16_t)0xDEAD);
			Assert::AreEqual(events[EVT_CORE][4].index, (uint16_t)0x1A2B);
			Assert::AreEqual(events[EVT_CORE][5].index, (uint16_t)0xFEDC);
			Assert::AreEqual(events[EVT_CORE][6].index, (uint16_t)0x1234);
		}

		TEST_METHOD(test_parse_events_str_EVT_DSU_1_event)
		{
			std::map<enum evt_class, std::deque<struct evt_noted>> events;
			std::map<enum evt_class, std::vector<struct evt_noted>> groups;
			std::wstring note;
			struct pmu_device_cfg pmu_cfg = { 0 };

			pmu_cfg.gpc_nums[EVT_CORE] = 6;	// parse_events_str only uses gpc_nums[]
			pmu_cfg.gpc_nums[EVT_DSU] = 6;

			parse_events_str(L"/dsu/l3d_cache_refill", events, groups, note, pmu_cfg);

			Assert::AreEqual(events[EVT_CORE].size(), (size_t)0);
			Assert::AreEqual(events[EVT_DSU].size(), (size_t)1);

			Assert::AreEqual(events[EVT_DSU][0].index, (uint16_t)0x2A);
		}

		TEST_METHOD(test_parse_events_str_EVT_DSU_2_events)
		{
			std::map<enum evt_class, std::deque<struct evt_noted>> events;
			std::map<enum evt_class, std::vector<struct evt_noted>> groups;
			std::wstring note;
			struct pmu_device_cfg pmu_cfg = { 0 };

			pmu_cfg.gpc_nums[EVT_CORE] = 6;	// parse_events_str only uses gpc_nums[]
			pmu_cfg.gpc_nums[EVT_DSU] = 6;

			parse_events_str(L"/dsu/l3d_cache,/dsu/l3d_cache_refill", events, groups, note, pmu_cfg);

			Assert::AreEqual(events[EVT_CORE].size(), (size_t)0);
			Assert::AreEqual(events[EVT_DSU].size(), (size_t)2);

			Assert::AreEqual(events[EVT_DSU][0].index, (uint16_t)0x2B);
			Assert::AreEqual(events[EVT_DSU][1].index, (uint16_t)0x2A);
		}

		TEST_METHOD(test_parse_events_str_EVT_DSU_2_events_case_insensitive_prefix)
		{
			std::map<enum evt_class, std::deque<struct evt_noted>> events;
			std::map<enum evt_class, std::vector<struct evt_noted>> groups;
			std::wstring note;
			struct pmu_device_cfg pmu_cfg = { 0 };

			pmu_cfg.gpc_nums[EVT_CORE] = 6;	// parse_events_str only uses gpc_nums[]
			pmu_cfg.gpc_nums[EVT_DSU] = 6;

			parse_events_str(L"/DSU/l3d_cache,/Dsu/l3d_cache_refill", events, groups, note, pmu_cfg);

			Assert::AreEqual(events[EVT_CORE].size(), (size_t)0);
			Assert::AreEqual(events[EVT_DSU].size(), (size_t)2);

			Assert::AreEqual(events[EVT_DSU][0].index, (uint16_t)0x2B);
			Assert::AreEqual(events[EVT_DSU][1].index, (uint16_t)0x2A);
		}

		TEST_METHOD(test_parse_events_str_EVT_DSU_2_events_uppercase)
		{
			std::map<enum evt_class, std::deque<struct evt_noted>> events;
			std::map<enum evt_class, std::vector<struct evt_noted>> groups;
			std::wstring note;
			struct pmu_device_cfg pmu_cfg = { 0 };

			pmu_cfg.gpc_nums[EVT_CORE] = 6;	// parse_events_str only uses gpc_nums[]
			pmu_cfg.gpc_nums[EVT_DSU] = 6;

			parse_events_str(L"/DSU/L3D_CACHE,/DSU/L3D_CACHE_REFILL", events, groups, note, pmu_cfg);

			Assert::AreEqual(events[EVT_CORE].size(), (size_t)0);
			Assert::AreEqual(events[EVT_DSU].size(), (size_t)2);

			Assert::AreEqual(events[EVT_DSU][0].index, (uint16_t)0x2B);
			Assert::AreEqual(events[EVT_DSU][1].index, (uint16_t)0x2A);
		}

		TEST_METHOD(test_parse_events_str_EVT_DMC_CLKDIV2_1_event)
		{
			std::map<enum evt_class, std::deque<struct evt_noted>> events;
			std::map<enum evt_class, std::vector<struct evt_noted>> groups;
			std::wstring note;
			struct pmu_device_cfg pmu_cfg = { 0 };

			pmu_cfg.gpc_nums[EVT_CORE] = 6;	// parse_events_str only uses gpc_nums[]
			pmu_cfg.gpc_nums[EVT_DSU] = 6;
			pmu_cfg.gpc_nums[EVT_DMC_CLK] = 2;
			pmu_cfg.gpc_nums[EVT_DMC_CLKDIV2] = 8;

			parse_events_str(L"/dmc_clkdiv2/rdwr", events, groups, note, pmu_cfg);

			Assert::AreEqual(events[EVT_CORE].size(), (size_t)0);
			Assert::AreEqual(events[EVT_DSU].size(), (size_t)0);
			Assert::AreEqual(events[EVT_DMC_CLK].size(), (size_t)0);
			Assert::AreEqual(events[EVT_DMC_CLKDIV2].size(), (size_t)1);

			Assert::AreEqual(events[EVT_DMC_CLKDIV2][0].index, (uint16_t)0x12);
		}

		TEST_METHOD(test_parse_events_str_EVT_DMC_CLKDIV2_1_event_uppercase)
		{
			std::map<enum evt_class, std::deque<struct evt_noted>> events;
			std::map<enum evt_class, std::vector<struct evt_noted>> groups;
			std::wstring note;
			struct pmu_device_cfg pmu_cfg = { 0 };

			pmu_cfg.gpc_nums[EVT_CORE] = 6;	// parse_events_str only uses gpc_nums[]
			pmu_cfg.gpc_nums[EVT_DSU] = 6;
			pmu_cfg.gpc_nums[EVT_DMC_CLK] = 2;
			pmu_cfg.gpc_nums[EVT_DMC_CLKDIV2] = 8;

			parse_events_str(L"/dmc_clkdiv2/RDWR", events, groups, note, pmu_cfg);

			Assert::AreEqual(events[EVT_CORE].size(), (size_t)0);
			Assert::AreEqual(events[EVT_DSU].size(), (size_t)0);
			Assert::AreEqual(events[EVT_DMC_CLK].size(), (size_t)0);
			Assert::AreEqual(events[EVT_DMC_CLKDIV2].size(), (size_t)1);

			Assert::AreEqual(events[EVT_DMC_CLKDIV2][0].index, (uint16_t)0x12);
		}

		TEST_METHOD(test_parse_events_str_EVT_DMC_CLKDIV2_1_event_prefix_uppercase)
		{
			std::map<enum evt_class, std::deque<struct evt_noted>> events;
			std::map<enum evt_class, std::vector<struct evt_noted>> groups;
			std::wstring note;
			struct pmu_device_cfg pmu_cfg = { 0 };

			pmu_cfg.gpc_nums[EVT_CORE] = 6;	// parse_events_str only uses gpc_nums[]
			pmu_cfg.gpc_nums[EVT_DSU] = 6;
			pmu_cfg.gpc_nums[EVT_DMC_CLK] = 2;
			pmu_cfg.gpc_nums[EVT_DMC_CLKDIV2] = 8;

			parse_events_str(L"/DMC_CLKDIV2/rdwr", events, groups, note, pmu_cfg);

			Assert::AreEqual(events[EVT_CORE].size(), (size_t)0);
			Assert::AreEqual(events[EVT_DSU].size(), (size_t)0);
			Assert::AreEqual(events[EVT_DMC_CLK].size(), (size_t)0);
			Assert::AreEqual(events[EVT_DMC_CLKDIV2].size(), (size_t)1);

			Assert::AreEqual(events[EVT_DMC_CLKDIV2][0].index, (uint16_t)0x12);
		}

		TEST_METHOD(test_parse_events_str_misc)
		{
			{
				auto wrapper_extra_large_num = [=]() {
					std::map<enum evt_class, std::deque<struct evt_noted>> events;
					std::map<enum evt_class, std::vector<struct evt_noted>> groups;
					std::wstring note;
					struct pmu_device_cfg pmu_cfg = { 0 };

					pmu_cfg.gpc_nums[EVT_CORE] = 6;	// parse_events_str only uses gpc_nums[]

					parse_events_str(L"r10000", events, groups, note, pmu_cfg);
				};
				Assert::ExpectException<fatal_exception>(wrapper_extra_large_num);
			}

			{
				auto wrapper_extra_large_num_2 = [=]() {
					std::map<enum evt_class, std::deque<struct evt_noted>> events;
					std::map<enum evt_class, std::vector<struct evt_noted>> groups;
					std::wstring note;
					struct pmu_device_cfg pmu_cfg = { 0 };

					pmu_cfg.gpc_nums[EVT_CORE] = 6;	// parse_events_str only uses gpc_nums[]

					parse_events_str(L"r1,r10000", events, groups, note, pmu_cfg);
				};
				Assert::ExpectException<fatal_exception>(wrapper_extra_large_num_2);
			}
		}
	};
}
