#pragma once
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

#include <assert.h>
#include <algorithm>
#include <iostream>
#include <map>
#include <string>
#include <vector>
#include <iomanip>
#include <variant>
#include "outpututil.h"

// minwindef.h define a macro called max making it impossible to use std::max without undefining the macro first
#undef max

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

	PrettyTable(const PrettyTable<CharType>& root) : m_header_underline(root.m_header_underline)
	{
		m_columns_align = root.m_columns_align;
		m_columns_length = root.m_columns_length;
		m_columns_max_width = root.m_columns_max_width;
		m_header = root.m_header;
		m_table = root.m_table;
	};

	PrettyTable<CharType>& operator=(const PrettyTable<CharType>& rhs)
	{
		m_columns_align = rhs.m_columns_align;
		m_columns_length = rhs.m_columns_length;
		m_columns_max_width = rhs.m_columns_max_width;
		m_header = rhs.m_header;
		m_table = rhs.m_table;
		return *this;
	}

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

	OutputStream& GetManOutputStream(OutputStream& os, PrettyTable& pt)
	{
		// Clearing out the stream before proceding. No need to clear bits as none are used here.
		pt.m_out_stream.str(LITERALCONSTANTS_GET(""));
		// Calculating the output.
		pt.PrintMan();
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

	template <int I, typename T>
	void InsertSingle(T& val)
	{
		if constexpr (std::is_same_v<T, std::vector<StringType>>)
		{
			for (auto& item : val)
			{
				m_table[m_header[I]].push_back(item);
			}
		} else {
			for (auto& elem : val)
			{
				if constexpr (std::is_same_v<T, std::vector<uint64_t>>)
				{
					m_table[m_header[I]].push_back(IntToDecWithCommas(elem));
				}
				else if constexpr (std::is_same_v<T, std::vector<double>>) {
					m_table[m_header[I]].push_back(DoubleToWideString(elem));
				}
				else if constexpr (std::is_same_v<T, std::vector<uint32_t>>) {
					m_table[m_header[I]].push_back(std::to_wstring(elem));
				}
				else if constexpr (std::is_same_v<T, std::vector<float>>) {
					m_table[m_header[I]].push_back(std::to_wstring(elem));
				}
				else if constexpr (std::is_same_v<T, std::vector<PrettyTable<CharType>>>)
				{
					m_table[m_header[I]].push_back(elem);
				}
				else {
					m_table[m_header[I]].push_back(StringType(elem));
				}
			}
		}

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
			InsertSingle<I>(arg1);
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
	/// Calculate the number of rows that this item is going to require
	/// </summary>
	/// <param name="item_number">Element index in column vector.</param>
	size_t GetRowCount(size_t item_number)
	{
		size_t row_count = 0;
		// For all headers (columns) count the number of rows
		for (auto& header : m_header)
		{
			//For all items on that column: single elements count as 1, PrettyTables add their rows
			size_t rows = 0;
			if (item_number >= m_table[header].size())
			{
				// We are past the end of this vector so we will just print spaces
				rows = 1;
			}
			else
			{
				rows = std::visit([](auto&& arg)->size_t
				{
					using T = std::decay_t<decltype(arg)>;
					if constexpr (std::is_same_v<T, StringType>)
					{
						return 1ll;
					}
					else if constexpr (std::is_same_v<T, PrettyTable<CharType>>)
					{
						return arg.GetRowCount();
					}
					else {
						static_assert(std::_Always_false<T>, "Invalid type");
					}
				}, m_table[header][item_number]);
			}

			if (row_count < rows)
			{
				row_count = rows;
			}
		}
		return row_count;
	}

	/// <summary>
	/// Calculate the number of rows that the whole table is going to take
	/// </summary>
	size_t GetRowCount()
	{
		size_t row_count = 1ll + (m_header_underline ? 1ll : 0ll); // Header + underline
		size_t num_items = m_table[m_header[0]].size(); // It should be the same for all columns [TODO] Assert this somewhere?
		for (auto i = 0; i < num_items; i++)
		{
			row_count += GetRowCount(i);
		}
		return row_count;
	}

	/// <summary>
	/// Prints a particular row given the number of rows so far and the item it should belong to.
	/// </summary>
	/// <param name="row_number">Row number that is going to be printed.</param>
	/// <param name="item_number">Element index in column vector.</param>
	/// <param name="acc_row_number">The accumulated number of rows so far</param>
	/// <param name="out_stream">Reference to the stream that is going to be used to print this row</param>
	/// <param name="isEmbedded">Tell if the current row belongs to an embedded PrettyTable</param>
	void PrintRow(size_t row_number, size_t item_number, size_t acc_row_number, std::wstringstream& out_stream, bool isEmbedded = false)
	{
		size_t j = 0;	// Rows in column index
		size_t item_relative_row_number = row_number - acc_row_number;

		if (!isEmbedded) PrintLeftMargin(out_stream);

		// Print header requested
		if (row_number == 0)
		{
			// Print colum names (headers)
			for (const auto& header : m_header) {
				// Calculate maximum width of each column
				size_t max = GetColumnMaxWidth(m_table[header]);
				m_columns_max_width.push_back(std::max(max, header.size()));

				// Calculate length for each column
				m_columns_length.push_back(m_table[header].size());

				PrintColumnSeparator(out_stream);
				out_stream << std::setw(m_columns_max_width[j]);
				PrintColumnAlign(m_columns_align[j], out_stream);
				out_stream << header;
				j++;
			}
			return;
		} // Print underline requested
		else if (row_number == 1 && m_header_underline)
		{
			// Print (if enabled) column name underline
			if (m_header_underline) {
				j = 0;	// Rows in column index
				for (const auto& header : m_header) {
					PrintColumnSeparator(out_stream);
					out_stream << std::setw(m_columns_max_width[j]);
					PrintColumnAlign(m_columns_align[j], out_stream);
					out_stream << StringType(header.length(), LiteralConstants<CharType>::m_equal[0]);
					j++;
				}
			}
			return;
		}

		for (const auto& header : m_header)
		{
			PrintColumnSeparator(out_stream);

			PrintColumnAlign(m_columns_align[j], out_stream);

			std::visit([&](auto&& arg)
			{
				using T = std::decay_t<decltype(arg)>;
				if constexpr (std::is_same_v<T, StringType>)
				{
					out_stream << std::setw(m_columns_max_width[j]);
					if (item_relative_row_number > 0)
					{
						out_stream << StringType(m_columns_max_width[j], LiteralConstants<CharType>::m_space[0]);
					}
					else {
						out_stream << arg;
					}
				}
				else if constexpr (std::is_same_v<T, PrettyTable<CharType>>)
				{
					long long off = item_relative_row_number - 1 - (m_header_underline ? 1 : 0);
					size_t curr_item_number = off < 0 ? 0 : off;
					size_t row_size = arg.GetRowCount();
					if (item_relative_row_number >= row_size)
					{
						out_stream << std::setw(m_columns_max_width[j]);
						out_stream << StringType(m_columns_max_width[j], LiteralConstants<CharType>::m_space[0]);
					}
					else
					{
						// Properly map row_number/acc_row_number to this PrettyTable's row
						size_t item_row_size = 0;
						if (curr_item_number >= 0 && off == 0)
						{
							// Count headers and underline
							item_row_size += 1ll + (m_header_underline ? 1ll : 0ll);
							if (curr_item_number > 0)
							{
								item_row_size = m_rows_acc_count[curr_item_number];
							}
						}
						else {
							item_row_size = item_relative_row_number;
						}

						arg.PrintRow(item_relative_row_number, curr_item_number, item_row_size, out_stream, true);
					}
				}
				else {
					static_assert(std::_Always_false<T>, "Invalid type");
				}
			}, m_table[header][item_number]);

			j++;
		}
	}

	/// <summary>
	/// Print pretty table to m_out_stream
	/// </summary>
	void Print()
	{
		//Pre-calculate rows
		size_t acc = 0;
		for (size_t i = 0; i < m_table[m_header[0]].size(); i++)
		{
			size_t count = GetRowCount(i);
			m_rows_count.push_back(count);
			m_rows_acc_count.push_back(acc);
			acc += count;
		}

		size_t row_num = 0;
		PrintRow(row_num, 0, row_num, m_out_stream);
		m_out_stream << std::flush << std::endl;
		row_num++;
		if (m_header_underline)
		{
			PrintRow(row_num, 0, row_num, m_out_stream);
			m_out_stream << std::flush << std::endl;
			row_num++;
		}
		size_t num_items = m_table[m_header[0]].size();
		for (auto i = 0; i < num_items; i++)
		{
			size_t row_count = m_rows_count[i];
			for (auto j = 0; j < row_count; j++)
			{
				PrintRow(j + row_num, i, row_num, m_out_stream);
				m_out_stream << std::flush << std::endl;
			}
			row_num += row_count;
		}
	}

	void PrintManRow(size_t row_number, size_t item_number, size_t acc_row_number, std::wstringstream& out_stream)
	{
		size_t j = 0;	// Rows in column index
		size_t item_relative_row_number = row_number - acc_row_number;

		for (const auto& header : m_header)
		{
			if (j != 0) 
			{
				out_stream << std::endl << L"    ";
			}

			std::visit([&](auto&& arg)
				{
					using T = std::decay_t<decltype(arg)>;
					if constexpr (std::is_same_v<T, StringType>)
					{
						if (item_relative_row_number == 0)
						{
							out_stream << arg;
						}
					}
					else if constexpr (std::is_same_v<T, PrettyTable<CharType>>)
					{
						long long off = item_relative_row_number - 1 - (m_header_underline ? 1 : 0);
						size_t curr_item_number = off < 0 ? 0 : off;
						size_t row_size = arg.GetRowCount();
						if (item_relative_row_number >= row_size)
						{
							out_stream << std::setw(m_columns_max_width[j]);
							out_stream << StringType(m_columns_max_width[j], LiteralConstants<CharType>::m_space[0]);
						}
						else
						{
							// Properly map row_number/acc_row_number to this PrettyTable's row
							size_t item_row_size = 0;
							if (curr_item_number >= 0 && off == 0)
							{
								// Count headers and underline
								item_row_size += 1ll + (m_header_underline ? 1ll : 0ll);
								if (curr_item_number > 0)
								{
									item_row_size = m_rows_acc_count[curr_item_number];
								}
							}
							else {
								item_row_size = item_relative_row_number;
							}

							arg.PrintRow(item_relative_row_number, curr_item_number, item_row_size, out_stream, true);
						}
					}
					else 
					{
						static_assert(std::_Always_false<T>, "Invalid type");
					}
				}, m_table[header][item_number]);

			j++;
		}
	}

	void PrintMan()
	{
		//Pre-calculate rows
		size_t acc = 0;
		for (size_t i = 0; i < m_table[m_header[0]].size(); i++)
		{
			size_t count = GetRowCount(i);
			m_rows_count.push_back(count);
			m_rows_acc_count.push_back(acc);
			acc += count;
		}

		size_t row_num = 0;

		m_out_stream << std::flush << std::endl;
		row_num++;

		size_t num_items = m_table[m_header[0]].size();
		for (auto i = 0; i < num_items; i++)
		{
			size_t row_count = m_rows_count[i];
			for (auto j = 0; j < row_count; j++)
			{
				PrintManRow(j + row_num, i, row_num, m_out_stream);
				m_out_stream << std::flush << std::endl;
			}
			row_num += row_count;
		}
	}

private:
	size_t length()
	{
		size_t max = 0;
		size_t num_cols = 0;
		for (auto& [key, val] : m_table)
		{
			auto colWidth = std::max(GetColumnMaxWidth(val), m_header[num_cols].size());
			if (colWidth > max)
				max = colWidth;
			num_cols++;
		}
		return max * num_cols + m_COLUMN_SEPARATOR * (num_cols);
	}

	/// <summary>
	/// Left margin for whole pretty table from beggining of the screen
	/// </summary>
	/// <param name="out_stream">Reference to the stream that is going to be used</param>
	void PrintLeftMargin(std::wstringstream& out_stream) {
		out_stream << StringType(m_LEFT_MARGIN, LiteralConstants<CharType>::m_space[0]);
	}

	/// <summary>
	/// Print white space column separation.
	/// </summary>
	/// <param name="out_stream">Reference to the stream that is going to be used</param>
	void PrintColumnSeparator(std::wstringstream& out_stream) {
		out_stream << StringType(m_COLUMN_SEPARATOR, LiteralConstants<CharType>::m_space[0]);
	}

	/// <summary>
	/// Control how std::wcout aligns text.
	/// </summary>
	/// <param name="Align">LEFT or RIGHT</param>
	/// <param name="out_stream">Reference to the stream that is going to be used</param>
	void PrintColumnAlign(ColumnAlign Align, std::wstringstream& out_stream) {
		switch (Align) {
		case LEFT:  out_stream << std::left; break;
		case RIGHT: out_stream << std::right; break;
		}
	}

	/// <summary>
	/// Get maximal width of column. Calculate for every row in
	/// column its width and get max of it.
	/// </summary>
	/// <param name="Column">VECTOR of WSTRINGS with rows</param>
	/// <returns>Length of longest column cell in Column</returns>
	size_t GetColumnMaxWidth(std::vector<std::variant<StringType, PrettyTable<CharType>>>& Column)
	{
		if (Column.empty()) return 0;

		using VectorElementT = decltype(Column[0]);
		auto max_len = std::max_element(Column.begin(), Column.end(), [](VectorElementT a, VectorElementT b) {
			return std::visit([=](auto&& arg1, auto&& arg2) -> bool {
				return arg1.length() < arg2.length();
			}, a, b);
		});

		return std::visit([=](auto&& arg) -> size_t { return arg.length(); }, *max_len);
	}

	// Consts
	const size_t m_LEFT_MARGIN = 6;
	const size_t m_COLUMN_SEPARATOR = 2;

	// Class members
	StringStream m_out_stream;
	std::map<StringType,
		std::vector<std::variant<StringType, PrettyTable<CharType>>>> m_table;	// Table where key is column name and value is VECTOR of WSTRINGs
	std::vector<StringType> m_header;				// VECTOR of column names (header) WSTRINGs
	bool m_header_underline;						// Enable header underlining, default true
	std::vector<size_t> m_columns_max_width;		// Max width of each column, index with int(rown number)
	std::vector<size_t> m_columns_length;			// Length (how many rows) for each rows, index with int(rown number)
	std::vector<size_t> m_rows_count;				// Number of rows for all items
	std::vector<size_t> m_rows_acc_count;			// Number of accumulated rows for all items
	std::vector<ColumnAlign> m_columns_align;		// Column align (left, right), index with int(rown number)
};
