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

#include "wperf/metric.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace wperftest_metric
{
	TEST_CLASS(wperftest_metric_builtins)
	{
	public:

		TEST_METHOD(test_metric_builtin_imix)
		{
			Assert::AreEqual(metric_gen_metric_based_on_gpc_num(L"imix", 0), std::wstring(L""));
			Assert::AreEqual(metric_gen_metric_based_on_gpc_num(L"imix", 1), std::wstring(L"{inst_spec}"));
			Assert::AreEqual(metric_gen_metric_based_on_gpc_num(L"imix", 2), std::wstring(L"{inst_spec,dp_spec}"));
			Assert::AreEqual(metric_gen_metric_based_on_gpc_num(L"imix", 3), std::wstring(L"{inst_spec,dp_spec,vfp_spec}"));
			Assert::AreEqual(metric_gen_metric_based_on_gpc_num(L"imix", 4), std::wstring(L"{inst_spec,dp_spec,vfp_spec,ase_spec}"));
			Assert::AreEqual(metric_gen_metric_based_on_gpc_num(L"imix", 5), std::wstring(L"{inst_spec,dp_spec,vfp_spec,ase_spec,ld_spec}"));
			Assert::AreEqual(metric_gen_metric_based_on_gpc_num(L"imix", 6), std::wstring(L"{inst_spec,dp_spec,vfp_spec,ase_spec,ld_spec,st_spec}"));
			Assert::AreEqual(metric_gen_metric_based_on_gpc_num(L"imix", 7), std::wstring(L"{inst_spec,dp_spec,vfp_spec,ase_spec,ld_spec,st_spec}"));
		}

		TEST_METHOD(test_metric_builtin_icache)
		{
			Assert::AreEqual(metric_gen_metric_based_on_gpc_num(L"icache", 0), std::wstring(L""));
			Assert::AreEqual(metric_gen_metric_based_on_gpc_num(L"icache", 1), std::wstring(L"{l1i_cache}"));
			Assert::AreEqual(metric_gen_metric_based_on_gpc_num(L"icache", 2), std::wstring(L"{l1i_cache,l1i_cache_refill}"));
			Assert::AreEqual(metric_gen_metric_based_on_gpc_num(L"icache", 3), std::wstring(L"{l1i_cache,l1i_cache_refill,l2i_cache}"));
			Assert::AreEqual(metric_gen_metric_based_on_gpc_num(L"icache", 4), std::wstring(L"{l1i_cache,l1i_cache_refill,l2i_cache,l2i_cache_refill}"));
			Assert::AreEqual(metric_gen_metric_based_on_gpc_num(L"icache", 5), std::wstring(L"{l1i_cache,l1i_cache_refill,l2i_cache,l2i_cache_refill,inst_retired}"));
			Assert::AreEqual(metric_gen_metric_based_on_gpc_num(L"icache", 6), std::wstring(L"{l1i_cache,l1i_cache_refill,l2i_cache,l2i_cache_refill,inst_retired}"));
			Assert::AreEqual(metric_gen_metric_based_on_gpc_num(L"icache", 7), std::wstring(L"{l1i_cache,l1i_cache_refill,l2i_cache,l2i_cache_refill,inst_retired}"));
		}

		TEST_METHOD(test_metric_builtin_dcache)
		{
			Assert::AreEqual(metric_gen_metric_based_on_gpc_num(L"dcache", 0), std::wstring(L""));
			Assert::AreEqual(metric_gen_metric_based_on_gpc_num(L"dcache", 1), std::wstring(L"{l1d_cache}"));
			Assert::AreEqual(metric_gen_metric_based_on_gpc_num(L"dcache", 2), std::wstring(L"{l1d_cache,l1d_cache_refill}"));
			Assert::AreEqual(metric_gen_metric_based_on_gpc_num(L"dcache", 3), std::wstring(L"{l1d_cache,l1d_cache_refill,l2d_cache}"));
			Assert::AreEqual(metric_gen_metric_based_on_gpc_num(L"dcache", 4), std::wstring(L"{l1d_cache,l1d_cache_refill,l2d_cache,l2d_cache_refill}"));
			Assert::AreEqual(metric_gen_metric_based_on_gpc_num(L"dcache", 5), std::wstring(L"{l1d_cache,l1d_cache_refill,l2d_cache,l2d_cache_refill,inst_retired}"));
			Assert::AreEqual(metric_gen_metric_based_on_gpc_num(L"dcache", 6), std::wstring(L"{l1d_cache,l1d_cache_refill,l2d_cache,l2d_cache_refill,inst_retired}"));
			Assert::AreEqual(metric_gen_metric_based_on_gpc_num(L"dcache", 7), std::wstring(L"{l1d_cache,l1d_cache_refill,l2d_cache,l2d_cache_refill,inst_retired}"));
		}

		TEST_METHOD(test_metric_builtin_itlb)
		{
			Assert::AreEqual(metric_gen_metric_based_on_gpc_num(L"itlb", 0), std::wstring(L""));
			Assert::AreEqual(metric_gen_metric_based_on_gpc_num(L"itlb", 1), std::wstring(L"{l1i_tlb}"));
			Assert::AreEqual(metric_gen_metric_based_on_gpc_num(L"itlb", 2), std::wstring(L"{l1i_tlb,l1i_tlb_refill}"));
			Assert::AreEqual(metric_gen_metric_based_on_gpc_num(L"itlb", 3), std::wstring(L"{l1i_tlb,l1i_tlb_refill,l2i_tlb}"));
			Assert::AreEqual(metric_gen_metric_based_on_gpc_num(L"itlb", 4), std::wstring(L"{l1i_tlb,l1i_tlb_refill,l2i_tlb,l2i_tlb_refill}"));
			Assert::AreEqual(metric_gen_metric_based_on_gpc_num(L"itlb", 5), std::wstring(L"{l1i_tlb,l1i_tlb_refill,l2i_tlb,l2i_tlb_refill,inst_retired}"));
			Assert::AreEqual(metric_gen_metric_based_on_gpc_num(L"itlb", 6), std::wstring(L"{l1i_tlb,l1i_tlb_refill,l2i_tlb,l2i_tlb_refill,inst_retired}"));
			Assert::AreEqual(metric_gen_metric_based_on_gpc_num(L"itlb", 7), std::wstring(L"{l1i_tlb,l1i_tlb_refill,l2i_tlb,l2i_tlb_refill,inst_retired}"));
		}

		TEST_METHOD(test_metric_builtin_dtlb)
		{
			Assert::AreEqual(metric_gen_metric_based_on_gpc_num(L"dtlb", 0), std::wstring(L""));
			Assert::AreEqual(metric_gen_metric_based_on_gpc_num(L"dtlb", 1), std::wstring(L"{l1d_tlb}"));
			Assert::AreEqual(metric_gen_metric_based_on_gpc_num(L"dtlb", 2), std::wstring(L"{l1d_tlb,l1d_tlb_refill}"));
			Assert::AreEqual(metric_gen_metric_based_on_gpc_num(L"dtlb", 3), std::wstring(L"{l1d_tlb,l1d_tlb_refill,l2d_tlb}"));
			Assert::AreEqual(metric_gen_metric_based_on_gpc_num(L"dtlb", 4), std::wstring(L"{l1d_tlb,l1d_tlb_refill,l2d_tlb,l2d_tlb_refill}"));
			Assert::AreEqual(metric_gen_metric_based_on_gpc_num(L"dtlb", 5), std::wstring(L"{l1d_tlb,l1d_tlb_refill,l2d_tlb,l2d_tlb_refill,inst_retired}"));
			Assert::AreEqual(metric_gen_metric_based_on_gpc_num(L"dtlb", 6), std::wstring(L"{l1d_tlb,l1d_tlb_refill,l2d_tlb,l2d_tlb_refill,inst_retired}"));
			Assert::AreEqual(metric_gen_metric_based_on_gpc_num(L"dtlb", 7), std::wstring(L"{l1d_tlb,l1d_tlb_refill,l2d_tlb,l2d_tlb_refill,inst_retired}"));
		}

		// Shunting Yard Algorithm calculation
		TEST_METHOD(test_metric_SY_Algorithm_calculations_bad_speculation)
		{
			std::wstring formula_sy = L"100 1 op_retired op_spec / - 1 stall_slot cpu_cycles 8 * / - * br_mis_pred 4 * cpu_cycles / + *";

			std::map<std::wstring, double> vars = {
				{std::wstring(L"op_retired"), 517},
				{std::wstring(L"op_spec"), 2468},
				{std::wstring(L"stall_slot"), 709},
				{std::wstring(L"cpu_cycles"), 2456},
				{std::wstring(L"br_mis_pred"), 1733},
			};


			double op_retired = vars[L"op_retired"];
			double op_spec = vars[L"op_spec"];;
			double stall_slot = vars[L"stall_slot"];;
			double cpu_cycles = vars[L"cpu_cycles"];;
			double br_mis_pred = vars[L"br_mis_pred"];;

			double val_compute = (100 * (((1 - (op_retired / op_spec)) * (1 - (stall_slot / (cpu_cycles * 8)))) + ((br_mis_pred * 4) / cpu_cycles)));
			double val_sy = metric_calculate_shunting_yard_expression(vars, formula_sy);

			Assert::AreEqual(val_compute, val_sy);
		}

		TEST_METHOD(test_metric_SY_Algorithm_calculations_retiring)
		{
			std::wstring formula_sy = L"100 op_retired op_spec / * 1 stall_slot cpu_cycles 8 * / - *";

			std::map<std::wstring, double> vars = {
				{std::wstring(L"cpu_cycles"), 7},
				{std::wstring(L"op_retired"), 2},
				{std::wstring(L"op_spec"), 3},
				{std::wstring(L"stall_slot"), 4},
			};

			double cpu_cycles = vars[L"cpu_cycles"];
			double op_retired = vars[L"op_retired"];;
			double op_spec = vars[L"op_spec"];;
			double stall_slot = vars[L"stall_slot"];;

			double val_compute = ((100 * (op_retired / op_spec)) * (1 - (stall_slot / (cpu_cycles * 8))));
			double val_sy = metric_calculate_shunting_yard_expression(vars, formula_sy);

			Assert::AreEqual(val_compute, val_sy);
		}

		TEST_METHOD(test_metric_SY_Algorithm_calculations_load_percentage)
		{
			std::wstring formula_sy = L"ld_spec inst_spec / 100 *";

			{
				std::map<std::wstring, double> vars = {
					{std::wstring(L"ld_spec"), 7},
					{std::wstring(L"inst_spec"), 2},
				};

				double ld_spec = vars[L"ld_spec"];
				double inst_spec = vars[L"inst_spec"];;

				double val_compute = ((ld_spec / inst_spec) * 100);
				double val_sy = metric_calculate_shunting_yard_expression(vars, formula_sy);

				Assert::AreEqual(val_compute, val_sy);
			}

			{
				std::map<std::wstring, double> vars = {
					{std::wstring(L"ld_spec"), 123456},
					{std::wstring(L"inst_spec"), 7890},
				};

				double ld_spec = vars[L"ld_spec"];
				double inst_spec = vars[L"inst_spec"];;

				double val_compute = ((ld_spec / inst_spec) * 100);
				double val_sy = metric_calculate_shunting_yard_expression(vars, formula_sy);

				Assert::AreEqual(val_compute, val_sy);
			}

		}
	};
}
