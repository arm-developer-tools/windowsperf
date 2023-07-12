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

typedef struct _METRIC_INFO
{
    /// Metric name
    const wchar_t* metric_name;
    /// Event ID
    uint16_t event_idx;
} METRIC_INFO, *PMETRIC_INFO;

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
///
/// LIST_CONF list_conf = { CORE_EVT /* Only list Core PMU events */ };
/// EVENT_INFO einfo;
/// if (wperf_list_events(&list_conf, NULL))
/// {
///   while (wperf_list_events(&list_conf, &einfo))
///   {
///     printf("Event type=%d, id=%u, name=%ls\n", einfo.type, einfo.id, einfo.name);
///   }
/// }
///
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
/// <returns>true if the call succeeds, false if not.</returns>
bool wperf_list_events(PLIST_CONF list_conf, PEVENT_INFO einfo);

/// <summary>
/// Get the number of supported events. This must be called after wperf_list_events.
/// </summary>
/// <example> This example shows how to call the wperf_list_num_events routine.
/// <code>
/// wperf_init();
///
/// LIST_CONF list_conf = { CORE_EVT /* Only list Core PMU events */ };
/// if (wperf_list_events(&list_conf, NULL))
/// {
///   int num_events;
///   if (wperf_list_num_events(&list_conf, &num_events))
///   {
///     printf("num_events=%d\n", num_events);
///   }
/// }
///
/// wperf_close();
/// </code>
/// </example>
/// <param name="list_conf">Pointer to a caller-allocated LIST_CONF struct.
/// Users configure which type of PMU events to list through setting the fields
/// in LIST_CONF (refer to the definition of LIST_CONF for more details).</param>
/// <param name="num_events">Pointer to a caller allocated int. This lib routine will
/// fill the int pointed to by num_events with the number of supported events.</param>
/// <returns>true if the call succeeds, false if not.</returns>
bool wperf_list_num_events(PLIST_CONF list_conf, int *num_events);

/// <summary>
/// Works like a generator, yields the next event from the list of events
/// for all builtin metrics each time it's called.
/// </summary>
/// <example> This example shows how to call the wperf_list_metrics routine.
/// <code>
/// wperf_init();
///
/// LIST_CONF list_conf = { CORE_EVT /* Only list Core PMU events */ };
/// METRIC_INFO minfo;
/// if (wperf_list_metrics(&list_conf, NULL))
/// {
///   while (wperf_list_metrics(&list_conf, &minfo))
///   {
///     printf("Metric name=%ls, event_idx=%u\n", minfo.metric_name, minfo.event_idx);
///   }
/// }
///
/// wperf_close();
/// </code>
/// </example>
/// <param name="list_conf">Pointer to a caller-allocated LIST_CONF struct.
/// Users configure which type of PMU events to list through setting the fields
/// in LIST_CONF (refer to the definition of LIST_CONF for more details).</param>
/// <param name="minfo">Setting minfo to NULL, this routine will retrieve the list
/// of events for all builtin metrics internally and be ready for subsequent calls
/// to yield event by event for all builtin metrics. When passed as a pointer to a
/// caller-allocated METRIC__INFO struct. This lib routine will populate the struct
/// pointed to by minfo with the next event from the list of events for all builtin
/// metrics.</param>
/// <returns>true if the call succeeds, false if not.</returns>
bool wperf_list_metrics(PLIST_CONF list_conf, PMETRIC_INFO minfo);

/// <summary>
/// Get the number of builtin metrics. This must be called after wperf_list_metrics.
/// </summary>
/// <example> This example shows how to call the wperf_list_num_metrics routine.
/// <code>
/// wperf_init();
///
/// LIST_CONF list_conf = { CORE_EVT /* Only list Core PMU events */ };
/// if (wperf_list_metrics(&list_conf, NULL))
/// {
///   int num_metrics;
///   if (wperf_list_num_metrics(&list_conf, &num_metrics))
///   {
///     printf("num_metrics=%d\n", num_metrics);
///   }
/// }
///
/// wperf_close();
/// </code>
/// </example>
/// <param name="list_conf">Pointer to a caller-allocated LIST_CONF struct.
/// Users configure which type of PMU events to list through setting the fields
/// in LIST_CONF (refer to the definition of LIST_CONF for more details).</param>
/// <param name="num_metrics">Pointer to a caller allocated int. This lib routine will fill
/// the int pointed to by num_metrics with the number of builtin metrics.</param>
/// <returns>true if the call succeeds, false if not.</returns>
bool wperf_list_num_metrics(PLIST_CONF list_conf, int *num_metrics);

