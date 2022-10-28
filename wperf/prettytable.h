#pragma once

#include <algorithm>
#include <iomanip>
#include <iostream>
#include <map>
#include <string>
#include <vector>

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
/// 
/// </summary>
class PrettyTable
{
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

	/// <summary>
	/// Define if you want to underline column headers with '='
	/// </summary>
	/// <param name="value">TRUE if underline (default), FALSE to disable underline</param>
	void SetHeaderUnderline(bool value) {
		m_header_underline = value;
	}

	void AddColumn(std::wstring ColumnName, std::vector<std::wstring> ColumnValues, enum ColumnAlign Align = LEFT);
	void Print();

private:
	/// <summary>
	/// Left margin for whole pretty table from beggining of the screen
	/// </summary>
	void PrintLeftMargin() {
		std::wcout << std::wstring(m_LEFT_MARGIN, L' ');
	}

	/// <summary>
	/// Print white space column separation.
	/// </summary>
	void PrintColumnSeparator() {
		std::wcout << std::wstring(m_COLUMN_SEPARATOR, L' ');
	}

	/// <summary>
	/// Control how std::wcout aligns text.
	/// </summary>
	/// <param name="Align">LEFT or RIGHT</param>
	void PrintColumnAlign(ColumnAlign Align) {
		switch (Align) {
		case LEFT:  std::wcout << std::left; break;
		case RIGHT: std::wcout << std::right; break;
		}
	}

	/// <summary>
	/// Get maximal width of column. Calculate for every row in
	/// column its width and get max of it.
	/// </summary>
	/// <param name="Column">VECTOR of WSTRINGS with rows</param>
	/// <returns></returns>
	size_t GetColumnMaxWidth(std::vector<std::wstring> Column);

	// Consts
	const size_t m_LEFT_MARGIN = 6;
	const size_t m_COLUMN_SEPARATOR = 2;

	// Class members
	std::map<std::wstring,
		     std::vector<std::wstring>> m_table;	// Table where key is column name and value is VECTOR of WSTRINGs
	std::vector<std::wstring> m_header;				// VECTOR of column names (header) WSTRINGs
	bool m_header_underline;						// Enable header underlining, default true
	std::vector<size_t> m_columns_max_width;		// Max width of each column, index with int(rown number)
	std::vector<size_t> m_columns_length;			// Length (how many rows) for each rows, index with int(rown number)
	std::vector<ColumnAlign> m_columns_align;		// Column align (left, right), index with int(rown number)
};

