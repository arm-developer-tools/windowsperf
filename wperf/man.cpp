#include "man.h"
#include "output.h"
#include "exception.h"

#include <string>
#include <map>
#include <vector>
#include <iostream>

static std::vector<std::wstring> _col1;
static std::vector<std::wstring> _col2;

struct Item 
{
	enum Type {Event,Metric,Group_Metric} type;
	std::wstring product;
	std::wstring name;
};

static std::vector<Item> requested_items;


bool inline static is_valid_cpu(const pmu_device& pdev, std::wstring name)
{
	return pdev.m_product_configuration.count(name) > 0;
}

void print_metric(pmu_device& pdev, const std::wstring& product_name, const std::wstring& requested_metric) 
{
	product_metric metric = pdev.m_product_metrics[product_name][requested_metric];

	_col1.push_back(L"NAME");
	_col2.push_back(metric.name + L" - " + metric.title);
	_col1.push_back(L"FORMULA");
	_col2.push_back(metric.metric_formula);
}


void print_event(pmu_device& pdev, const std::wstring& product_name, const std::wstring& requested_event) 
{
	product_event event = pdev.m_product_events[product_name][requested_event];

	_col1.push_back(L"NAME");
	_col2.push_back(event.name + L" - " + event.title);
}

void print_groups_metric(pmu_device& pdev, const std::wstring& product_name, const std::wstring& requested_arg) 
{
	product_group_metrics group_metrics = pdev.m_product_groups_metrics[product_name][requested_arg];

	_col1.push_back(L"NAME");
	_col2.push_back(group_metrics.name + L" - " + group_metrics.title);
	_col1.push_back(L"METRICS");
	_col2.push_back(group_metrics.metrics_raw);
}

bool find_in_product_data(pmu_device& pdev, const std::wstring& product_name, const std::wstring& requested_arg)
{	
	if (pdev.m_product_metrics[product_name].count(requested_arg) > 0)
	{
		requested_items.push_back({ Item::Metric, product_name, requested_arg });
		return true;
	}

	else if (pdev.m_product_events[product_name].count(requested_arg) > 0) 
	{
		requested_items.push_back({ Item::Event, product_name, requested_arg });
		return true;
	}

	else if (pdev.m_product_groups_metrics[product_name].count(requested_arg) > 0) 
	{
		requested_items.push_back({ Item::Group_Metric, product_name, requested_arg });
		return true;
	}

	return false;
}


void man(pmu_device& pdev, std::vector<std::wstring>& args, const wchar_t& delim, const std::wstring& product_name, std::vector<std::wstring>& col1, std::vector<std::wstring>& col2) 
{
	_col1.clear();
	_col2.clear();

	std::wstring detected_cpu = product_name;
	std::wstring cur_cpu;
	std::wstring requested_arg;


	for (auto &arg : args)
	{
		if (arg.find(delim) != std::wstring::npos) 
		{
			cur_cpu = arg.substr(0, arg.find(L'/'));
			requested_arg = arg.substr(arg.find(L'/') + 1, arg.back());
		}
		else 
		{
			cur_cpu = detected_cpu;
			requested_arg = arg;
		}

		if (!is_valid_cpu(pdev, cur_cpu)) 
		{
			m_out.GetErrorOutputStream() << "CPU name: \"" << cur_cpu << "\" not found!" << std::endl;
			throw fatal_exception("ERROR_CPU_NAME");
		}

		//find event,group or event_group for cur_cpu and print them
		if (!find_in_product_data(pdev, cur_cpu, requested_arg))
		{
			m_out.GetErrorOutputStream() << "\"" << requested_arg << "\" not found! Ensure it is compatible with the specified CPU" << std::endl;
			throw fatal_exception("ERROR_REQUESTED_ARG");
		}
	}

	for (const auto& item : requested_items) 
	{
		switch (item.type) 
		{
			case Item::Event:
				print_event(pdev, item.product, item.name);
				break;
			case Item::Metric:
				print_metric(pdev, item.product, item.name);
				break;
			case Item::Group_Metric:
				print_groups_metric(pdev, item.product, item.name);
				break;
		}
	}

	col1 = _col1;
	col2 = _col2;

}
