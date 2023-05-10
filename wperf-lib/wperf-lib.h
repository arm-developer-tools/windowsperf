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

#ifdef __cplusplus
}
#endif