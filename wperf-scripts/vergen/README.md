[[_TOC_]]

# Build step generators

This scripts generate meta-data used in build steps.

## gengitver.ps1

Generates current Git repo HEAD SHA and defined WPERF_GIT_VER_STR wide string which contain it.
This is used in pre-build step in e.g. `wperf` project.

```
PS windowsperf\wperf-scripts\vergen\> .\gengitver.ps1
#define WPERF_GIT_VER_STR L"ba0233ae-dirty"
```
