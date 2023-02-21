# BSD 3-Clause License
#
# Copyright (c) 2022, Arm Limited
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
#
# 1. Redistributions of source code must retain the above copyright notice, this
#    list of conditions and the following disclaimer.
#
# 2. Redistributions in binary form must reproduce the above copyright notice,
#    this list of conditions and the following disclaimer in the documentation
#    and/or other materials provided with the distribution.
#
# 3. Neither the name of the copyright holder nor the names of its
#    contributors may be used to endorse or promote products derived from
#    this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
# AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
# DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
# FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
# DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
# SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
# CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
# OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#
# Makefile (GNU Make 3.81)
#

.PHONY: all clean docs test wperf wperf-driver wperf-test

#
# *** INTRODUCTION ***
#
# Now when we have unit testing project `wperf-test` we depend on x64
# wperf build to link tests with unit test project. This change makes sure
# we build `wperf-test` for arch=x64. This will build `wperf` and
# `wperf-test` (later depends on `wperf`).
#
# So `make all` will:
#
#     1) Build `wperf` arch=x64
#     2) Build `wperf-test` arch=x64
#     3) Rebuild solution for `config` and `arch` (default ARM64)
#
# Please note we are building:
#
#     * cross x64 -> ARM64 for WindowsPerf suite and
#     * x64 -> x64 `wperf` and its unit tests.
#
# *** HOW TO SWITCH BETWEEN Debug/Release AND ARM64/x64 ***
#
# Use 'config' to switch between `Debug` and `Release`.
# Use 'arch' to switch between `ARM64` and `x64`.
#
# Note: `wperf-test` project require `wperf` project to be built first.
# Currently WindowsPerf solution defines this build dependency and you can
# just straight build `make ... wperf-test` and dependencies will build.
#
# Examples:
#
#     make config=Release all
#     make config=Debug all
#
#     make config=Release arch=x64 wperf-test
#     make config=Debug arch=x64 wperf-test
#
# *** BUILDING UNIT TESTS ***
#
# Note: You can currently build unit tests only on x64 host!
#
#     make test                                          (Default Debug/x64)
#     make config=Release test                           (Release/x64)
#
# or more verbose version of above:
#
#     make wperf-test wperf-test-run                     (Default Debug/x64)
#     make config=Release wperf-test wperf-test-run      (Release/x64)
#
# *** RELEASE BINARY PACKAGING ***
#
# Use `make config=Release release` to package `wperf` and `wperf-driver`.
#

# By default we build for ARM64 target. Define arch variable to change defalt
# value.
make_arch=ARM64

# By default we build with Debug configuration. Define config variable to change defalt
# value.
make_config=Debug

ifdef arch
	make_arch=$(arch)
endif

ifdef config
	make_config=$(config)
endif

#
# Building rules
#
all: wperf-test
	devenv windowsperf.sln /Rebuild "$(make_config)|$(make_arch)" 2>&1

wperf:
	devenv windowsperf.sln /Rebuild "$(make_config)|${make_arch}" /Project wperf\wperf.vcxproj 2>&1

wperf-driver:
	devenv windowsperf.sln /Rebuild "$(make_config)|$(make_arch)" /Project wperf-driver\wperf-driver.vcxproj 2>&1

wperf-test:
	devenv windowsperf.sln /Rebuild "$(make_config)|x64" /Project wperf-test\wperf-test.vcxproj 2>&1

wperf-test-run:
	vstest.console x64\$(make_config)\wperf-test.dll

test: wperf-test wperf-test-run

release:
	mkdir release
	tar zcf release/WinodwsPerf.tar.gz wperf/$(make_arch)/$(make_config)/wperf.exe wperf-driver/$(make_arch)/$(make_config)/wperf-driver
	tar acf release/WinodwsPerf.zip wperf/$(make_arch)/$(make_config)/wperf.exe wperf-driver/$(make_arch)/$(make_config)/wperf-driver
	tar jcf release/WinodwsPerf.tar.bz2 wperf/$(make_arch)/$(make_config)/wperf.exe wperf-driver/$(make_arch)/$(make_config)/wperf-driver
	tar Jcf release/WinodwsPerf.tar.xz wperf/$(make_arch)/$(make_config)/wperf.exe wperf-driver/$(make_arch)/$(make_config)/wperf-driver
	tar cf release/WinodwsPerf.tar wperf/$(make_arch)/$(make_config)/wperf.exe wperf-driver/$(make_arch)/$(make_config)/wperf-driver

clean:
	devenv windowsperf.sln /Clean "$(make_config)|$(make_arch)" 2>&1

purge:
	rm -rf wperf/ARM64 wperf/ARM64EC wperf/x64
	rm -rf wperf-driver/ARM64 wperf-driver/ARM64EC wperf-driver/x64
	rm -rf wperf-test/ARM64 wperf-test/x64
	rm -rf ARM64/ ARM64EC/ x64/

docs:
	doxygen

docs-clean:
	rm -rf docs/ html/ latex/
