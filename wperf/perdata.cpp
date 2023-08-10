// BSD 3-Clause License
//
// Copyright (c) 2022, Arm Limited
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

#include "perfdata.h"
#include <fstream>

#define GET_64ALIGNED_SIZE(X) static_cast<UINT16>((std::ceil(static_cast<double>(X) / sizeof(UINT64)))*sizeof(UINT64))

void PerfDataWriter::WriteCommandLine(const int argc, const wchar_t* argv[])
{
	m_cmdline.count = argc;
	m_cmdline.data = new perfdata::perf_data_string[argc];

	for (auto i = 0; i < argc; i++)
	{
		std::string str = MultiByteFromWideString(argv[i]);
		str += '\0';
		m_cmdline_size += sizeof(UINT32) + sizeof(char) * str.length();
		m_cmdline.data[i].size = static_cast<UINT32>(str.length());
		m_cmdline.data[i].data = new char[m_cmdline.data[i].size];
		memcpy(m_cmdline.data[i].data, str.c_str(), sizeof(char) * str.length());
	}
	m_feature_count++;
	m_has_cmdline = true;
	set_feature(perfdata::HEADER_CMDLINE);
}

size_t PerfDataWriter::WriteAttributeSection(size_t data_offset)
{
	size_t written = 0;
	m_file.seekp(data_offset);
	for (auto& attr : m_attributes)
	{
		size_t size_to_write = sizeof(perfdata::perf_file_attr);
		m_file.write(reinterpret_cast<char*>(&attr), size_to_write);
		written += size_to_write;
	}
	return written;
}

size_t PerfDataWriter::WriteFeatures(size_t file_section_offset, size_t data_offset)
{
	size_t written = 0;
	if (m_has_cmdline)
	{
		perfdata::perf_file_section section{0};
		section.offset = data_offset;
		section.size = m_cmdline_size + sizeof(UINT32); // Data + count

		m_file.seekp(file_section_offset);
		m_file.write(reinterpret_cast<char*>(&section), sizeof(perfdata::perf_file_section));
		written += sizeof(perfdata::perf_file_section);

		m_file.seekp(data_offset);
		m_file.write(reinterpret_cast<char*>(&m_cmdline.count), sizeof(UINT32));
		written += sizeof(UINT32);

		for (UINT32 i = 0; i < m_cmdline.count; i++)
		{
			size_t size_to_write = sizeof(char) * m_cmdline.data[i].size;
			m_file.write(reinterpret_cast<char*>(&m_cmdline.data[i].size), sizeof(UINT32));
			written += sizeof(UINT32);
			m_file.write(m_cmdline.data[i].data, size_to_write);
			written += size_to_write;
		}
	}
	return written;
}

size_t PerfDataWriter::WriteDataSection(size_t offset)
{
	m_file.seekp(offset);
	size_t written = 0;
	for (auto& el : m_events)
	{
		std::visit([&](auto&& arg)
		{
			m_file.write(reinterpret_cast<char*>(&arg), arg.header.size);
			written += arg.header.size;
		}, el);
	}
	return written;
}

PerfDataWriter::PerfEvent PerfDataWriter::get_comm_event(DWORD pid, std::wstring& command)
{
	PerfDataWriter::PerfEvent var;
	perfdata::perf_data_comm_event event { 0 };
	event.header.size = sizeof(event);
	event.header.type = perfdata::PERF_RECORD_COMM;
	event.header.misc = PERF_RECORD_MISC_USER;
	event.pid = pid;
	event.tid = pid;

	std::string token = MultiByteFromWideString(command.c_str());
	memcpy(event.comm, token.c_str(), sizeof(char) * min(token.length(), 16));

	var = event;
	return var;
}

PerfDataWriter::PerfEvent PerfDataWriter::get_sample_event(DWORD pid, UINT64 ip, UINT32 cpu)
{
	PerfDataWriter::PerfEvent var;
	perfdata::perf_data_sample_event event {0};
	event.header.size = sizeof(event);
	event.header.type = perfdata::PERF_RECORD_SAMPLE;
	event.header.misc = PERF_RECORD_MISC_USER;
	event.pid = pid;
	event.tid = pid;
	event.cpu = cpu;
	event.ip = ip;

	var = event;
	return var;
}

PerfDataWriter::PerfEvent PerfDataWriter::get_mmap_event(DWORD pid, UINT64 addr, UINT64 len, std::wstring& filename, UINT64 pgoff)
{
	PerfDataWriter::PerfEvent var;
	perfdata::perf_data_mmap_event event{ 0 };

	std::string token = MultiByteFromWideString(filename.c_str());

	event.header.size = sizeof(event) - sizeof(event.filename) + static_cast<UINT16>(GET_64ALIGNED_SIZE(token.size()+1));
	event.header.type = perfdata::PERF_RECORD_MMAP;
	event.header.misc = PERF_RECORD_MISC_USER;
	event.pid = pid;
	event.tid = pid;
	event.start = addr;
	event.len = len;
	event.pgoff = pgoff;

	memset(event.filename, 0, sizeof(char) * PATH_MAX);
	memcpy(event.filename, token.c_str(), sizeof(char) * min(token.size()+1, PATH_MAX));
	
	var = event;
	return var;
}

void PerfDataWriter::Write(std::string filename)
{
	size_t offset = 0;
	m_file.open(filename.c_str(), std::ios::binary);

	m_file_header.attrs.size = sizeof(perfdata::perf_file_attr) * m_attributes.size();

	offset += sizeof(perfdata::perf_file_header);
	offset += WriteAttributeSection(offset);
	m_file_header.data.offset = offset;
	m_file_header.data.size = WriteDataSection(offset);
	offset += m_file_header.data.size;

	offset += WriteFeatures(offset, offset + get_features_section_size());

	m_file.seekp(0);
	m_file.write(reinterpret_cast<char*>(&m_file_header), sizeof(perfdata::perf_file_header));
	m_file.close();
}