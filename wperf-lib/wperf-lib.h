#pragma once

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _VERSION_INFO
{
    unsigned int major;
    unsigned int minor;
    unsigned int patch;
} VERSION_INFO, *PVERSION_INFO;

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
    unsigned short id;
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
/// EVENT_INFO einfo;
/// if (wperf_list_events(NULL))
/// {
///   while (wperf_list_events(&einfo))
///   {
///     printf("Event type=%d, id=%u\n", einfo.type, einfo.id);
///   }
/// }
/// wperf_close();
/// </code>
/// </example>
/// <param name="einfo">Setting einfo to NULL, this routine will retrieve the
/// full list of supported events internally and be ready for subsequent calls
/// to yield event by event. When passed as a pointer to a caller-allocated
/// EVENT_INFO struct. This lib routine will populate the struct pointed to by
/// einfo with the next event from the list of supported events.</param>
/// <returns></returns>
bool wperf_list_events(PEVENT_INFO einfo);

#ifdef __cplusplus
}
#endif