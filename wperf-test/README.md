# wperf-test

Project `wperf-test` is VS Native Unit Test project. It references `wperf` project. Its purpose is to contain and execute `wperf`'s unit tests.

# Building and running unit tests

Note: You can currently build unit tests only on `x64` host!

Build and run `wperf` unit tests with:

| Command                    | Configuration     |
| ---                        | ---               |
| `make test`                | Default Debug/x64 |
| `make config=Release test` | Release/x64       |

or more verbose version of above:

| Command                                         | Configuration     |
| ---                                             | ---               |
| `make wperf-test wperf-test-run`                | Default Debug/x64 |
| `make config=Release wperf-test wperf-test-run` | Release/x64       |

The `wperf-test` project require `wperf` project to be built first. WindowsPerf VS solution defines this build dependency and you can just run `make ... wperf-test` and all dependencies will build.

Note: You can use `config=` command line option to switch between `Debug` and `Release` configurations.

## Example make test output

```
> make test
...
1>------ Rebuild All started: Project: wperf (wperf\wperf), Configuration: Debug x64 ------
...
2>------ Rebuild All started: Project: wperf-test, Configuration: Debug x64 ------
...
========== Rebuild All: 2 succeeded, 0 failed, 0 skipped ==========
========== Rebuild started at 9:59 AM and took 12.367 seconds ==========
vstest.console x64\Debug\wperf-test.dll
Microsoft (R) Test Execution Command Line Tool Version 17.5.0 (x64)
Copyright (c) Microsoft Corporation.  All rights reserved.

Starting test execution, please wait...
A total of 1 test files matched the specified pattern.
  Passed test_check_cores_in_pmu_ctl_hdr_p_null [< 1 ms]
  Passed test_check_cores_in_pmu_ctl_hdr_p_cores_count [< 1 ms]
  Passed test_check_cores_in_pmu_ctl_hdr_p_cores_no_error [< 1 ms]
  Passed test_check_cores_in_pmu_ctl_hdr_p_cores_no [< 1 ms]
  Passed test_gen_pdb_name_dll [< 1 ms]
  Passed test_gen_pdb_name_exe [< 1 ms]
  Passed test_gen_pdb_name_dll_path [< 1 ms]
  Passed test_gen_pdb_name_multiple_dll_dots [< 1 ms]
  Passed test_MultiByteFromWideString [< 1 ms]
  Passed test_IntToHexWideString [< 1 ms]
  Passed test_IntToHexWideString_wchar_t [< 1 ms]
  Passed test_IntToDecWideString [< 1 ms]
  Passed test_DoubleToWideString [< 1 ms]
  Passed test_DoubleToWideStringExt [< 1 ms]
  Passed test_TokenizeWideStringOfInts_error [< 1 ms]
  Passed test_TokenizeWideStringOfInts_empty [< 1 ms]
  Passed test_TokenizeWideStringOfInts_clear_output [< 1 ms]
  Passed test_TokenizeWideStringOfInts_0 [< 1 ms]
  Passed test_TokenizeWideStringOfInts_012 [< 1 ms]
  Passed test_TokenizeWideStringOfInts_fibb [< 1 ms]
  Passed test_ReplaceFileExtension [< 1 ms]
  Passed test_IntToDecWithCommas [< 1 ms]
  Passed test_IntToDecWithCommas_neg [< 1 ms]
  Passed test_IntToDecWithCommas_Ramanujan_Tau [< 1 ms]
  Passed test_tablejson_char [< 1 ms]
  Passed test_tablejson_wchar [< 1 ms]
  Passed test_jsonobject_char [< 1 ms]
  Passed test_jsonobject_wchar [< 1 ms]

Test Run Successful.
Total tests: 28
     Passed: 28
 Total time: 0.3613 Seconds
```

`VSTest.Console.exe` command-line tool executes all unit tests in the project and presents user with simple summary. If you want to learn more read [VSTest.Console.exe command-line options](https://learn.microsoft.com/en-us/visualstudio/test/vstest-console-options?view=vs-2022) article.

# Writing tests

Test code is written in C++ Microsoft Native Unit Test Framework.

Read [Microsoft.VisualStudio.TestTools.CppUnitTestFramework API reference](https://learn.microsoft.com/en-us/visualstudio/test/microsoft-visualstudio-testtools-cppunittestframework-api-reference?view=vs-2022) article to learrn how to write C++ unit tests based on the Microsoft Native Unit Test Framework.

# Command line testing introduction

You can run `wperf`'s unit test project `wperf-test` from the command line with top-level `Makefile`. Below documentation describes how to interact with `Makefile` in order to run unit tests for `wperf`.

Note: You can also run unit tests from VS. See [Get started with unit testing](https://learn.microsoft.com/en-us/visualstudio/test/getting-started-with-unit-testing?view=vs-2022&tabs=cpp%2Cmsunittest#run-unit-tests) article for more details.

---

Now when we have unit testing project we depend on x64 `wperf` build to link tests with `wperf-test` unit test project.

In order to satisfy `wperf-test`'s link dependencies `Makefile` command `make all` will:

1. Build `wperf` with `arch=x64`.
2. Build `wperf-test` with `arch=x64` (this project will link specific `.obj` files from `wperf`).
3. Rebuild solution (`wperf`, `wperf-driver`) for `ARM64`.

Please note we are building:

- cross x64 -> ARM64 for WindowsPerf suite and
- x64 -> x64 `wperf` and its unit tests.
