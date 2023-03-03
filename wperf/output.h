#pragma once
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

#include <variant>
#include "prettytable.h"
#include "json.h"

// Just a handy definition to get compile time Integer value.
template <int I>
struct Integer
{ const static int value = I; };

// Base class to all table outputs.
template <typename CharType>
struct TableOutputTraits
{
    inline const static std::tuple<> headers;
    inline const static CharType* key;
    inline const static int size = 0;
};

template <typename CharType>
struct PredefinedEventsOutputTraits : public TableOutputTraits<CharType>
{
    inline const static std::tuple<CharType *, CharType*, CharType*> headers = 
        std::make_tuple(LITERALCONSTANTS_GET("Alias Name"),
                        LITERALCONSTANTS_GET("Raw Index"),
                        LITERALCONSTANTS_GET("Event Type"));
    inline const static int size = std::tuple_size_v<decltype(headers)>;
    inline const static CharType* key = LITERALCONSTANTS_GET("Predefined Events");
};

template <typename CharType>
struct MetricOutputTraits : public TableOutputTraits<CharType>
{
    inline const static std::tuple<CharType *, CharType*> headers = 
        std::make_tuple(LITERALCONSTANTS_GET("Metric"),
                        LITERALCONSTANTS_GET("Events"));
    inline const static int size = std::tuple_size_v<decltype(headers)>;
    inline const static CharType* key = LITERALCONSTANTS_GET("Predefined Metrics");
};

template <typename CharType, bool isMultiplexed=true>
struct PerformanceCounterOutputTraits : public TableOutputTraits<CharType>
{
    inline const static int size = std::conditional_t<isMultiplexed, Integer<6>, Integer<4>>::value;
    inline const static std::tuple<CharType *, CharType*, CharType*, CharType*, CharType*, CharType*> headers =
        std::make_tuple(LITERALCONSTANTS_GET("counter value"),
                        LITERALCONSTANTS_GET("event name"),
                        LITERALCONSTANTS_GET("event idx"),
                        LITERALCONSTANTS_GET("event note"),
                        LITERALCONSTANTS_GET("multiplexed"),
                        LITERALCONSTANTS_GET("scaled value"));
    inline const static CharType* key = LITERALCONSTANTS_GET("Performance counter");
};

template <typename CharType, bool isMultiplexed=true>
struct SystemwidePerformanceCounterOutputTraits : public TableOutputTraits<CharType>
{
    inline const static int size = std::conditional_t<isMultiplexed, Integer<5>, Integer<4>>::value;
    inline const static std::tuple<CharType *, CharType*, CharType*, CharType*, CharType*> headers =
        std::make_tuple(LITERALCONSTANTS_GET("counter value"),
                        LITERALCONSTANTS_GET("event name"),
                        LITERALCONSTANTS_GET("event idx"),
                        LITERALCONSTANTS_GET("event note"),
                        LITERALCONSTANTS_GET("scaled value"));
    inline const static CharType* key = LITERALCONSTANTS_GET("Systemwide Overall Performance Counters");
};

template <typename CharType>
struct PMUPerformanceCounterOutputTraits : public TableOutputTraits<CharType>
{
    inline const static std::tuple<CharType *, CharType*, CharType*, CharType*, CharType*> headers =
        std::make_tuple(LITERALCONSTANTS_GET("pmu id"),
                        LITERALCONSTANTS_GET("counter value"),
                        LITERALCONSTANTS_GET("event name"),
                        LITERALCONSTANTS_GET("event idx"),
                        LITERALCONSTANTS_GET("event note"));
    inline const static int size = std::tuple_size_v<decltype(headers)>;
    inline const static CharType* key = LITERALCONSTANTS_GET("PMU Performance Counters");
};

