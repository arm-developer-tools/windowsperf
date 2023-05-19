#include "wperf-lib.h"
#include "padding.h"
#include "pmu_device.h"
#include "wperf-common/public.h"
#include <regex>

typedef struct _COUNTING_INFO
{
    uint64_t counter_value;
    uint16_t event_idx;
    uint64_t multiplexed_scheduled;
    uint64_t multiplexed_round;
    uint64_t scaled_value;
    EVENT_NOTE evt_note;
} COUNTING_INFO;

static pmu_device* __pmu_device = nullptr;
static struct pmu_device_cfg* __pmu_cfg = nullptr;
static std::map<enum evt_class, std::vector<uint16_t>> __list_events;
static size_t __list_index = 0;
static std::map<enum evt_class, std::vector<struct evt_noted>> __ioctl_events;
static std::map<uint8_t, std::vector<COUNTING_INFO>> __countings;

extern "C" bool wperf_init()
{
    try
    {
        __pmu_device = new pmu_device();
        __pmu_device->init();
        __pmu_cfg = new pmu_device_cfg();
        __pmu_device->get_pmu_device_cfg(*__pmu_cfg);
    }
    catch (...)
    {
        return false;
    }

    return true;
}

extern "C" bool wperf_close()
{
    try
    {
        if (__pmu_device)
            delete __pmu_device;

        if (__pmu_cfg)
            delete __pmu_cfg;

        __list_events.clear();
        __ioctl_events.clear();
        __countings.clear();
    }
    catch(...)
    {
        return false;
    }

    return true;
}

extern "C" bool wperf_driver_version(PVERSION_INFO driver_ver)
{
    try
    {
        struct version_info version;

        __pmu_device->do_version_query(version);
        driver_ver->major = version.major;
        driver_ver->minor = version.minor;
        driver_ver->patch = version.patch;
    }
    catch (...)
    {
        return false;
    }

    return true;
}

extern "C" bool wperf_version(PVERSION_INFO wperf_ver)
{
    wperf_ver->major = MAJOR;
    wperf_ver->minor = MINOR;
    wperf_ver->patch = PATCH;

    return true;
}

extern "C" bool wperf_list_events(PLIST_CONF list_conf, PEVENT_INFO einfo)
{
    try
    {
        if (!einfo)
        {
            // Call events_query to initialize __list_events
            // with the list of all supported events.
            __list_events.clear();
            __pmu_device->events_query(__list_events);
            __list_index = 0;
        }
        else if (list_conf->list_event_types&CORE_EVT)
        {
            // Yield the next event.
            if (__list_index < __list_events[EVT_CORE].size())
            {
                einfo->type = CORE_TYPE;
                einfo->id = __list_events[EVT_CORE][__list_index];
                einfo->name = pmu_events::get_core_event_name(einfo->id);
                __list_index++;
            }
            else
            {
                // No more to yield.
                return false;
            }
        }
        else
        {
            // Only Core PMU events are supported for now.
            return false;
        }
    }
    catch (...)
    {
        return false;
    }

    return true;
}

