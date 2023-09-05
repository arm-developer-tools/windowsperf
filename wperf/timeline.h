#pragma once
// BSD 3-Clause License
//
// Copyright (c) 2023, Arm Limited
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// 1. Redistributions of source code must retain the above copyright notice, this
//    list of conditions and the following disclaimer.
//
// 2. Redistributions in binary form must reproduce the above copyright notice,
//    this list of conditions and the following disclaimer in the documentation
//    and/or other materials provided with the distribution.
//
// 3. Neither the name of the copyright holder nor the names of its
//    contributors may be used to endorse or promote products derived from
//    this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
// FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
// DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
// SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
// CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
// OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#include <windows.h>
#include <fstream>
#include <map>
#include <string>
#include <vector>
#include "wperf-common/iorequest.h"


struct timeline_header
{
	bool multiplexing = false;
	bool include_kernel = false;
	double count_interval = 1.0;
	std::wstring vendor_name;
	std::wstring event_class;
	std::string filename;			// Name of the timeline file (we will also show it in the header)
};

/* Timeline file content
+------------------------------+
|                              |
|       timeline_headers       |
|                              |
+------------------------------+

+------------------------------+
|    timeline_header_cores     |
+------------------------------+
| timeline_header_event_names  | + timeline_header_metric_names
+------------------------------+
| timeline_header_event_values | + timeline_header_metric_values
+------------------------------+
*/

namespace timeline {

	extern std::map<enum evt_class, struct timeline_header> timeline_headers;
	extern std::map<enum evt_class, std::vector<std::wstring>> timeline_header_cores;
	extern std::map<enum evt_class, std::vector<std::wstring>> timeline_header_event_names;
	extern std::map<enum evt_class, std::vector<std::vector<std::wstring>>> timeline_header_event_values;	// [EVT_CLAS][line][event_values]

	extern std::map<enum evt_class, std::vector<std::wstring>> timeline_header_metric_names;
	extern std::map<enum evt_class, std::vector<std::vector<std::wstring>>> timeline_header_metric_values;

	void init();
	void print();
	void print_header(std::wofstream& timeline_outfile, const enum evt_class e_class);
}