template <typename CharType>
struct L3CacheMetricOutputTraits : public TableOutputTraits<CharType>
{
    inline const static std::tuple<CharType *, CharType*, CharType*, CharType*> headers = 
        std::make_tuple(LITERALCONSTANTS_GET("cluster"),
                        LITERALCONSTANTS_GET("cores"),
                        LITERALCONSTANTS_GET("read_bandwidth"),
                        LITERALCONSTANTS_GET("miss_rate"));
    inline const static int size = std::tuple_size_v<decltype(headers)>;
    inline const static CharType* key = LITERALCONSTANTS_GET("L3 Cache Metrics");
};

template <typename CharType>
struct DDRMetricOutputTraits : public TableOutputTraits<CharType>
{
    inline const static std::tuple<CharType *, CharType*> headers = 
        std::make_tuple(LITERALCONSTANTS_GET("channel"),
                        LITERALCONSTANTS_GET("rw_bandwidth"));
    inline const static int size = std::tuple_size_v<decltype(headers)>;
    inline const static CharType* key = LITERALCONSTANTS_GET("DDR Metrics");
};

template <typename CharType>
struct TestOutputTraits : public TableOutputTraits<CharType>
{
    inline const static std::tuple<CharType *, CharType*> headers = 
        std::make_tuple(LITERALCONSTANTS_GET("Test Name"),
                        LITERALCONSTANTS_GET("Result"));
    inline const static int size = std::tuple_size_v<decltype(headers)>;
    inline const static CharType* key = LITERALCONSTANTS_GET("Test Results");
};

template <typename CharType>
struct VersionOutputTraits : public TableOutputTraits<CharType>
{
    inline const static std::tuple<CharType*, CharType*> headers =
        std::make_tuple(LITERALCONSTANTS_GET("Component"),
                        LITERALCONSTANTS_GET("Version"));
    inline const static int size = std::tuple_size_v<decltype(headers)>;
    inline const static CharType* key = LITERALCONSTANTS_GET("Version");
};

// Generic TableOutput that handles instantiating and dispatching to the proper Table formating class.
template <typename CharType>
class TableOutput
{
public:
    enum TableType
    {
        JSON,
        PRETTY,
        ALL,
    };

private:
    typedef typename std::conditional_t<std::is_same_v<CharType, char>, std::ostream, std::wostream> OutputStream;
    typedef typename std::conditional_t<std::is_same_v<CharType, char>, std::string, std::wstring> StringType;
    typedef typename std::conditional_t<std::is_same_v<CharType, char>, std::stringstream, std::wstringstream> StringStream;

    TableType m_type;

    void copy(const TableOutput& rhs)
    {
        m_type = rhs.m_type;
        m_core = rhs.m_core;
        if(m_type == JSON || m_type == ALL)
        {
            TableJSON<CharType> obj = rhs.m_tableJSON;
            m_tableJSON = obj;
        }
    }
public:
    TableJSON<CharType> m_tableJSON;
    PrettyTable<CharType> m_tablePretty;

    StringType m_core;

    TableOutput(TableType type) : m_type(type)
    {
    }

    TableOutput() : TableOutput(JSON) {}

    TableOutput(const TableOutput& rhs)
    {
        copy(rhs);
    }

    TableOutput& operator=(const TableOutput<CharType>& rhs)
    {
        copy(rhs);
        return *this;
    }

    template <typename PresetTable, int I = 0>
    void PresetHeaders()
    {
        if constexpr(I < PresetTable::size)
        {
            AddColumn(std::get<I>(PresetTable::headers));
            PresetHeaders<PresetTable, I + 1>();
        } else {
            SetKey(PresetTable::key);
        }
    }

    void AddColumn(const StringType& header)
    {
        m_tableJSON.AddColumn(header);
        m_tablePretty.AddColumn(header);
    }

    template <typename... Ts>
    void Insert(Ts... args)
    {
        m_tableJSON.Insert(args...);
        m_tablePretty.Insert(args...);
    }

