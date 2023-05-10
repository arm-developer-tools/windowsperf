#include "wperf-lib.h"
#include "pmu_device.h"
#include "wperf-common/public.h"

static pmu_device* __pmu_device = nullptr;
static struct pmu_device_cfg* __pmu_cfg = nullptr;
static std::map<enum evt_class, std::vector<uint16_t>> __list_events;
static size_t __list_index = 0;

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