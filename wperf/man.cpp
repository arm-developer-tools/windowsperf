#include "man.h"
#include "output.h"
#include "exception.h"
#include "utils.h"

#include <algorithm>
#include <string>
#include <map>
#include <vector>
#include <iostream>

bool inline static man_is_valid_cpu(const pmu_device& pdev, std::wstring name);
std::vector<std::wstring> static man_get_valid_cpu_list(const pmu_device& pdev);
void static man_print_metric(const pmu_device& pdev, const std::wstring& product_name, const std::wstring& requested_metric);
void static man_print_event(const pmu_device& pdev, const std::wstring& product_name, const std::wstring& requested_event);
void static man_print_groups_metric(const pmu_device& pdev, const std::wstring& product_name, const std::wstring& requested_arg);
bool static man_find_in_product_data(const pmu_device& pdev, std::wstring product_name, const std::wstring& requested_arg);

static std::vector<std::wstring> man_col1;
static std::vector<std::wstring> man_col2;

struct man_Item 
{
	enum Type {Event,Metric,Group_Metric} type;
	std::wstring product;
	std::wstring name;
};

static std::vector<man_Item> requested_items;

bool inline static man_is_valid_cpu(const pmu_device& pdev, std::wstring name)
{
	std::vector<std::wstring> arm_arch = { pdev.m_PRODUCT_ARMV8A, pdev.m_PRODUCT_ARMV9A };
	return pdev.m_product_configuration.count(name) > 0 ||
		std::count(arm_arch.begin(), arm_arch.end(), name);
}

std::vector<std::wstring> static man_get_valid_cpu_list(const pmu_device& pdev)
{
	std::vector<std::wstring> result = { pdev.m_PRODUCT_ARMV8A, pdev.m_PRODUCT_ARMV9A};
	for (auto const& product : pdev.m_product_configuration)
		result.push_back(product.first);
	return result;
}

void static man_print_metric(const pmu_device& pdev, const std::wstring& product_name, const std::wstring& requested_metric)
{
	product_metric metric = pdev.m_product_metrics.at(product_name).at(requested_metric);

	man_col1.push_back(L"CPU");
	man_col2.push_back(product_name);

	man_col1.push_back(L"NAME");
	man_col2.push_back(metric.name + L" - " + metric.title);

	std::wstring formatted_events = metric.events_raw;
	ReplaceAllTokensInWString(formatted_events, L",", L", ");

	man_col1.push_back(L"EVENTS");
	man_col2.push_back(formatted_events);

	man_col1.push_back(L"DESCRIPTION");
	man_col2.push_back(metric.description);

	man_col1.push_back(L"FORMULA");
	man_col2.push_back(metric.metric_formula);
	man_col1.push_back(L"UNIT");
	man_col2.push_back(metric.metric_unit);
}


void static man_print_event(const pmu_device& pdev, const std::wstring& product_name, const std::wstring& requested_event)
{
	product_event event = pdev.m_product_events.at(product_name).at(requested_event);
	
	man_col1.push_back(L"CPU");
	man_col2.push_back(product_name);

	man_col1.push_back(L"NAME");
	man_col2.push_back(event.name + L" - " + event.title);

	man_col1.push_back(L"DESCRIPTION");
	man_col2.push_back(event.description);
}

void static man_print_groups_metric(const pmu_device& pdev, const std::wstring& product_name, const std::wstring& requested_arg)
{
	product_group_metrics group_metrics = pdev.m_product_groups_metrics.at(product_name).at(requested_arg);

	man_col1.push_back(L"CPU");
	man_col2.push_back(product_name);

	man_col1.push_back(L"NAME");
	man_col2.push_back(group_metrics.name + L" - " + group_metrics.title);

	man_col1.push_back(L"DESCRIPTION");
	man_col2.push_back(group_metrics.description);

	std::wstring formatted_metrics = group_metrics.metrics_raw;
	ReplaceAllTokensInWString(formatted_metrics, L",", L", ");

	man_col1.push_back(L"METRICS");
	man_col2.push_back(formatted_metrics);
}

bool static man_find_in_product_data(const pmu_device& pdev, std::wstring product_name, const std::wstring& requested_arg)
{
	try 
	{
		if (pdev.is_alias_product_name(product_name)) 
		{
			product_name = pdev.get_product_name(product_name);
		}

		const auto& metrics_map = pdev.m_product_metrics.at(product_name);
		const auto& events_map = pdev.m_product_events.at(product_name);
		const auto& group_metrics_map = pdev.m_product_groups_metrics.at(product_name);

		if (metrics_map.find(requested_arg) != metrics_map.end()) 
		{
			requested_items.push_back({ man_Item::Metric, product_name, requested_arg });
			return true;
		}
		else if (events_map.find(requested_arg) != events_map.end()) 
		{
			requested_items.push_back({ man_Item::Event, product_name, requested_arg });
			return true;
		}
		else if (group_metrics_map.find(requested_arg) != group_metrics_map.end()) 
		{
			requested_items.push_back({ man_Item::Group_Metric, product_name, requested_arg });
			return true;
		}
		else
		{
			return false;
		}
	}
	catch (const std::out_of_range&) {
		return false;
	}
}


void man(const pmu_device& pdev, std::vector<std::wstring>& args, const wchar_t& delim, const std::wstring& product_name, std::vector<std::wstring>& col1, std::vector<std::wstring>& col2) 
{
	man_col1.clear();
	man_col2.clear();

	std::wstring detected_cpu = product_name;
	std::wstring cur_cpu;
	std::wstring requested_arg;

	if (args.empty())
	{
		m_out.GetErrorOutputStream() << L"warning: no argument(s) specified, specify an event, metric, or group of metrics to obtain relevant, manual style, information." << std::endl;
		throw fatal_exception("ERROR_REQUESTED_ARG");
	}

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

		if (!man_is_valid_cpu(pdev, cur_cpu)) 
		{
			std::wstring avail_cpus = WStringJoin(man_get_valid_cpu_list(pdev), L", ");

			m_out.GetErrorOutputStream() << L"warning: CPU name: \"" << cur_cpu << "\" not found, use " << avail_cpus << L"." << std::endl;
			throw fatal_exception("ERROR_CPU_NAME");
		}

		//find event,group or event_group for cur_cpu and print them
		if (!man_find_in_product_data(pdev, cur_cpu, requested_arg))
		{
			m_out.GetErrorOutputStream() << L"warning: \"" << requested_arg << L"\" not found! Ensure it is compatible with the specified CPU" << std::endl;
			throw fatal_exception("ERROR_REQUESTED_ARG");
		}
	}

	for (const auto& item : requested_items) 
	{
		switch (item.type) 
		{
			case man_Item::Event:
				man_print_event(pdev, item.product, item.name);
				break;
			case man_Item::Metric:
				man_print_metric(pdev, item.product, item.name);
				break;
			case man_Item::Group_Metric:
				man_print_groups_metric(pdev, item.product, item.name);
				break;
		}
	}

	col1 = man_col1;
	col2 = man_col2;
}
