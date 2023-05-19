#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _VERSION_INFO
{
    unsigned int major;
    unsigned int minor;
    unsigned int patch;
} VERSION_INFO, *PVERSION_INFO;

#define CORE_EVT (1 << 0) // Core PMU event
// TODO: Add DSU_EVT, DMC_EVT etc.

typedef struct _LIST_CONF
{
    // Each bit in list_event_types indicates whether certain types of events
    // should be listed. For example, if list_event_types&CORE_EVT is set, core
    // PMU events should be listed.
    uint64_t list_event_types;
} LIST_CONF, *PLIST_CONF;

typedef enum _EVT_TYPE
{
    CORE_TYPE,
    DSU_TYPE,
    DMC_CLK_TYPE,
    DMC_CLKDIV2_TYPE,
} EVT_TYPE;

typedef struct _EVENT_INFO
{
    EVT_TYPE type;
    uint16_t id;
    const wchar_t* name;
} EVENT_INFO, *PEVENT_INFO;

/// <summary>
/// Initialize wperf-lib.
/// </summary>
/// <returns>true if the call succeeds, false if not.</returns>
bool wperf_init();

/// <summary>
/// Close wperf-lib.
/// </summary>
/// <returns>true if the call succeeds, false if not.</returns>
bool wperf_close();

/// <summary>
/// Get wperf-driver's version.
/// </summary>
/// <example> This example shows how to call the wperf_driver_version routine.
/// <code>
/// wperf_init();
/// VERSION_INFO v;
/// if (wperf_driver_version(&v))
///   printf("wperf-driver version: %u.%u.%u\n", v.major, v.minor, v.patch);
/// wperf_close();
/// </code>
/// </example>
/// <param name="driver_ver">Pointer to caller allocated VERSION_INFO
/// which this routine will populate with wperf-driver's version info.</param>
/// <returns>true if the call succeeds, false if not.</returns>
bool wperf_driver_version(PVERSION_INFO driver_ver);

/// <summary>
/// Get wperf-lib's version.
/// </summary>
/// <example> This example shows how to call the wperf_version routine.
/// <code>
/// wperf_init();
/// VERSION_INFO v;
/// if (wperf_version(&v))
///   printf("wperf-lib version: %u.%u.%u\n", v.major, v.minor, v.patch);
/// wperf_close();
/// </code>
/// </example>
/// <param name="wperf_ver">Pointer to caller allocated VERSION_INFO
/// which this routine will populate with wperf-lib's version info.</param>
/// <returns>true if the call succeeds, false if not.</returns>
bool wperf_version(PVERSION_INFO wperf_ver);

/// <summary>
/// Works like a generator, yields the next element from the list of all
/// supported events each time it's called.
/// </summary>
/// <example> This example shows how to call the wperf_list_events routine.
/// <code>
/// wperf_init();
/// LIST_CONF list_conf = { CORE_EVT /* Only list Core PMU events */ };
/// EVENT_INFO einfo;
/// if (wperf_list_events(&list_conf, NULL))
/// {
///   while (wperf_list_events(&list_conf, &einfo))
///   {
///     printf("Event type=%d, id=%u, name=%ls\n", einfo.type, einfo.id, einfo.name);
///   }
/// }
/// wperf_close();
/// </code>
/// </example>
/// <param name="list_conf">Pointer to a caller-allocated LIST_CONF struct.
/// Users configure which type of PMU events to list through setting the fields
/// in LIST_CONF (refer to the definition of LIST_CONF for more details).</param>
/// <param name="einfo">Setting einfo to NULL, this routine will retrieve the
/// full list of supported events internally and be ready for subsequent calls
/// to yield event by event. When passed as a pointer to a caller-allocated
/// EVENT_INFO struct. This lib routine will populate the struct pointed to by
/// einfo with the next event from the list of supported events.</param>
/// <returns></returns>
bool wperf_list_events(PLIST_CONF list_conf, PEVENT_INFO einfo);

// With this example "-e inst_spec,dp_spec,{ld_spec,st_spec} -m dcache",
// normal events are inst_spec and dp_spec,
// group events are {ld_spec,st_spec},
// metric is dcache.

typedef enum _EVT_NOTE_TYPE
{
    NORMAL_EVT_NOTE,
    GROUP_EVT_NOTE,
    METRIC_EVT_NOTE
} EVT_NOTE_TYPE;

typedef struct _GROUP_NOTE
{
    /// Group ID
    uint16_t group_id;
} GROUP_NOTE;

typedef struct _METRIC_NOTE
{
    /// Group ID
    uint16_t group_id;
    /// Metric name
    const wchar_t* name;
} METRIC_NOTE;