/// <summary>
/// Get the number of events for all builtin metrics. This must be called after wperf_list_metrics.
/// </summary>
/// <example> This example shows how to call the wperf_list_num_metrics_events routine.
/// <code>
/// wperf_init();
///
/// LIST_CONF list_conf = { CORE_EVT /* Only list Core PMU events */ };
/// if (wperf_list_metrics(&list_conf, NULL))
/// {
///   int num_metrics_events;
///   if (wperf_list_num_metrics_events(&list_conf, &num_metrics_events))
///   {
///     printf("num_metrics_events=%d\n", num_metrics_events);
///   }
/// }
///
/// wperf_close();
/// </code>
/// </example>
/// <param name="list_conf">Pointer to a caller-allocated LIST_CONF struct.
/// Users configure which type of PMU events to list through setting the fields
/// in LIST_CONF (refer to the definition of LIST_CONF for more details).</param>
/// <param name="num_metrics_events"></param>
/// <returns>true if the call succeeds, false if not.</returns>
bool wperf_list_num_metrics_events(PLIST_CONF list_conf, int *num_metrics_events);

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

typedef struct _TEST_CONF
{
    /// The number of normal events to count.
    /// With "-e inst_spec,dp_spec", this value is 2).
    int num_events;
    /// The list of normal events to count.
    /// With "-e inst_spec,dp_spec", this array looks like {0x1B (ID of inst_spec), 0x73 (ID of dp_spec)}.
    uint16_t* events;
    /// The number of metrics to count.
    /// With "-m dcache", this value is 1.
    int num_metrics;
    /// The list of metrics to count.
    /// With "-m dcache", this looks like {"dcache"}.
    const wchar_t** metric_events;
} TEST_CONF, *PTEST_CONF;

typedef enum _RESULT_TYPE
{
    NUM_RESULT,
    BOOL_RESULT,
    EVT_NOTE_RESULT,
    WSTRING_RESULT,
} RESULT_TYPE;

typedef struct _TEST_INFO
{
    /// Test name
    const wchar_t* name;
    /// Test result
    RESULT_TYPE type;
    union {
        uint64_t num_result;
        bool bool_result;
        const wchar_t* wstring_result;
        struct {
            uint16_t index;
            enum evt_type type;
        } evt_note_result;
    };
} TEST_INFO, *PTEST_INFO;

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
/// <returns>true if the call succeeds, false if not.</returns>
bool wperf_stat(PSTAT_CONF stat_conf, PSTAT_INFO stat_info);

typedef struct _SAMPLE_CONF
{
    /// The PE file path.
    const wchar_t *pe_file;
    /// The PDB file path.
    const wchar_t *pdb_file;
    /// The name of the image to sample.
    const wchar_t *image_name;
    /// The index of the core on which the image runs.
    uint8_t core_idx;
    /// The number of normal events to sample.
    /// With "-e inst_spec:100000,dp_spec:200000", this value is 2).
    int num_events;
    /// The list of normal events to sample.
    /// With "-e inst_spec:100000,dp_spec:200000", this array looks like {0x1B (ID of inst_spec), 0x73 (ID of dp_spec)}.
    uint16_t *events;
    /// The list of sampling intervals for each sampled event.
    /// With "-e inst_spec:100000,dp_spec:200000", this array looks like {100000, 200000}.
    uint32_t *intervals;
    /// Set this to true to get short symbol names, false to get long symbol names.
    bool display_short;
    /// Sampling duration (in seconds, accurary of 0.1 sec).
    double duration;
    /// Set this to true if kernel mode should be included, false if not.
    bool kernel_mode;
    /// Set this to true if funtions should be annotated, false if not.
    bool annotate;
} SAMPLE_CONF, *PSAMPLE_CONF;

typedef struct _SAMPLE_INFO
{
    uint16_t event;
    const wchar_t* symbol;
    uint32_t count;
    double overhead;
} SAMPLE_INFO, *PSAMPLE_INFO;

typedef struct _ANNOTATE_INFO
{
    uint16_t event;
    const wchar_t* symbol;
    const wchar_t* source_file;
    uint32_t line_number;
    uint64_t hits;
} ANNOTATE_INFO, * PANNOTATE_INFO;

typedef struct _SAMPLE_STATS
{
    uint64_t sample_generated;
    uint64_t sample_dropped;
} SAMPLE_STATS, *PSAMPLE_STATS;