extern "C" bool wperf_stat(PSTAT_CONF stat_conf, PSTAT_INFO stat_info)
{
    static size_t event_index = 0;
    static size_t core_index = 0;

    try
    {
        if (!stat_info)
        {
            __ioctl_events.clear();
            __countings.clear();
            event_index = 0;
            core_index = 0;

            std::vector<uint8_t> cores_idx;
            for (int i = 0; i < stat_conf->num_cores; i++)
            {
                cores_idx.push_back(stat_conf->cores[i]);
            }
            // Only CORE events are supported at the moment.
            std::vector<enum evt_class> e_classes = { EVT_CORE };
            uint32_t enable_bits = __pmu_device->enable_bits(e_classes);
            __pmu_device->post_init(cores_idx, 0, false, enable_bits);

            uint32_t stop_bits = __pmu_device->stop_bits();
            __pmu_device->stop(stop_bits);

            // Process events from stat_conf.
            std::map<enum evt_class, std::deque<struct evt_noted>> events;
            for (int i = 0; i < stat_conf->num_events; i++)
            {
                events[EVT_CORE].push_back({ stat_conf->events[i], EVT_NORMAL, L"e" });
            }

            // Process groups from stat_conf.
            std::map<enum evt_class, std::vector<struct evt_noted>> groups;
            for (int i = 0; i < stat_conf->group_events.num_groups; i++)
            {
                groups[EVT_CORE].push_back({ (uint16_t)stat_conf->group_events.num_group_events[i], EVT_HDR, L"" });
                for (int j = 0; j < stat_conf->group_events.num_group_events[i]; j++)
                {
                    groups[EVT_CORE].push_back({ stat_conf->group_events.events[i][j], EVT_GROUPED, L"" });
                }
            }

            // Process metrics from stat_conf.
            std::map<std::wstring, metric_desc>& metrics = __pmu_device->builtin_metrics;
            for (int i = 0; i < stat_conf->num_metrics; i++)
            {
                const wchar_t* metric = stat_conf->metric_events[i];
                if (metrics.find(metric) == metrics.end())
                {
                    // Not a valid builtin metric.
                    return false;
                }

                metric_desc desc = metrics[metric];
                for (const auto& x : desc.events)
                    events[x.first].insert(events[x.first].end(), x.second.begin(), x.second.end());
                for (const auto& y : desc.groups)
                    groups[y.first].insert(groups[y.first].end(), y.second.begin(), y.second.end());
            }

            set_event_padding(__ioctl_events, *__pmu_cfg, events, groups);

            bool do_kernel = stat_conf->kernel_mode;
            for (uint32_t core_idx : cores_idx)
                __pmu_device->events_assign(core_idx, __ioctl_events, do_kernel);

            double count_duration = stat_conf->duration;
            int64_t counting_duration_iter = count_duration > 0 ?
                static_cast<int64_t>(count_duration * 10) : _I64_MAX;

            __pmu_device->reset(enable_bits);

            __pmu_device->start(enable_bits);

            int64_t t_count = counting_duration_iter;

            while (t_count > 0)
            {
                t_count--;
                Sleep(100);
            }

            __pmu_device->stop(enable_bits);

            if (enable_bits & CTL_FLAG_CORE)
            {
                __pmu_device->core_events_read();
                ReadOut* core_outs = __pmu_device->get_core_outs();

                std::vector<uint8_t> counting_cores = __pmu_device->get_cores_idx();
                for (auto i : counting_cores)
                {
                    UINT32 evt_num = core_outs[i].evt_num;
                    struct pmu_event_usr* evts = core_outs[i].evts;
                    uint64_t round = core_outs[i].round;
                    for (UINT32 j = 0; j < evt_num; j++)
                    {
                        // Ignore padding events.
                        if (j >= 1 && (__ioctl_events[EVT_CORE][j - 1].type == EVT_PADDING))
                            continue;

                        struct pmu_event_usr* evt = &evts[j];
                        COUNTING_INFO counting_info;
                        counting_info.counter_value = evt->value;
                        counting_info.event_idx = evt->event_idx;
                        if (evt->event_idx == CYCLE_EVT_IDX)
                        {
                            counting_info.evt_note.type = NORMAL_EVT_NOTE;
                        }
                        else
                        {
                            std::wstring note = __ioctl_events[EVT_CORE][j - 1].note;
                            std::wsmatch m;
                            if (note == L"e")
                            {
                                // This is a NORMAL_EVT_NOTE.
                                counting_info.evt_note.type = NORMAL_EVT_NOTE;
                            }
                            else if (std::regex_match(note, m, std::wregex(L"g([0-9]+),([a-z]+)")))
                            {
                                // This is a METRIC_EVT_NOTE.
                                auto it = metrics.find(m[2]);
                                if (it == metrics.end())
                                {
                                    // Not a valide builtin metric.
                                    return false;
                                }
                                counting_info.evt_note.type = METRIC_EVT_NOTE;
                                counting_info.evt_note.note.metric_note.group_id = std::stoi(m[1]);
                                counting_info.evt_note.note.metric_note.name = it->first.c_str();
                            }
                            else if (std::regex_match(note, m, std::wregex(L"g([0-9]+)")))
                            {
                                // This is a GROUP_EVT_NOTE.
                                counting_info.evt_note.type = GROUP_EVT_NOTE;
                                counting_info.evt_note.note.group_note.group_id = std::stoi(m[1]);
                            }
                            else
                            {
                                // Not a valid event note type.
                                return false;
                            }
                        }

                        counting_info.multiplexed_scheduled = evt->scheduled;
                        counting_info.multiplexed_round = round;
                        counting_info.scaled_value = (uint64_t)((double)evt->value / ((double)evt->scheduled / (double)round));
                        __countings[i].push_back(counting_info);
                    }
                }
            }
        }
        else
        {
            std::vector<uint8_t> counting_cores = __pmu_device->get_cores_idx();
            if (core_index >= counting_cores.size())
            {
                // No more core to yield.
                return false;
            }

            uint8_t core_idx = counting_cores[core_index];
            if (event_index >= __countings[core_idx].size())
            {
                // Start all over for the next core.
                event_index = 0;
                core_index++;
                if (core_index >= counting_cores.size())
                {
                    // No more core to yield.
                    return false;
                }
                core_idx = counting_cores[core_index];
            }

            // Yield next stat_info.
            stat_info->core_idx = core_idx;
            stat_info->counter_value = __countings[core_idx][event_index].counter_value;
            stat_info->event_idx = __countings[core_idx][event_index].event_idx;
            stat_info->multiplexed_scheduled = __countings[core_idx][event_index].multiplexed_scheduled;
            stat_info->multiplexed_round = __countings[core_idx][event_index].multiplexed_round;
            stat_info->scaled_value = __countings[core_idx][event_index].scaled_value;
            stat_info->evt_note = __countings[core_idx][event_index].evt_note;
            event_index++;
        }
    }
    catch (...)
    {
        return false;
    }

    return true;
}