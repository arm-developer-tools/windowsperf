#pragma once
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

#include <assert.h>
#include <algorithm>
#include <iostream>
#include <map>
#include <string>
#include <vector>
#include <iomanip>
#include "outpututil.h"

/// <summary>
/// Simple implementation of table with columns and rows.
/// This implementation focuses on displaying `wperf stat`
/// outputs.
///
/// Example usage:
///
/// WindowsPerfPrettyTable table;
/// table.AddColumn(L"counter value", { L"48759549",  L"460333515", L"433565",
///										L"4584058", L"428984802", L"411258067",
///										L"49864771", L"410906825", L"4361" },
///					PrettyTable::RIGHT);
/// table.AddColumn(L"event name", { L"cycle", L"inst_spec", L"vfp_spec",
///									 L"ase_spec", L"dp_spec", L"ld_spec",
///									 L"st_spec", L"br_immed_spec", L"crypto_spec" });
/// table.AddColumn(L"event idx", { L"fixed", L"0x1b", L"0x75",
///									L"0x74", L"0x73", L"0x70",
///									L"0x71", L"0x78", L"0x77" });
/// table.AddColumn(L"multiplexd", { L"17/17", L"13/17", L"13/17",
///									 L"13/17", L"13/17", L"13/17",
///									 L"13/17", L"12/17", L"12/17" },
///					PrettyTable::RIGHT);
/// table.AddColumn(L"scaled value", { L"48759549", L"78897673", L"43892",
///									   L"763768", L"37903202", L"14722087",
///									   L"12900085", L"15451335", L"511" },
///					PrettyTable::RIGHT);
///
/// table.Print();
///
///
/// On-screen output::
///
///              +--------------------------------------+-------------+---- PrettyTable::RIGHT
///              |                                      |             |
///        counter value  event name     event idx  multiplexd  scaled value		<--- m_header
///        =============  ==========     =========  ==========  ============		<--- m_header_underline(true)
///             48759549  cycle          fixed           17/17      48759549
///            460333515  inst_spec      0x1b            13/17      78897673
///               433565  vfp_spec       0x75            13/17         43892
///              4584058  ase_spec       0x74            13/17        763768
///            428984802  dp_spec        0x73            13/17      37903202
///            411258067  ld_spec        0x70            13/17      14722087
///             49864771  st_spec        0x71            13/17      12900085
///            410906825  br_immed_spec  0x78            12/17      15451335
///                 4361  crypto_spec    0x77            12/17           511
///
///                       |___________|
///                             |
///	                            +--- table.AddColumn("event name",
///													 {{L"cycle", L"inst_spec", L"vfp_spec",
///													   L"ase_spec", L"dp_spec", L"ld_spec",
///                                                    L"st_spec", L"br_immed_spec", L"crypto_spec"});
///
/// The template argument CharType defines if the string outputs will be done on normal or wide strings.
/// 
/// </summary>
template <typename CharType=wchar_t>
class PrettyTable
{
	/// String type to use based on current Char type template argument
	typedef typename std::conditional_t<std::is_same_v<CharType, char>, std::string, std::wstring> StringType;
	/// Output Stream type to use based on current Char type template argument
	typedef typename std::conditional_t<std::is_same_v<CharType, char>, std::ostream, std::wostream> OutputStream;
	/// String Stream type to use based on current Char type template argument
	typedef typename std::conditional_t<std::is_same_v<CharType, char>, std::stringstream, std::wstringstream> StringStream;

public:
	/// <summary>
	/// Column text align (left or right)
	/// </summary>
	enum ColumnAlign
	{
		LEFT,		/// Align left std::wcout with std::left
		RIGHT		/// Align left std::wcout with td::right
	};
    
public:
	/// <summary>
	/// Constructor of pretty table
	/// </summary>
	PrettyTable()
		: m_header_underline(true) {
	};

	/// operator<< overload to handle printing to stream output.
	friend OutputStream& operator<<(OutputStream &os, PrettyTable& pt)
	{
		// Clearing out the stream before proceding. No need to clear bits as none are used here.
		pt.m_out_stream.str(LITERALCONSTANTS_GET(""));
		// Calculating the output.
		pt.Print();
		os << pt.m_out_stream.str();
		return os;
	}

	/// <summary>
	/// Define if you want to underline column headers with '='
	/// </summary>
	/// <param name="value">TRUE if underline (default), FALSE to disable underline</param>
	void SetHeaderUnderline(bool Value) {
		m_header_underline = Value;
	}

	// Variadic function stop case.
	template <int I = 0>
	void Insert() {}

	// Variadic function to insert arbitrary number of columns with column types. Currently this only handles StringTypes but will be expanded in the future.
	template <int I = 0, typename T1, typename... Ts>
	void Insert(T1 arg1, Ts... args)
	{
		if(m_header.size() > I)
		{
			m_table[m_header[I]] = arg1;
			Insert<I+1>(args...);
		}
	}

