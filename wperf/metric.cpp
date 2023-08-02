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

#include <algorithm>
#include <iomanip>
#include <list>
#include <map>
#include <stack>
#include <string>
#include <sstream>
#include <vector>


// Predefined simple metrics
static std::vector<std::wstring> imix = { L"inst_spec", L"dp_spec", L"vfp_spec", L"ase_spec", L"ld_spec", L"st_spec" };
static std::vector<std::wstring> icache = { L"l1i_cache", L"l1i_cache_refill", L"l2i_cache", L"l2i_cache_refill", L"inst_retired" };
static std::vector<std::wstring> dcache = { L"l1d_cache", L"l1d_cache_refill", L"l2d_cache", L"l2d_cache_refill", L"inst_retired" };
static std::vector<std::wstring> itlb = { L"l1i_tlb", L"l1i_tlb_refill", L"l2i_tlb", L"l2i_tlb_refill", L"inst_retired" };
static std::vector<std::wstring> dtlb = { L"l1d_tlb", L"l1d_tlb_refill", L"l2d_tlb", L"l2d_tlb_refill", L"inst_retired" };

// Map names of metric to its definition
static std::map<std::wstring, std::vector<std::wstring>> metric_builtin = {

    {std::wstring(L"imix"),     imix},
    {std::wstring(L"icache"),   icache},
    {std::wstring(L"dcache"),   dcache},
    {std::wstring(L"itlb"),     itlb},
    {std::wstring(L"dtlb"),     dtlb},
};


std::vector<std::wstring> metric_get_builtin_metric_names()
{
    std::vector<std::wstring> result;

    for (const auto& [name, _] : metric_builtin)
        result.push_back(name);
    return result;
}

std::wstring metric_gen_metric_based_on_gpc_num(std::wstring name, uint8_t gpc_num)
{
    std::wstring result;

    if (metric_builtin.count(name))
    {
        for (uint8_t i = 0; i < gpc_num; i++)
        {
            if (i >= metric_builtin[name].size())
                break;

            if (result.empty())
                result = L"{" + metric_builtin[name][i];
            else
                result += L"," + metric_builtin[name][i];
        }

        if (gpc_num)
            result += L"}";
    }

    return result;
}

bool metris_token_is_operator(const std::wstring op)
{
    static std::vector<std::wstring> operators = { L"*", L"/", L"+", L"-" };
    return std::find(operators.begin(), operators.end(), op) != operators.end();
}

double metric_calculate_shunting_yard_expression(const std::map<std::wstring, double>& vars, const std::wstring& formula_sy)
{
    // Tokenize formula
    std::list<std::wstring> tokens;
    std::wstring token;
    std::wistringstream ss(formula_sy);

    while (std::getline(ss, token, L' '))
        tokens.push_back(token);

    // Calculate SY formula
    std::stack<double> stack;
    while (tokens.size())
    {
        std::wstring t = tokens.front();
        tokens.pop_front();

        if (metris_token_is_operator(t))
        {
            double y = stack.top(); stack.pop();
            double x = stack.top(); stack.pop();
            double val = x; // x OP y

            switch (t[0])
            {
            case L'*': val *= y; break;
            case L'/':
                if (y == 0)     // To avoid division by zero we return 0
                    return 0;
                val /= y;
                break;
            case L'+': val += y; break;
            case L'-': val -= y; break;
            }

            stack.push(val);
        }
        else
        {
            if (vars.count(t))
                stack.push(vars.at(t));
            else
                stack.push(_wtof(t.c_str()));
        }
    }

    return stack.top();
}
