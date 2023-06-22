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
static std::vector<std::wstring> __list_metrics;
static std::map<std::wstring, std::vector<uint16_t>> __list_metrics_events;
static std::map<enum evt_class, std::vector<struct evt_noted>> __ioctl_events;
static std::map<uint8_t, std::vector<COUNTING_INFO>> __countings;
static std::vector<TEST_INFO> __tests;

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
        __list_metrics.clear();
        __list_metrics_events.clear();
        __ioctl_events.clear();
        __countings.clear();
        __tests.clear();
    }
    catch(...)
    {
        return false;
    }

    return true;
}

extern "C" bool wperf_driver_version(PVERSION_INFO driver_ver)
{
    if (!driver_ver || !__pmu_device)
    {
        // driver_ver and __pmu_device should not be NULL.
        return false;
    }

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
    if (!wperf_ver)
    {
        // wperf_ver should not be NULL.
        return false;
    }

    wperf_ver->major = MAJOR;
    wperf_ver->minor = MINOR;
    wperf_ver->patch = PATCH;

    return true;
}

extern "C" bool wperf_list_events(PLIST_CONF list_conf, PEVENT_INFO einfo)
{
    if (!list_conf || !__pmu_device)
    {
        // list_conf and __pmu_device should not be NULL.
        return false;
    }

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

extern "C" bool wperf_list_num_events(PLIST_CONF list_conf, int *num_events)
{
    if (!list_conf || !num_events)
    {
        // list_conf and num_events should not be NULL.
        return false;
    }

    if (list_conf->list_event_types & CORE_EVT)
    {
        // Only core events are supported now.
        *num_events = (int)__list_events[EVT_CORE].size();
    }

    return true;
}

extern "C" bool wperf_list_metrics(PLIST_CONF list_conf, PMETRIC_INFO minfo)
{
    if (!list_conf || !__pmu_device)
    {
        // list_conf and __pmu_device should not be NULL.
        return false;
    }

    static size_t metrics_index = 0;
    static size_t event_index = 0;

    try
    {
        if (!minfo)
        {
            // Initialize __list_metrics with the list of all builtin metrics.
            // Initialize __list_metrics_events with the list of events for each metric.
            __list_metrics.clear();
            __list_metrics_events.clear();
            metrics_index = 0;
            event_index = 0;
            std::map<std::wstring, metric_desc>& builtin_metrics = __pmu_device->builtin_metrics;
            for (auto [metric, desc] : builtin_metrics)
            {
                __list_metrics.push_back(metric);
                for (const auto& event : desc.groups[EVT_CORE])
                {
                    if (event.type != EVT_HDR)
                    {
                        __list_metrics_events[metric].push_back(event.index);
                    }
                }
            }
        }
        else if (list_conf->list_event_types & CORE_EVT)
        {
            if (metrics_index >= __list_metrics.size())
            {
                // No more metrics.
                return false;
            }

            auto& metric = __list_metrics[metrics_index];
            if (event_index >= __list_metrics_events[metric].size())
            {
                // No more events for the current metric, move on to the next one.
                metrics_index++;
                event_index = 0;
                if (metrics_index >= __list_metrics.size())
                {
                    // No more metrics.
                    return false;
                }
                metric = __list_metrics[metrics_index];
            }

			// Yield the next event for the current metric.
			minfo->metric_name = metric.c_str();
			minfo->event_idx = __list_metrics_events[metric][event_index];
			event_index++;
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

extern "C" bool wperf_list_num_metrics(PLIST_CONF list_conf, int *num_metrics)
{
    if (!list_conf || !num_metrics)
    {
        // list_conf and num_metrics should not be NULL.
        return false;
    }

    if (list_conf->list_event_types & CORE_EVT)
    {
        // Only core events are supported now.
        *num_metrics = (int)__list_metrics.size();
    }

    return true;
}

extern "C" bool wperf_list_num_metrics_events(PLIST_CONF list_conf, int *num_metrics_events)
{
    if (!list_conf || !num_metrics_events)
    {
        // list_conf and num_metrics_events should not be NULL.
        return false;
    }

    if (list_conf->list_event_types & CORE_EVT)
    {
        // Only core events are supported now.
        int num_events = 0;
        for (auto& [_, events] : __list_metrics_events)
        {
            num_events += (int)events.size();
        }
        *num_metrics_events = num_events;
    }

    return true;
}

extern "C" bool wperf_stat(PSTAT_CONF stat_conf, PSTAT_INFO stat_info)
{
    if (!stat_conf || !__pmu_device || !__pmu_cfg)
    {
        // stat_conf, __pmu_device and __pmu_cfg should not be NULL.
        return false;
    }

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
                const ReadOut* core_outs = __pmu_device->get_core_outs();

                std::vector<uint8_t> counting_cores = __pmu_device->get_cores_idx();
                for (auto i : counting_cores)
                {
                    UINT32 evt_num = core_outs[i].evt_num;
                    const struct pmu_event_usr* evts = core_outs[i].evts;
                    uint64_t round = core_outs[i].round;
                    for (UINT32 j = 0; j < evt_num; j++)
                    {
                        // Ignore padding events.
                        if (j >= 1 && (__ioctl_events[EVT_CORE][j - 1].type == EVT_PADDING))
                            continue;

                        const struct pmu_event_usr* evt = &evts[j];
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

extern "C" bool wperf_num_cores(int *num_cores)
{
    if (!num_cores || !__pmu_device)
    {
        // num_cores and __pmu_device should not be NULL.
        return false;
    }

    *num_cores = __pmu_device->core_num;
    return true;
}

static inline void wperf_test_insert_int(const wchar_t *name, int result)
{
    TEST_INFO tinfo;
    tinfo.name = name;
    tinfo.type = INT_RESULT;
    tinfo.int_result = result;
    __tests.push_back(tinfo);
}

static inline void wperf_test_insert_bool(const wchar_t *name, bool result)
{
    TEST_INFO tinfo;
    tinfo.name = name;
    tinfo.type = BOOL_RESULT;
    tinfo.bool_result = result;
    __tests.push_back(tinfo);
}

static inline void wperf_test_insert_evt_note(const wchar_t *name, struct evt_noted result)
{
    TEST_INFO tinfo;
    tinfo.name = name;
    tinfo.type = EVT_NOTE_RESULT;
    tinfo.evt_note_result.index = result.index;
    tinfo.evt_note_result.type = result.type;
    __tests.push_back(tinfo);
}

static inline void wperf_test_insert_wstring(const wchar_t *name, const wchar_t* result)
{
    TEST_INFO tinfo;
    tinfo.name = name;
    tinfo.type = WSTRING_RESULT;
    tinfo.wstring_result = result;
    __tests.push_back(tinfo);
}

extern "C" bool wperf_test(PTEST_CONF tconf, PTEST_INFO tinfo)
{
    if (!tconf || !__pmu_device || !__pmu_cfg)
    {
        // tconf, __pmu_device, and __pmu_cfg should not be NULL.
        return false;
    }

    static size_t test_index = 0;

    try
    {
        if (!tinfo)
        {
            __ioctl_events.clear();
            __tests.clear();
            test_index = 0;
            struct hw_cfg hw_cfg;
            __pmu_device->query_hw_cfg(hw_cfg);

            // Only CORE events are supported at the moment.
            std::vector<enum evt_class> e_classes = { EVT_CORE };
            uint32_t enable_bits = __pmu_device->enable_bits(e_classes);

            // Tests for request.ioctl_events
            wperf_test_insert_bool(L"request.ioctl_events [EVT_CORE]", enable_bits & CTL_FLAG_CORE);
            wperf_test_insert_bool(L"request.ioctl_events [EVT_DSU]", enable_bits & CTL_FLAG_DSU);
            wperf_test_insert_bool(L"request.ioctl_events [EVT_DMC_CLK/EVT_DMC_CLKDIV2]", enable_bits & CTL_FLAG_DMC);

            // Test for pmu_device.vendor_name
            wperf_test_insert_wstring(L"pmu_device.vendor_name", __pmu_device->get_vendor_name(hw_cfg.vendor_id));

            // Test for product name (if available)
            wperf_test_insert_wstring(L"pmu_device.product_name", __pmu_device->m_product_name.c_str());
            wperf_test_insert_wstring(L"pmu_device.product_name(extended)", __pmu_device->get_product_name_ext().c_str());

            // Tests for pmu_device.events_query(events)
            std::map<enum evt_class, std::vector<uint16_t>> events;
            __pmu_device->events_query(events);
            wperf_test_insert_int(L"pmu_device.events_query(events) [EVT_CORE]", events.count(EVT_CORE) ? events[EVT_CORE].size() : 0);
            wperf_test_insert_int(L"pmu_device.events_query(events) [EVT_DSU]", events.count(EVT_DSU) ? events[EVT_DSU].size() : 0);
            wperf_test_insert_int(L"pmu_device.events_query(events) [EVT_DMC_CLK]", events.count(EVT_DMC_CLK) ? events[EVT_DMC_CLK].size() : 0);
            wperf_test_insert_int(L"pmu_device.events_query(events) [EVT_DMC_CLKDIV2]", events.count(EVT_DMC_CLKDIV2) ? events[EVT_DMC_CLKDIV2].size() : 0);

            // Tests for PMU_CTL_QUERY_HW_CFG
            wperf_test_insert_int(L"PMU_CTL_QUERY_HW_CFG [arch_id]", hw_cfg.arch_id);
            wperf_test_insert_int(L"PMU_CTL_QUERY_HW_CFG [core_num]", hw_cfg.core_num);
            wperf_test_insert_int(L"PMU_CTL_QUERY_HW_CFG [fpc_num]", hw_cfg.fpc_num);
            wperf_test_insert_int(L"PMU_CTL_QUERY_HW_CFG [gpc_num]", hw_cfg.gpc_num);
            wperf_test_insert_int(L"PMU_CTL_QUERY_HW_CFG [total_gpc_num]", hw_cfg.total_gpc_num);
            wperf_test_insert_int(L"PMU_CTL_QUERY_HW_CFG [part_id]", hw_cfg.part_id);
            wperf_test_insert_int(L"PMU_CTL_QUERY_HW_CFG [pmu_ver]", hw_cfg.pmu_ver);
            wperf_test_insert_int(L"PMU_CTL_QUERY_HW_CFG [rev_id]", hw_cfg.rev_id);
            wperf_test_insert_int(L"PMU_CTL_QUERY_HW_CFG [variant_id]", hw_cfg.variant_id);
            wperf_test_insert_int(L"PMU_CTL_QUERY_HW_CFG [vendor_id]", hw_cfg.vendor_id);
            wperf_test_insert_int(L"PMU_CTL_QUERY_HW_CFG [midr_value]", hw_cfg.midr_value);

            // Tests General Purpose Counters detection
            wperf_test_insert_int(L"gpc_nums[EVT_CORE]", __pmu_device->gpc_nums[EVT_CORE]);
            wperf_test_insert_int(L"gpc_nums[EVT_DSU]", __pmu_device->gpc_nums[EVT_DSU]);
            wperf_test_insert_int(L"gpc_nums[EVT_DMC_CLK]", __pmu_device->gpc_nums[EVT_DMC_CLK]);
            wperf_test_insert_int(L"gpc_nums[EVT_DMC_CLKDIV2]", __pmu_device->gpc_nums[EVT_DMC_CLKDIV2]);

            // Tests Fixed Purpose Counters detection
            wperf_test_insert_int(L"fpc_nums[EVT_CORE]", __pmu_device->fpc_nums[EVT_CORE]);
            wperf_test_insert_int(L"fpc_nums[EVT_DSU]", __pmu_device->fpc_nums[EVT_DSU]);
            wperf_test_insert_int(L"fpc_nums[EVT_DMC_CLK]", __pmu_device->fpc_nums[EVT_DMC_CLK]);
            wperf_test_insert_int(L"fpc_nums[EVT_DMC_CLKDIV2]", __pmu_device->fpc_nums[EVT_DMC_CLKDIV2]);

            std::map<enum evt_class, std::deque<struct evt_noted>> events_map;
            for (int i = 0; i < tconf->num_events; i++)
            {
                events_map[EVT_CORE].push_back({tconf->events[i], EVT_NORMAL, L"e"});
            }
            std::map<std::wstring, metric_desc>& metrics = __pmu_device->builtin_metrics;
            for (int i = 0; i < tconf->num_metrics; i++)
            {
                const wchar_t* metric = tconf->metric_events[i];
                if (metrics.find(metric) == metrics.end())
                {
                    // Not a valid builtin metric.
                    return false;
                }

                metric_desc desc = metrics[metric];
                for (const auto& x : desc.events)
                    events_map[x.first].insert(events_map[x.first].end(), x.second.begin(), x.second.end());
            }

            std::map<enum evt_class, std::vector<struct evt_noted>> groups;
            set_event_padding(__ioctl_events, *__pmu_cfg, events_map, groups);

            for (const auto& e : __ioctl_events[EVT_CORE])
            {
                wperf_test_insert_evt_note(L"ioctl_events[EVT_CORE]", e);
            }
        }
        else
        {
            if (test_index >= __tests.size())
            {
                return false;
            }

            *tinfo = __tests[test_index];
            test_index++;
        }
    }
    catch (...)
    {
        return false;
    }

    return true;
}
