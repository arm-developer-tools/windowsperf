#pragma once
#include "pmu_device.h"
#include <string>
#include <map>
#include <vector>
#include <iostream>

void print_metric(const pmu_device& pdev, const std::wstring& product_name, const std::wstring& requested_metric);
void print_event(const pmu_device& pdev, const std::wstring& product_name, const std::wstring& requested_event);
void print_groups_metric(const pmu_device& pdev, const std::wstring& product_name, const std::wstring& requested_arg);
bool find_in_product_data(pmu_device& pdev, std::wstring product_name, const std::wstring& requested_arg);
void man(pmu_device& pdev, std::vector<std::wstring>& token_args, const wchar_t& delim, const std::wstring& product_name, std::vector<std::wstring>& col1, std::vector<std::wstring>& col2);
