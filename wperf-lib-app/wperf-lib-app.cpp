#include <stdio.h>
#include "wperf-lib.h"

int __cdecl
wmain(
    int argc,
    wchar_t* argv[]
)
{
    if (!wperf_init())
        return 1;

	VERSION_INFO v;
	if (wperf_driver_version(&v))
		printf("wperf_driver_version: %u.%u.%u\n", v.major, v.minor, v.patch);

	if (wperf_version(&v))
		printf("wperf_version: %u.%u.%u\n", v.major, v.minor, v.patch);

	LIST_CONF list_conf = { CORE_EVT /* Only list Core PMU events */ };
	EVENT_INFO einfo;

	if (wperf_list_events(&list_conf, NULL))
	{
		int num_events;
		if (wperf_list_num_events(&list_conf, &num_events))
		{
			printf("wperf_list_num_events: %d\n", num_events);
		}

		while (wperf_list_events(&list_conf, &einfo))
		{
			printf("wperf_list_events: type=%d, id=%u, name=%ls\n", einfo.type, einfo.id, einfo.name);
		}
	}

	METRIC_INFO minfo;

	if (wperf_list_metrics(&list_conf, NULL))
	{
		int num_metrics;
		if (wperf_list_num_metrics(&list_conf, &num_metrics))
		{
			printf("wperf_list_num_metrics: %d\n", num_metrics);
		}

		int num_metrics_events;
		if (wperf_list_num_metrics_events(&list_conf, &num_metrics_events))
		{
			printf("wperf_list_num_metrics_events: %d\n", num_metrics_events);
		}

		while (wperf_list_metrics(&list_conf, &minfo))
		{
			printf("wperf_list_metrics: metric_name=%ls, event_idx=%u\n", minfo.metric_name, minfo.event_idx);
		}
	}

	uint8_t cores[2] = { 0, 3 };
	uint16_t events[2] = { 0x1B, 0x73 };
	int num_group_events[1] = { 2 };
	uint16_t group_events[2] = { 0x70, 0x71 };
	uint16_t* gevents[1] = { group_events };
	const wchar_t* metric_events[1] = { L"dcache" };
	STAT_CONF stat_conf =
	{
		2, // num_cores
		cores, // cores
		2, // num_events
		events, // events
		{1/*num_groups*/, num_group_events/*num_group_events*/, gevents/*events*/}, // group_events
		1, // num_metrics
		metric_events, // metric_events
		1, // duration
		false // kernel_mode
	};
	if (wperf_stat(&stat_conf, NULL))
	{
		STAT_INFO stat_info;
		while (wperf_stat(&stat_conf, &stat_info))
		{
			printf("wperf_stat: core_idx=%u, event_idx=%u, counter_value=%llu, evt_note=", stat_info.core_idx, stat_info.event_idx, stat_info.counter_value);
			switch (stat_info.evt_note.type)
			{
			case NORMAL_EVT_NOTE:
				printf("NORMAL_EVT_NOTE\n");
				break;
			case GROUP_EVT_NOTE:
				printf("GROUP_EVT_NOTE, group_id=%u\n", stat_info.evt_note.note.group_note.group_id);
				break;
			case METRIC_EVT_NOTE:
				printf("METRIC_EVT_NOTE, group_id=%u, metric_name=%ls\n", stat_info.evt_note.note.metric_note.group_id, stat_info.evt_note.note.metric_note.name);
				break;
			default:
				printf("Unrecognized event note type\n");
			}
		}
	}

	int num_cores;
	if (wperf_num_cores(&num_cores))
	{
		printf("wperf_num_cores: %d\n", num_cores);
	}

    if (!wperf_close())
        return 1;

    return 0;
}