    void SetAlignment(int ColumnIndex, enum PrettyTable<CharType>::ColumnAlign Align)
    {
        m_tablePretty.SetAlignment(ColumnIndex, Align);
    }

    void SetKey(const StringType& key)
    {
        m_tableJSON.SetKey(key);
    }

    void InsertExtra(const StringType& key, const StringType& value)
    {
        m_tableJSON.InsertAdditional(key, value);
    }

    StringStream Print(TableType type = PRETTY)
    {
        StringStream strstream;
        switch(type)
        {
            case JSON: strstream << m_tableJSON; break;
            case PRETTY: strstream << m_tablePretty; break;
        }
        return strstream;
    }
};

template <typename CharType>
struct WPerfStatJSON
{
    typedef typename std::conditional_t<std::is_same_v<CharType, char>, std::ostream, std::wostream> OutputStream;
    typedef typename std::conditional_t<std::is_same_v<CharType, char>, std::stringstream, std::wstringstream> StringStream;

    std::vector<TableOutput<CharType>> m_corePerformanceTables;
    std::vector<TableOutput<CharType>> m_DSUPerformanceTables;
    TableOutput<CharType> m_coreOverall, m_DSUOverall, m_DMCDDDR, m_pmu, m_DSUL3metric;
    bool m_multiplexing;
    bool m_kernel;
    double m_duration;

    StringStream Print()
    {
        StringStream os;
        enum TableOutput<CharType>::TableType jsonType = TableOutput<CharType>::JSON;
        os << LiteralConstants<CharType>::m_cbracket_open << std::endl;
        {
            os << LITERALCONSTANTS_GET("\"core\": ") <<  LiteralConstants<CharType>::m_cbracket_open << std::endl;
            os << LITERALCONSTANTS_GET("\"Multiplexing\": ");
            if (m_multiplexing)
                os << LITERALCONSTANTS_GET("true");
            else
                os << LITERALCONSTANTS_GET("false");
            os << LiteralConstants<CharType>::m_comma << std::endl;
            os << LITERALCONSTANTS_GET("\"Kernel_mode\": ");
            if (m_kernel)
                os << LITERALCONSTANTS_GET("true");
            else
                os << LITERALCONSTANTS_GET("false");
            os << LiteralConstants<CharType>::m_comma << std::endl;
            bool isFirst = true;
            for(auto& table : m_corePerformanceTables)
            {
                if(!isFirst)
                {
                    os << LiteralConstants<CharType>::m_comma << std::endl;
                } else {
                    isFirst = false;
                }
                os << LiteralConstants<CharType>::m_quotes << table.m_core << LiteralConstants<CharType>::m_quotes;
                os << LiteralConstants<CharType>::m_colon << table.Print(jsonType).str() << std::endl;
            }
            if(!m_corePerformanceTables.empty())
                os << LiteralConstants<CharType>::m_comma << std::endl;
            os << LITERALCONSTANTS_GET("\"overall\": ") << m_coreOverall.Print(jsonType).str() << std::endl;
            os << LiteralConstants<CharType>::m_cbracket_close << std::endl;
        }
        os << LiteralConstants<CharType>::m_comma << std::endl;
        {
            os << LITERALCONSTANTS_GET("\"dsu\": ") <<  LiteralConstants<CharType>::m_cbracket_open << std::endl;
            bool isFirst = true;
            for(auto& table : m_DSUPerformanceTables)
            {
                if(!isFirst)
                {
                    os << LiteralConstants<CharType>::m_comma << std::endl;
                } else {
                    isFirst = false;
                }
                os << LiteralConstants<CharType>::m_quotes << table.m_core << LiteralConstants<CharType>::m_quotes;
                os << LiteralConstants<CharType>::m_colon << table.Print(jsonType).str() << std::endl;
            }
            if(!m_DSUPerformanceTables.empty())
                os << LiteralConstants<CharType>::m_comma << std::endl;
            os << LITERALCONSTANTS_GET("\"l3metric\": ") << m_DSUL3metric.Print(jsonType).str() << LiteralConstants<CharType>::m_comma << std::endl;
            os << LITERALCONSTANTS_GET("\"overall\": ") << m_DSUOverall.Print(jsonType).str() << std::endl;
            os << LiteralConstants<CharType>::m_cbracket_close << std::endl;
        }
        os << LiteralConstants<CharType>::m_comma << std::endl;
        {
            os << LITERALCONSTANTS_GET("\"dmc\": ") <<  LiteralConstants<CharType>::m_cbracket_open << std::endl;
            os << LITERALCONSTANTS_GET("\"pmu\": ") << m_pmu.Print(jsonType).str() << LiteralConstants<CharType>::m_comma << std::endl;
            os << LITERALCONSTANTS_GET("\"ddr\": ") << m_DMCDDDR.Print(jsonType).str() << std::endl;
            os << LiteralConstants<CharType>::m_cbracket_close << LiteralConstants<CharType>::m_comma << std::endl;
        }
        os << LITERALCONSTANTS_GET("\"Time_elapsed\": ") << m_duration << std::endl;
        os << LiteralConstants<CharType>::m_cbracket_close;
        return os;
    }
};

