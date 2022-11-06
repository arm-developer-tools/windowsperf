#include "prettytable.h"

/// <summary>
/// Add column to the table. Columns will be added from left to right.
/// </summary>
/// <param name="ColumnName">Name of the column</param>
/// <param name="ColumnValues">VECTOR of WSTRING with column text values</param>
/// <param name="Align">Align whole column to LEFT or RIGHT</param>
void PrettyTable::AddColumn(std::wstring ColumnName, std::vector<std::wstring> ColumnValues, enum ColumnAlign Align) {
	m_header.push_back(ColumnName);
	m_columns_align.push_back(Align);
	m_table[ColumnName] = ColumnValues;
};

/// <summary>
/// Print pretty table on screen with std::wcout
/// </summary>
void PrettyTable::Print() {
	PrintLeftMargin();

	// Print colum names (headers)
	size_t j = 0;	// Rows in column index
	for (auto header : m_header) {
		// Calculate maximum width of each column
		size_t max = GetColumnMaxWidth(m_table[header]);
		m_columns_max_width.push_back(std::max(max, header.size()));

		// Calculate length for each column
		m_columns_length.push_back(m_table[header].size());

		PrintColumnSeparator();
		std::wcout << std::setw(max);
		PrintColumnAlign(m_columns_align[j]);
		std::wcout << header;
		j++;
	}
	std::wcout << std::endl;

	// Print (if enabled) column name underline
	if (m_header_underline) {
		PrintLeftMargin();

		j = 0;	// Rows in column index
		for (auto header : m_header) {
			PrintColumnSeparator();
			std::wcout << std::setw(m_columns_max_width[j]);
			PrintColumnAlign(m_columns_align[j]);
			std::wcout << std::wstring(header.length(), L'=');
			j++;
		}
		std::wcout << std::endl;
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
			std::wcout << std::setw(m_columns_max_width[j]);
			PrintColumnAlign(m_columns_align[j]);
			std::wcout << column[i];
			j++;
		}
		i++;
		std::wcout << std::endl;
	}
}

/// <summary>
/// Get maximal width of column. Calculate for every row in
/// column its width and get max of it.
/// </summary>
/// <param name="Column">VECTOR of WSTRINGS with rows</param>
/// <returns>Length of longest column cell in Column</returns>
size_t PrettyTable::GetColumnMaxWidth(std::vector<std::wstring> Column) {
	auto max_len = std::max_element(Column.begin(), Column.end(), [](std::wstring a, std::wstring b) {
		return a.length() < b.length();
		});
	return (*max_len).length();
}