/// <summary>
/// Works like a generator, yields the next SAMPLE_INFO from the list of all SAMPLE_INFOs
/// each time it's called sample by sample for all requested events.
/// </summary>
/// <example> This example shows how to call the wperf_sample routine.
/// <code>
/// wperf_init();
///
/// uint16_t sample_events[2] = { 0x70, 0x71 };
/// uint32_t intervals[2] = { 100000, 200000 };
/// SAMPLE_CONF sample_conf =
/// {
///   L"c:\\cpython\\PCbuild\\arm64\\python_d.exe", // pe_file
///   L"c:\\cpython\\PCbuild\\arm64\\python_d.pdb", // pdb_file
///   L"python_d.exe", // image_name
///   1, // core_idx
///   2, // num_events
///   sample_events, // events
///   intervals, // intervals
///   true, // display_short
///   10, // duration
///   false // kernel_mode
/// }
///
/// if (wperf_sample(&sample_conf, NULL))
/// {
///   SAMPLE_INFO sample_info;
///   while (wperf_sample(&sample_conf, &sample_info))
///   {
///     printf("sample event=%u, name=%ls, count=%u, overhead=%f\n", sample_info.event, sample_info.symbol, sample_info.count, sample_info.overhead);
///   }
/// }
///
/// wperf_close();
/// </code>
/// </example>
/// <param name="sample_conf">Pointer to a caller-allocated SAMPLE_CONF struct. User configures
/// the various parameters of the sampling mode through setting the fields in sample_conf
/// (refer to the definition of SAMPLE_CONF for more details).</param>
/// <param name="sample_info">Setting sample_info to NULL, this lib routine will collect
/// samples for all the requested events internally and be ready for subsequent calls to
/// yield sample info sample by sample for all requestd sampling events. When passed as a pointer
/// to a caller-allocated SAMPLE_INFO structure. The lib routine will populate the SAMPLE_INFO
/// pointed to by sample_info with the sample information defined in SAMPLE_INFO for each sample
/// of each requested sampling event.</param>
/// <returns>true if the call succeeds, false if not.</returns>
bool wperf_sample(PSAMPLE_CONF sample_conf, PSAMPLE_INFO sample_info);

/// <summary>
/// Works like a generator, yields the next ANNOTATE_INFO from the list of all ANNOTATE_INFOs
/// each time it's called sample by sample for all requested events. Compared to wperf_sample,
/// this routine returns finer grained details like the line of source code where the event occurs.
/// The wperf_sample API must be called first with sample_info set to NULL and sample_conf->annotate
/// set to true before calling this API.
/// </summary>
/// <example> This example shows how to call the wperf_sample_annotate routine.
/// <code>
/// wperf_init();
///
/// uint16_t sample_events[2] = { 0x70, 0x71 };
/// uint32_t intervals[2] = { 100000, 200000 };
/// SAMPLE_CONF sample_conf =
/// {
///   L"c:\\cpython\\PCbuild\\arm64\\python_d.exe", // pe_file
///   L"c:\\cpython\\PCbuild\\arm64\\python_d.pdb", // pdb_file
///   L"python_d.exe", // image_name
///   1, // core_idx
///   2, // num_events
///   sample_events, // events
///   intervals, // intervals
///   true, // display_short
///   10, // duration
///   false, // kernel_mode
///   true, // annotate
/// }
///
/// if (wperf_sample(&sample_conf, NULL))
/// {
///   ANNOTATE_INFO annotate_info;
///   while (wperf_sample_annotate(&sample_conf, &annotate_info))
///   {
///     printf("wperf_sample_annotate: event=%u, name=%ls, source=%ls, line=%u, hits=%llu\n", annotate_info.event, annotate_info.symbol, annotate_info.source_file, annotate_info.line_number, annotate_info.hits);
///   }
/// }
///
/// wperf_close();
/// </code>
/// </example>
/// <param name="sample_conf">Pointer to a caller-allocated SAMPLE_CONF struct. User configures
/// the various parameters of the sampling mode through setting the fields in sample_conf
/// (refer to the definition of SAMPLE_CONF for more details).</param>
/// <param name="annotate_info">Pointer to a caller-allocated ANNOTATE_INFO structure. The lib
/// routine will populate the ANNOTATE_INFO pointed to by annotate_info with the sample information
/// defined in ANNOTATE_INFO for each requested sampling event.</param>
/// <returns>true if the call succeeds, false if not.</returns>
bool wperf_sample_annotate(PSAMPLE_CONF sample_conf, PANNOTATE_INFO annotate_info);