template <typename CharType>
struct WPerfListJSON
{
    typedef typename std::conditional_t<std::is_same_v<CharType, char>, std::ostream, std::wostream> OutputStream;
    typedef typename std::conditional_t<std::is_same_v<CharType, char>, std::stringstream, std::wstringstream> StringStream;

    TableOutput<CharType> m_Events, m_Metrics;

    StringStream Print()
    {
        StringStream os;
        enum TableOutput<CharType>::TableType jsonType = TableOutput<CharType>::JSON;
        m_Events.m_tableJSON.m_isEmbedded = true;
        m_Metrics.m_tableJSON.m_isEmbedded = true;
        os << LiteralConstants<CharType>::m_cbracket_open << std::endl;
        os << m_Events.Print(jsonType).str() << LiteralConstants<CharType>::m_comma << std::endl;
        os << m_Metrics.Print(jsonType).str() << std::endl;
        os << LiteralConstants<CharType>::m_cbracket_close;
        return os;
    }
};

// Class to control the output, it handles if the program is in quiet mode or not, which output to use (either wcout or cout)
// as well as outputing to a file in case the user requested it.
template <typename CharType>
struct OutputControl
{
	typedef typename std::conditional_t<std::is_same_v<CharType, char>, std::string, std::wstring> StringType;
	typedef typename std::conditional_t<std::is_same_v<CharType, char>, std::ostream, std::wostream> OutputStream;
	typedef typename std::conditional_t<std::is_same_v<CharType, char>, std::stringstream, std::wstringstream> StringStream;
    typedef typename std::conditional_t<std::is_same_v<CharType, char>, std::ofstream, std::wofstream> FileStream;

    bool m_isQuiet = false;
    bool m_shouldWriteToFile = false;

    StringType m_filename;

    OutputStream& GetOutputStream()
    {
        if constexpr(std::is_same_v<CharType, char>)
        {
            if(m_isQuiet)
            {
                std::cout.setstate(std::ios::failbit);
            } else {
                std::cout.clear();
            }
            return std::cout;
        } else {
            if(m_isQuiet)
            {
                std::wcout.setstate(std::ios::failbit);
            } else {
                std::wcout.clear();
            }
            return std::wcout;
        }
    }

    OutputStream& GetErrorOutputStream()
    {
        if constexpr(std::is_same_v<CharType, char>)
        {
            return std::cerr;
        } else {
            return std::wcerr;
        }
    }