typedef struct _EVENT_NOTE
{
    /// Event type
    EVT_NOTE_TYPE type;
    /// Depending on the event type, this field could be one of the following types.
    union
    {
        GROUP_NOTE group_note;
        METRIC_NOTE metric_note;
    } note;
} EVENT_NOTE;

typedef struct _GROUP_EVENT_INFO
{
    /// The number of event groups to count.
    /// With "-e {ld_spec,st_spec}, this value is 1.
    int num_groups;
    /// This array holds the number of events for each group.
    /// With "-e {ld_spec,st_spec}, this array has only 1 element and looks like {2}.
    int* num_group_events;
    /// This 2-dimensional array holds the list of events for each group.
    /// With "-e {ld_spec,st_spec}, this array looks like {{0x70 (ID of ld_spec), 0x71 (ID of st_spec)}}.
    uint16_t** events;
} GROUP_EVENT_INFO;

typedef struct _STAT_CONF
{
    /// The number of cores to count on.
    /// With "-c 0,3", this value is 2.
    int num_cores;
    /// The list of cores to count on.
    /// With "-c 0,3", this array looks like {0, 3}).
    uint8_t* cores;
    /// The number of normal events to count.
    /// With "-e inst_spec,dp_spec", this value is 2).
    int num_events;
    /// The list of normal events to count.
    /// With "-e inst_spec,dp_spec", this array looks like {0x1B (ID of inst_spec), 0x73 (ID of dp_spec)}.
    uint16_t* events;
    /// The group events to count.
    /// Refer to definition of GROUP_EVENT_INFO for more details.
    GROUP_EVENT_INFO group_events;
    /// The number of metrics to count.
    /// With "-m dcache", this value is 1.
    int num_metrics;
    /// The list of metrics to count.
    /// With "-m dcache", this looks like {"dcache"}.
    const wchar_t** metric_events;
    /// The counting duration (in seconds, accurary of 0.1 sec).
    double duration;
    /// Set this to true if kernel mode should be included, false if not.
    bool kernel_mode;
} STAT_CONF, *PSTAT_CONF;

typedef struct _STAT_INFO
{
    /// Core on which the event was counted.
    uint8_t core_idx;
    /// Counter value.
    uint64_t counter_value;
    /// Event ID.
    uint16_t event_idx;
    /// Time slots scheduled.
    uint64_t multiplexed_scheduled;
    /// Total time slots.
    uint64_t multiplexed_round;
    /// Scaled counter value.
    uint64_t scaled_value;
    /// Event note, refer to definition of EVENT_NOTE for details.
    EVENT_NOTE evt_note;
} STAT_INFO, *PSTAT_INFO;

/// <summary>
/// Works like a generator, yields the next STAT_INFO from the list of all STAT_INFOs
/// each time it's called event by event for all requested cores.
/// </summary>
/// <example> This example shows how to call the wperf_stat routine.
/// <code>
/// wperf_init();
///
/// uint8_t cores[2] = { 0, 3 };
/// uint16_t events[2] = { 0x1B, 0x73 };
/// int num_group_events[1] = { 2 };
/// uint16_t group_events[2] = { 0x70, 0x71 };
/// uint16_t* gevents[1] = { group_events };
/// const wchar_t* metric_events[1] = { L"dcache" };
/// STAT_CONF stat_conf =
/// {
///   2, // num_cores
///   cores, // cores
///   2, // num_events
///   events, // events
///   {1/*num_groups*/, num_group_events/*num_group_events*/, gevents/*events*/}, // group_events
///   1, // num_metrics
///   metric_events, // metric_events
///   1, // duration
///   false // kernel_mode
/// }
///
/// if (wperf_stat(&stat_conf, NULL))
/// {
///   STAT_INFO stat_info;
///   while (wperf_stat(&stat_conf, &stat_info))
///   {
///     printf("core_idx=%u, event_idx=%u, counter=%llu\n", stat_info.core_idx, stat_info.event_idx, stat_info.counter_value);
///   }
/// }
///
/// wperf_close();
/// </code>
/// </example>
/// <param name="stat_conf">Pointer to a caller-allocated STAT_CONF struct. User configures
/// the various parameters of the counting mode through setting the fields in stat_conf
/// (refer to the definition of STAT_CONF for more details).</param>
/// <param name="stat_info">Setting stat_info to NULL, this lib routine will collect
/// counter values for all the requested events internally and be ready for subsequent calls
/// to yield counter values event by event for all requestd cores. When passed as a pointer
/// to a caller-allocated STAT_INFO structure. The lib routine will populate the STAT_INFO
/// pointed to by stat_info with the counter values and other related information defined
/// in STAT_INFO for each requested event on each requestd core.</param>
/// <returns></returns>
bool wperf_stat(PSTAT_CONF stat_conf, PSTAT_INFO stat_info);

#ifdef __cplusplus
}
#endif