/// <summary>
/// Get sampling statistics.
/// </summary>
/// <example> This example shows how to call the wperf_sample_stats routine.
/// <code>
/// wperf_init();
///
/// uint16_t sample_events[2] = { 0x70, 0x71 };
/// uint32_t intervals[2] = { 100000, 200000 };
/// SAMPLE_CONF sample_conf =
/// {
///   L"c:\\cpython\\PCbuild\\arm64\\python_d.exe", // pe_file
///   L"c:\\cpython\\PCbuild\\arm64\\python_d.pdb", // pdb_file
///   L"python_d.exe", // image_name
///   1, // core_idx
///   2, // num_events
///   sample_events, // events
///   intervals, // intervals
///   true, // display_short
///   10, // duration
///   false // kernel_mode
/// }
///
/// if (wperf_sample(&sample_conf, NULL))
/// {
///   SAMPLE_STATS sample_stats;
///   if (wperf_sample_stats(&sample_conf, &sample_stats))
///   {
///     printf("sample_generated=%llu, sample_dropped=%llu\n", sample_stats.sample_generated, sample_stats.sample_dropped);
///   }
/// }
///
/// wperf_close();
/// </code>
/// </example>
/// <param name="sample_conf">Pointer to a caller-allocated SAMPLE_CONF struct. User configures
/// the various parameters of the sampling mode through setting the fields in sample_conf
/// (refer to the definition of SAMPLE_CONF for more details).</param>
/// <param name="sample_stats">Pointer to a caller-allocated SAMPLE_STATS structure. The lib routine
/// will populate the SAMPLE_STATS pointed to by sample_stats with the sampling statistics (refer to
/// the definition of SAMPLE_STATS for more details).</param>
/// <returns>true if the call succeeds, false if not.</returns>
bool wperf_sample_stats(PSAMPLE_CONF sample_conf, PSAMPLE_STATS sample_stats);

/// <summary>
/// Get the number of CPU cores.
/// </summary>
/// <example> This example shows how to call the wperf_num_cores routine.
/// <code>
/// wperf_init();
///
/// int num_cores;
/// if (wperf_num_cores(&num_cores))
/// {
///   printf("num_cores=%d\n", num_cores);
/// }
///
/// wperf_close();
/// </code>
/// </example>
/// <param name="num_cores">Pointer to a caller allocated int. This lib routine will
/// fill the int pointed to by num_cores with the number of cores.</param>
/// <returns>true if the call succeeds, false if not.</returns>
bool wperf_num_cores(int *num_cores);

/// <summary>
/// Works like a generator, yields the next TEST_INFO from the list of tests.
/// </summary>
/// <example> This example shows how to call the wperf_test routine.
/// <code>
/// wperf_init();
///
/// TEST_CONF test_conf = {};
/// TEST_INFO tinfo = {};
/// if (wperf_test(&test_conf, NULL))
/// {
///   while (wperf_test(&test_conf, &tinfo))
///   {
///     printf("%ls ", tinfo.name);
///     if (tinfo.type == WSTRING_RESULT)
///     {
///         printf("%ls", tinfo.wstring_result);
///     }
///     else if (tinfo.type == BOOL_RESULT)
///     {
///         printf("%s", tinfo.bool_result ? "True", "False");
///     }
///     else if (tinfo.type == NUM_RESULT)
///     {
///         printf("0x%x", tinfo.num_result);
///     }
///     printf("\n");
///   }
/// }
///
/// wperf_close();
/// </code>
/// </example>
/// <param name="tconf">Pointer to a caller-allocated LIST_CONF struct.
/// Users configure which type of PMU events to list configuration info on
/// through setting the fields in TEST_CONF (refer to the definition of TEST_CONF
/// for more details).</param>
/// <param name="tinfo">Setting tinfo to NULL, this routine will retrieve the
/// full list of configuration info internally and be ready for subsequent calls
/// to yield each info. When passed as a pointer to a caller-allocated
/// TEST_INFO struct, this lib routine will populate the struct pointed to by
/// tinfo with the next info from the list of configuration info.</param>
/// <returns>true if the call succeeds, false if not.</returns>
bool wperf_test(PTEST_CONF tconf, PTEST_INFO tinfo);

#ifdef __cplusplus
}
#endif