    void Print_(StringType& str)
    {
        bool curIsQuiet = m_isQuiet;
        //If the user requested to be quiet but to output JSON results we disable quiet mode just
        //tou output the JSON tables and then reenable it afterwards.
        if((m_outputType == TableOutput<CharType>::JSON || m_outputType == TableOutput<CharType>::ALL) && m_isQuiet)
        {
            m_isQuiet = false;
        }

        GetOutputStream() << str;
        
        if((m_outputType == TableOutput<CharType>::JSON || m_outputType == TableOutput<CharType>::ALL) && curIsQuiet)
        {
            m_isQuiet = true;
        }
    }

    void OutputToFile(StringType& str)
    {
        FileStream file;
        file.open(m_filename, std::fstream::out | std::fstream::trunc);
        if(file.is_open())
        {
            file << str;
            file.close();
        } else {
            GetErrorOutputStream() << LITERALCONSTANTS_GET("Unable to open ") << m_filename << std::endl;
        }
    }

    void Print(TableOutput<CharType>& table, bool printJson = false)
    {
        StringType s = table.Print(TableOutput<CharType>::PRETTY).str();
        GetOutputStream() << s;
        if((m_outputType == TableOutput<CharType>::JSON || m_outputType == TableOutput<CharType>::ALL) && printJson)
        {
            StringType sj = table.Print(TableOutput<CharType>::JSON).str();
            if(!m_shouldWriteToFile)
            {
                Print_(sj);
            } else {
                OutputToFile(sj);
            }
        }
    }

    void Print(WPerfStatJSON<CharType>& table)
    {
        if(m_outputType == TableOutput<CharType>::JSON || m_outputType == TableOutput<CharType>::ALL)
        {
            StringType s = table.Print().str();
            if(!m_shouldWriteToFile)
            {
                Print_(s);
            } else {
                OutputToFile(s);
            }
        }
    }

    void Print(WPerfListJSON<CharType>& table)
    {
        if(m_outputType == TableOutput<CharType>::JSON || m_outputType == TableOutput<CharType>::ALL)
        {
            StringType s = table.Print().str();
            if(!m_shouldWriteToFile)
            {
                Print_(s);
            } else {
                OutputToFile(s);
            }
        }
    }
};

// Handy namespace to facilitate gathering all output for WPerf along with global char type definitions and quick aliases. The L at the 
// end of each alias stands for local.
namespace WPerfOutput
{
    using GlobalCharType = wchar_t;
    using GlobalStringType = std::basic_string<GlobalCharType>;
    using TableOutputL = TableOutput<GlobalCharType>;
    using OutputControlL = OutputControl<GlobalCharType>;
    using ColumnAlignL = enum PrettyTable<GlobalCharType>::ColumnAlign;
    using PredefinedEventsOutputTraitsL = PredefinedEventsOutputTraits<GlobalCharType>;
    
    template <bool isMultiplexing>
    using PerformanceCounterOutputTraitsL = PerformanceCounterOutputTraits<GlobalCharType, isMultiplexing>;

    template <bool isMultiplexing>
    using SystemwidePerformanceCounterOutputTraitsL = SystemwidePerformanceCounterOutputTraits<GlobalCharType, isMultiplexing>;

    using L3CacheMetricOutputTraitsL = L3CacheMetricOutputTraits<GlobalCharType>;
    using PMUPerformanceCounterOutputTraitsL = PMUPerformanceCounterOutputTraits<GlobalCharType>;
    using DDRMetricOutputTraitsL = DDRMetricOutputTraits<GlobalCharType>;
    using TestOutputTraitsL = TestOutputTraits<GlobalCharType>;
    using MetricOutputTraitsL = MetricOutputTraits<GlobalCharType>;
    using VersionOutputTraitsL = VersionOutputTraits<GlobalCharType>;
    
    OutputControlL m_out;
    WPerfStatJSON<GlobalCharType> m_globalJSON;
    WPerfListJSON<GlobalCharType> m_globalListJSON;

    TableOutputL::TableType m_outputType = TableOutputL::PRETTY;
};