	// Individually set the Alignment of column ColumnIndex.
	void SetAlignment(int ColumnIndex, enum ColumnAlign Align)
	{
		assert(ColumnIndex < m_columns_align.size());
		m_columns_align[ColumnIndex] = Align;
	}

	// Add a column to the table with header and alignment only.
	void AddColumn(StringType ColumnName, enum ColumnAlign Align = LEFT)
	{
		m_header.push_back(ColumnName);
		m_columns_align.push_back(Align);
	}

	/// <summary>
	/// Add column to the table. Columns will be added from left to right.
	/// </summary>
	/// <param name="ColumnName">Name of the column</param>
	/// <param name="ColumnValues">VECTOR of WSTRING with column text values</param>
	/// <param name="Align">Align whole column to LEFT or RIGHT</param>
	void AddColumn(StringType ColumnName, std::vector<StringType> ColumnValues, enum ColumnAlign Align = LEFT)
	{
		m_header.push_back(ColumnName);
		m_columns_align.push_back(Align);
		m_table[ColumnName] = ColumnValues;
	}

	/// <summary>
	/// Print pretty table on screen with std::wcout
	/// </summary>
	void Print()
	{
		PrintLeftMargin();

		// Print colum names (headers)
		size_t j = 0;	// Rows in column index
		for (auto header : m_header) {
			// minwindef.h define a macro called max making it impossible to use std::max without undefining the macro first
			#undef max

			// Calculate maximum width of each column
			size_t max = GetColumnMaxWidth(m_table[header]);
			m_columns_max_width.push_back(std::max(max, header.size()));

			// Calculate length for each column
			m_columns_length.push_back(m_table[header].size());

			PrintColumnSeparator();
			m_out_stream << std::setw(max);
			PrintColumnAlign(m_columns_align[j]);
			m_out_stream << header;
			j++;
		}
		m_out_stream << std::endl;

		// Print (if enabled) column name underline
		if (m_header_underline) {
			PrintLeftMargin();

			j = 0;	// Rows in column index
			for (auto header : m_header) {
				PrintColumnSeparator();
				m_out_stream << std::setw(m_columns_max_width[j]);
				PrintColumnAlign(m_columns_align[j]);
				m_out_stream << StringType(header.length(), LiteralConstants<CharType>::m_equal[0]);
				j++;
			}
			m_out_stream << std::endl;
		}

		// Print columns
		auto iterator = std::max_element(m_columns_length.begin(), m_columns_length.end());
		size_t max = *iterator;		// How many rows in longest column
		size_t i = 0;	// Rows in column

		while (i < max) {
			PrintLeftMargin();
			j = 0;	// Index of column (from left)
			for (auto header : m_header) {
				auto column = m_table[header];
				PrintColumnSeparator();
				m_out_stream << std::setw(m_columns_max_width[j]);
				PrintColumnAlign(m_columns_align[j]);
				m_out_stream << column[i];
				j++;
			}
			i++;
			m_out_stream << std::flush << std::endl;
		}
	}

private:
	/// <summary>
	/// Left margin for whole pretty table from beggining of the screen
	/// </summary>
	void PrintLeftMargin() {
		m_out_stream << StringType(m_LEFT_MARGIN, LiteralConstants<CharType>::m_space[0]);
	}

	/// <summary>
	/// Print white space column separation.
	/// </summary>
	void PrintColumnSeparator() {
		m_out_stream << StringType(m_COLUMN_SEPARATOR, LiteralConstants<CharType>::m_space[0]);
	}

	/// <summary>
	/// Control how std::wcout aligns text.
	/// </summary>
	/// <param name="Align">LEFT or RIGHT</param>
	void PrintColumnAlign(ColumnAlign Align) {
		switch (Align) {
		case LEFT:  m_out_stream << std::left; break;
		case RIGHT: m_out_stream << std::right; break;
		}
	}

	/// <summary>
	/// Get maximal width of column. Calculate for every row in
	/// column its width and get max of it.
	/// </summary>
	/// <param name="Column">VECTOR of WSTRINGS with rows</param>
	/// <returns>Length of longest column cell in Column</returns>
	size_t GetColumnMaxWidth(std::vector<StringType> Column)
	{
		auto max_len = std::max_element(Column.begin(), Column.end(), [](PrettyTable<CharType>::StringType a, PrettyTable<CharType>::StringType b) {
			return a.length() < b.length();
			});
		return (*max_len).length();
	}

	// Consts
	const size_t m_LEFT_MARGIN = 6;
	const size_t m_COLUMN_SEPARATOR = 2;

	// Class members
	StringStream m_out_stream;
	std::map<StringType,
		     std::vector<StringType>> m_table;	// Table where key is column name and value is VECTOR of WSTRINGs
	std::vector<StringType> m_header;				// VECTOR of column names (header) WSTRINGs
	bool m_header_underline;						// Enable header underlining, default true
	std::vector<size_t> m_columns_max_width;		// Max width of each column, index with int(rown number)
	std::vector<size_t> m_columns_length;			// Length (how many rows) for each rows, index with int(rown number)
	std::vector<ColumnAlign> m_columns_align;		// Column align (left, right), index with int(rown number)
};
