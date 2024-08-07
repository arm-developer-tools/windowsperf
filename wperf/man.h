#pragma once
#include "pmu_device.h"
#include <string>
#include <map>
#include <vector>
#include <iostream>

void man(const pmu_device& pdev, std::vector<std::wstring>& token_args, const wchar_t& delim, const std::wstring& product_name, std::vector<std::wstring>& col1, std::vector<std::wstring>& col2);
