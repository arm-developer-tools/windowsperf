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

.PHONY: all clean docs wperf wperf-driver

#
# By default we vuild for ARM64 target. Define arch variable to change defalt
# value.
#
# 	> make arch=x64 all
#
make_arch=ARM64

ifdef arch
	make_arch=$(arch)
endif

#
# Building rules
#
all:
	devenv windowsperf.sln /Rebuild "Debug|$(make_arch)" 2>&1

wperf:
	devenv windowsperf.sln /Rebuild "Debug|${make_arch}" /Project wperf\wperf.vcxproj 2>&1

wperf-driver:
	devenv windowsperf.sln /Rebuild "Debug|$(make_arch)" /Project wperf-driver\wperf-driver.vcxproj 2>&1

clean:
	devenv windowsperf.sln /Clean "Debug|$(make_arch)" 2>&1

purge:
	rm -rf wperf/ARM64 wperf/x64 wperf/Debug
	rm -rf wperf-driver/ARM64 wperf-driver/x64 wperf-driver/Debug

docs:
	doxygen

docs-clean:
	rm -rf docs/ html/ latex/
