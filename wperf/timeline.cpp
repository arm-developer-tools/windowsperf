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

#include "timeline.h"
#include "utils.h"


namespace timeline {

	std::map<enum evt_class, struct timeline_header> timeline_headers;
    std::map<enum evt_class, std::vector<std::wstring>> timeline_header_cores;

    std::map<enum evt_class, std::vector<std::wstring>> timeline_header_event_names;
    std::map<enum evt_class, std::vector<std::vector<std::wstring>>> timeline_header_event_values;

    std::map<enum evt_class, std::vector<std::wstring>> timeline_header_metric_names;
    std::map<enum evt_class, std::vector<std::vector<std::wstring>>> timeline_header_metric_values;

	void init() {
		timeline_headers.clear();
		timeline_header_cores.clear();
		timeline_header_event_names.clear();
        timeline_header_event_values.clear();
        timeline_header_metric_names.clear();
        timeline_header_metric_values.clear();
	}

    void print()
    {
        for (auto& [e_class, header] : timeline_headers)
        {
            const auto& filename = timeline_headers[e_class].filename;

            std::wofstream timeline_outfile;
            timeline_outfile.open(filename);

            print_header(timeline_outfile, e_class);

            timeline_outfile << std::endl;

            for (const auto& core : timeline_header_cores[e_class])
                timeline_outfile << core << L",";

            timeline_outfile << std::endl;

            for (const auto& event_name : timeline_header_event_names[e_class])
                timeline_outfile << event_name << L",";

            // Print metric names at the end of event count values line
            for (const auto& metric_name : timeline_header_metric_names[e_class])
                timeline_outfile << L"M@" << metric_name << L",";

            timeline_outfile << std::endl;

            int lineno = 0;
            for (const auto& lines : timeline_header_event_values[e_class])
            {
                for (const auto& event_value : lines)
                    timeline_outfile << event_value << L",";

                // Print metric values at the end of event count values line
                if (timeline_header_metric_values.count(e_class))
                    for (const auto& metric_value : timeline_header_metric_values[e_class][lineno])
                        timeline_outfile << metric_value << L",";

                timeline_outfile << std::endl;
                lineno++;
            }

            timeline_outfile << std::endl;

            timeline_outfile.close();
        }
    }

	void print_header(std::wofstream& timeline_outfile, const enum evt_class e_class)
	{
        timeline_outfile << L"Multiplexing,";
        timeline_outfile << (timeline_headers[e_class].multiplexing ? L"TRUE" : L"FALSE");
        timeline_outfile << std::endl;

        if (e_class == EVT_CORE)
        {
            timeline_outfile << L"Kernel mode,";
            timeline_outfile << (timeline_headers[e_class].include_kernel ? L"TRUE" : L"FALSE");
            timeline_outfile << std::endl;
        }

        timeline_outfile << L"Count interval,";
        timeline_outfile << DoubleToWideString(timeline_headers[e_class].count_interval);
        timeline_outfile << std::endl;

        timeline_outfile << L"Vendor," << timeline_headers[e_class].vendor_name;
        timeline_outfile << std::endl;

        timeline_outfile << L"Event class," << timeline_headers[e_class].event_class;
        timeline_outfile << std::endl;
	}
}