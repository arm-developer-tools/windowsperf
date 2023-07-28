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
{
    const static int value = I;
};

// Base class to all table outputs.
template <typename CharType>
struct TableOutputTraits
{
    inline const static std::tuple<> headers;
    inline const static std::tuple<> columns;
    inline const static CharType* key;
    inline const static int size = 0;
};

template <typename CharType, bool isVerbose = false >
struct PredefinedEventsOutputTraits : public TableOutputTraits<CharType>
{
    typedef typename std::conditional_t<std::is_same_v<CharType, char>, std::string, std::wstring> StringType;
    inline const static std::tuple<StringType, StringType, StringType, StringType> columns;
    inline const static std::tuple<CharType*, CharType*, CharType*, CharType*> headers =
        std::make_tuple(LITERALCONSTANTS_GET("Alias Name"),
            LITERALCONSTANTS_GET("Raw Index"),
            LITERALCONSTANTS_GET("Event Type"),
            LITERALCONSTANTS_GET("Description"));
    inline const static int size = std::conditional_t<isVerbose, Integer<4>, Integer<3>>::value;
    inline const static CharType* key = LITERALCONSTANTS_GET("Predefined Events");
};

template <typename CharType, bool isVerbose = false >
struct MetricOutputTraits : public TableOutputTraits<CharType>
{
    typedef typename std::conditional_t<std::is_same_v<CharType, char>, std::string, std::wstring> StringType;
    inline const static std::tuple<StringType, StringType, StringType, StringType, StringType> columns;
    inline const static int size = std::conditional_t<isVerbose, Integer<5>, Integer<2>>::value;
    inline const static std::tuple<CharType*, CharType*, CharType*, CharType*, CharType*> headers =
        std::make_tuple(LITERALCONSTANTS_GET("Metric"),
            LITERALCONSTANTS_GET("Events"),
            LITERALCONSTANTS_GET("Formula"),
            LITERALCONSTANTS_GET("Unit"),
            LITERALCONSTANTS_GET("Description"));
    inline const static CharType* key = LITERALCONSTANTS_GET("Predefined Metrics");
};

template <typename CharType, bool isMultiplexed = true>
struct PerformanceCounterOutputTraits : public TableOutputTraits<CharType>
{
    typedef typename std::conditional_t<std::is_same_v<CharType, char>, std::string, std::wstring> StringType;
    inline const static std::tuple<uint64_t, StringType, StringType, StringType, StringType, uint64_t> columns;
    inline const static int size = std::conditional_t<isMultiplexed, Integer<6>, Integer<4>>::value;
    inline const static std::tuple<CharType*, CharType*, CharType*, CharType*, CharType*, CharType*> headers =
        std::make_tuple(LITERALCONSTANTS_GET("counter value"),
            LITERALCONSTANTS_GET("event name"),
            LITERALCONSTANTS_GET("event idx"),
            LITERALCONSTANTS_GET("event note"),
            LITERALCONSTANTS_GET("multiplexed"),
            LITERALCONSTANTS_GET("scaled value"));
    inline const static CharType* key = LITERALCONSTANTS_GET("Performance counter");
};

template <typename CharType, bool isMultiplexed = true>
struct SystemwidePerformanceCounterOutputTraits : public TableOutputTraits<CharType>
{
    typedef typename std::conditional_t<std::is_same_v<CharType, char>, std::string, std::wstring> StringType;
    inline const static std::tuple<uint64_t, StringType, StringType, StringType, uint64_t> columns;
    inline const static int size = std::conditional_t<isMultiplexed, Integer<5>, Integer<4>>::value;
    inline const static std::tuple<CharType*, CharType*, CharType*, CharType*, CharType*> headers =
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
    typedef typename std::conditional_t<std::is_same_v<CharType, char>, std::string, std::wstring> StringType;
    inline const static std::tuple<StringType, uint64_t, StringType, StringType, StringType> columns;
    inline const static std::tuple<CharType*, CharType*, CharType*, CharType*, CharType*> headers =
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
    typedef typename std::conditional_t<std::is_same_v<CharType, char>, std::string, std::wstring> StringType;
    inline const static std::tuple<StringType, StringType, StringType, StringType> columns;
    inline const static std::tuple<CharType*, CharType*, CharType*, CharType*> headers =
        std::make_tuple(LITERALCONSTANTS_GET("cluster"),
            LITERALCONSTANTS_GET("cores"),
            LITERALCONSTANTS_GET("read_bandwidth"),
            LITERALCONSTANTS_GET("miss_rate"));
    inline const static int size = std::tuple_size_v<decltype(headers)>;
    inline const static CharType* key = LITERALCONSTANTS_GET("L3 Cache Metrics");
};

template <typename CharType>
struct TelemetrySolutionMetricOutputTraits : public TableOutputTraits<CharType>
{
    typedef typename std::conditional_t<std::is_same_v<CharType, char>, std::string, std::wstring> StringType;
    inline const static std::tuple<StringType, StringType, StringType, StringType, StringType> columns;
    inline const static std::tuple<CharType*, CharType*, CharType*, CharType*, CharType*> headers =
        std::make_tuple(LITERALCONSTANTS_GET("core"),
            LITERALCONSTANTS_GET("product_name"),
            LITERALCONSTANTS_GET("metric_name"),
            LITERALCONSTANTS_GET("value"),
            LITERALCONSTANTS_GET("unit"));
    inline const static int size = std::tuple_size_v<decltype(headers)>;
    inline const static CharType* key = LITERALCONSTANTS_GET("Telemetry Solution Metrics");
};


template <typename CharType>
struct DDRMetricOutputTraits : public TableOutputTraits<CharType>
{
    typedef typename std::conditional_t<std::is_same_v<CharType, char>, std::string, std::wstring> StringType;
    inline const static std::tuple<StringType, StringType> columns;
    inline const static std::tuple<CharType*, CharType*> headers =
        std::make_tuple(LITERALCONSTANTS_GET("channel"),
            LITERALCONSTANTS_GET("rw_bandwidth"));
    inline const static int size = std::tuple_size_v<decltype(headers)>;
    inline const static CharType* key = LITERALCONSTANTS_GET("DDR Metrics");
};

template <typename CharType>
struct TestOutputTraits : public TableOutputTraits<CharType>
{
    typedef typename std::conditional_t<std::is_same_v<CharType, char>, std::string, std::wstring> StringType;
    inline const static std::tuple<StringType, StringType> columns;
    inline const static std::tuple<CharType*, CharType*> headers =
        std::make_tuple(LITERALCONSTANTS_GET("Test Name"),
            LITERALCONSTANTS_GET("Result"));
    inline const static int size = std::tuple_size_v<decltype(headers)>;
    inline const static CharType* key = LITERALCONSTANTS_GET("Test Results");
};

template <typename CharType>
struct VersionOutputTraits : public TableOutputTraits<CharType>
{
    typedef typename std::conditional_t<std::is_same_v<CharType, char>, std::string, std::wstring> StringType;
    inline const static std::tuple<StringType, StringType, StringType> columns;
    inline const static std::tuple<CharType*, CharType*, CharType*> headers =
        std::make_tuple(LITERALCONSTANTS_GET("Component"),
            LITERALCONSTANTS_GET("Version"),
            LITERALCONSTANTS_GET("GitVer"));
    inline const static int size = std::tuple_size_v<decltype(headers)>;
    inline const static CharType* key = LITERALCONSTANTS_GET("Version");
};

template <typename CharType>
struct SamplingOutputTraits : public TableOutputTraits<CharType>
{
    typedef typename std::conditional_t<std::is_same_v<CharType, char>, std::string, std::wstring> StringType;
    inline const static std::tuple<double, uint32_t, StringType> columns;
    inline const static std::tuple<CharType*, CharType*, CharType*> headers =
        std::make_tuple(LITERALCONSTANTS_GET("overhead"),
            LITERALCONSTANTS_GET("count"),
            LITERALCONSTANTS_GET("symbol"));
    inline const static int size = std::tuple_size_v<decltype(headers)>;
    inline const static CharType* key = LITERALCONSTANTS_GET("samples");
};

template <typename CharType>
struct SamplingAnnotateOutputTraits : public TableOutputTraits<CharType>
{
    typedef typename std::conditional_t<std::is_same_v<CharType, char>, std::string, std::wstring> StringType;
    inline const static std::tuple<StringType, uint64_t, uint64_t> columns;
    inline const static std::tuple<CharType*, CharType*, CharType*> headers =
        std::make_tuple(LITERALCONSTANTS_GET("filename"),
            LITERALCONSTANTS_GET("line_number"),
            LITERALCONSTANTS_GET("hits"));
    inline const static int size = std::tuple_size_v<decltype(headers)>;
    inline const static CharType* key = LITERALCONSTANTS_GET("source_code");
};

enum TableType
{
    JSON,
    PRETTY,
    ALL,
};

// Generic TableOutput that handles instantiating and dispatching to the proper Table formating class.
template <typename PresetTable, typename CharType>
class TableOutput
{
private:
    typedef typename std::conditional_t<std::is_same_v<CharType, char>, std::ostream, std::wostream> OutputStream;
    typedef typename std::conditional_t<std::is_same_v<CharType, char>, std::string, std::wstring> StringType;
    typedef typename std::conditional_t<std::is_same_v<CharType, char>, std::stringstream, std::wstringstream> StringStream;

    TableType m_type;

    void copy(const TableOutput<PresetTable, CharType>& rhs)
    {
        m_type = rhs.m_type;
        m_core = rhs.m_core;
        m_event = rhs.m_event;
        if (m_type == JSON || m_type == ALL)
        {
            TableJSON<PresetTable, CharType> obj = rhs.m_tableJSON;
            m_tableJSON = obj;
        }
    }
public:
    TableJSON<PresetTable, CharType> m_tableJSON;
    PrettyTable<CharType> m_tablePretty;

    StringType m_core;
    StringType m_event;

    TableOutput(TableType type) : m_type(type)
    {
    }

    TableOutput() : TableOutput(JSON) {}

    TableOutput(const TableOutput& rhs)
    {
        copy(rhs);
    }

    TableOutput<PresetTable, CharType>& operator=(const TableOutput<PresetTable, CharType>& rhs)
    {
        copy(rhs);
        return *this;
    }

    template <int I = 0>
    void PresetHeaders()
    {
        if constexpr (I < PresetTable::size)
        {
            AddColumn(std::get<I>(PresetTable::headers));
            PresetHeaders<I + 1>();
        }
        else {
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

    template <typename T>
    void InsertExtra(const StringType& key, const T& value)
    {
        m_tableJSON.InsertAdditional<T>(key, value);
    }

    StringStream Print(TableType type = PRETTY)
    {
        StringStream strstream;
        switch (type)
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
    using PerformanceCounterOutputTraitsTO = TableOutput<PerformanceCounterOutputTraits<CharType, false>, CharType>;
    using MultiplexingPerformanceCounterOutputTraitsTO = TableOutput<PerformanceCounterOutputTraits<CharType, true>, CharType>;

    using SystemwidePerformanceCounterOutputTraitsTO = TableOutput<SystemwidePerformanceCounterOutputTraits<CharType, false>, CharType>;
    using MultiplexedSystemwidePerformanceCounterOutputTraitsTO = TableOutput<SystemwidePerformanceCounterOutputTraits<CharType, true>, CharType>;

    typedef typename std::conditional_t<std::is_same_v<CharType, char>, std::ostream, std::wostream> OutputStream;
    typedef typename std::conditional_t<std::is_same_v<CharType, char>, std::stringstream, std::wstringstream> StringStream;

    std::vector<std::variant<PerformanceCounterOutputTraitsTO, MultiplexingPerformanceCounterOutputTraitsTO>> m_corePerformanceTables;
    std::vector<std::variant<PerformanceCounterOutputTraitsTO, MultiplexingPerformanceCounterOutputTraitsTO>> m_DSUPerformanceTables;
    std::variant<SystemwidePerformanceCounterOutputTraitsTO, MultiplexedSystemwidePerformanceCounterOutputTraitsTO> m_coreOverall, m_DSUOverall;
    TableOutput<L3CacheMetricOutputTraits<CharType>, CharType> m_DSUL3metric;
    TableOutput<TelemetrySolutionMetricOutputTraits<CharType>, CharType> m_TSmetric;
    TableOutput<PMUPerformanceCounterOutputTraits<CharType>, CharType> m_pmu;
    TableOutput<DDRMetricOutputTraits<CharType>, CharType> m_DMCDDDR;
    bool m_multiplexing = false;
    bool m_kernel = false;
    double m_duration = 0.f;

    StringStream Print()
    {
        StringStream os;
        enum TableType jsonType = TableType::JSON;
        os << LiteralConstants<CharType>::m_cbracket_open << std::endl;
        {
            os << LITERALCONSTANTS_GET("\"core\": ") << LiteralConstants<CharType>::m_cbracket_open << std::endl;
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
            for (auto& table : m_corePerformanceTables)
            {
                if (!isFirst)
                {
                    os << LiteralConstants<CharType>::m_comma << std::endl;
                }
                else {
                    isFirst = false;
                }
                std::visit([&os, &jsonType](auto&& arg) {
                    os << LiteralConstants<CharType>::m_quotes << arg.m_core << LiteralConstants<CharType>::m_quotes;
                    os << LiteralConstants<CharType>::m_colon << arg.Print(jsonType).str() << std::endl;
                }, table);
            }
            if (!m_corePerformanceTables.empty())
                os << LiteralConstants<CharType>::m_comma << std::endl;

            std::visit([&os, &jsonType](auto&& arg) { os << LITERALCONSTANTS_GET("\"overall\": ") << arg.Print(jsonType).str() << std::endl; }, m_coreOverall);
            os << LiteralConstants<CharType>::m_comma << std::endl;
            os << LITERALCONSTANTS_GET("\"ts_metric\": ") << m_TSmetric.Print(jsonType).str() << std::endl;
            os << LiteralConstants<CharType>::m_cbracket_close << std::endl;
        }
        os << LiteralConstants<CharType>::m_comma << std::endl;
        {
            os << LITERALCONSTANTS_GET("\"dsu\": ") << LiteralConstants<CharType>::m_cbracket_open << std::endl;
            bool isFirst = true;
            for (auto& table : m_DSUPerformanceTables)
            {
                if (!isFirst)
                {
                    os << LiteralConstants<CharType>::m_comma << std::endl;
                }
                else {
                    isFirst = false;
                }
                std::visit([&os, &jsonType](auto&& arg) {
                    os << LiteralConstants<CharType>::m_quotes << arg.m_core << LiteralConstants<CharType>::m_quotes;
                    os << LiteralConstants<CharType>::m_colon << arg.Print(jsonType).str() << std::endl;
                }, table);
            }
            if (!m_DSUPerformanceTables.empty())
                os << LiteralConstants<CharType>::m_comma << std::endl;

            os << LITERALCONSTANTS_GET("\"l3metric\": ") << m_DSUL3metric.Print(jsonType).str() << LiteralConstants<CharType>::m_comma << std::endl;
            std::visit([&os, &jsonType](auto&& arg) { os << LITERALCONSTANTS_GET("\"overall\": ") << arg.Print(jsonType).str() << std::endl; }, m_DSUOverall);
            os << LiteralConstants<CharType>::m_cbracket_close << std::endl;
        }
        os << LiteralConstants<CharType>::m_comma << std::endl;
        {
            os << LITERALCONSTANTS_GET("\"dmc\": ") << LiteralConstants<CharType>::m_cbracket_open << std::endl;
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

    using PredefinedEventsOutputTraitsTO = TableOutput<PredefinedEventsOutputTraits<CharType, false>, CharType>;
    using VerbosePredefinedEventsOutputTraitsTO = TableOutput<PredefinedEventsOutputTraits<CharType, true>, CharType>;

    using MetricOutputTraitsTO = TableOutput<MetricOutputTraits<CharType, false>, CharType>;
    using VerboseMetricOutputTraitsTO = TableOutput<MetricOutputTraits<CharType, true>, CharType>;

    std::variant<PredefinedEventsOutputTraitsTO, VerbosePredefinedEventsOutputTraitsTO> m_Events;
    std::variant<MetricOutputTraitsTO, VerboseMetricOutputTraitsTO> m_Metrics;
    bool isVerbose = false;

    StringStream Print()
    {
        StringStream os;
        enum TableType jsonType = TableType::JSON;

        std::visit([](auto&& arg) {
            arg.m_tableJSON.m_isEmbedded = true;
            }, m_Events);

        std::visit([](auto&& arg) {
            arg.m_tableJSON.m_isEmbedded = true;
            }, m_Metrics);
        
        os << LiteralConstants<CharType>::m_cbracket_open << std::endl;

        std::visit([&os, &jsonType](auto&& arg) {
            os << arg.Print(jsonType).str() << LiteralConstants<CharType>::m_comma << std::endl;
            }, m_Events);

        std::visit([&os, &jsonType](auto&& arg) {
            os << arg.Print(jsonType).str() << std::endl;
            }, m_Metrics);

        os << LiteralConstants<CharType>::m_cbracket_close;
        return os;
    }
};

template <typename CharType>
struct WPerfSamplingJSON
{
    typedef typename std::conditional_t<std::is_same_v<CharType, char>, std::ostream, std::wostream> OutputStream;
    typedef typename std::conditional_t<std::is_same_v<CharType, char>, std::stringstream, std::wstringstream> StringStream;
    typedef typename std::conditional_t<std::is_same_v<CharType, char>, std::string, std::wstring> StringType;

    using Samples = TableOutput<SamplingOutputTraits<CharType>, CharType>;
    using AnnotateVector = std::vector<std::pair<StringType,TableOutput<SamplingAnnotateOutputTraits<CharType>, CharType>>>;

    std::map<StringType, std::pair<Samples, AnnotateVector>> m_map;
    
    uint32_t m_sample_display_row = 0;
    uint64_t m_samples_generated = 0;
    uint64_t m_samples_dropped = 0;

    StringStream Print()
    {
        StringStream os{};
        enum TableType jsonType = TableType::JSON;
        os << LiteralConstants<CharType>::m_cbracket_open << std::endl;
        {
            os << LITERALCONSTANTS_GET("\"sampling\": ") << LiteralConstants<CharType>::m_cbracket_open << std::endl;
            os << LITERALCONSTANTS_GET("\"sample_display_row\": ") << m_sample_display_row;
            os << LiteralConstants<CharType>::m_comma << std::endl;
            os << LITERALCONSTANTS_GET("\"samples_generated\": ") << m_samples_generated;
            os << LiteralConstants<CharType>::m_comma << std::endl;
            os << LITERALCONSTANTS_GET("\"samples_dropped\": ") << m_samples_dropped;
            os << LiteralConstants<CharType>::m_comma << std::endl;
            bool isFirst = true;
            for (auto& [key,value] : m_map)
            {
                if (!isFirst)
                {
                    os << LiteralConstants<CharType>::m_comma << std::endl;
                }
                else {
                    isFirst = false;
                }

                std::get<0>(value).m_tableJSON.m_isEmbedded = true;

                os << LiteralConstants<CharType>::m_quotes << key << LiteralConstants<CharType>::m_quotes;
                os << LiteralConstants<CharType>::m_colon;
                os << LiteralConstants<CharType>::m_cbracket_open;
                os << std::get<0>(value).Print(jsonType).str();
                os << LiteralConstants<CharType>::m_comma << std::endl;
                os << LITERALCONSTANTS_GET("\"annotate\": ");    
                os << LiteralConstants<CharType>::m_bracket_open;
                bool isFirstInside = true;
                for(auto &[function_name, table] : std::get<1>(value))
                {
                    table.m_tableJSON.m_isEmbedded = true;
                    if(!isFirstInside)
                    {
                        os << LiteralConstants<CharType>::m_comma << std::endl;
                    }
                    else {
                        isFirstInside = false;
                    }
                    os << LiteralConstants<CharType>::m_cbracket_open;
                    os << LITERALCONSTANTS_GET("\"function_name\": ");
                    os << LiteralConstants<CharType>::m_quotes << function_name << LiteralConstants<CharType>::m_quotes;
                    os << LiteralConstants<CharType>::m_comma << std::endl;
                    os << table.Print(jsonType).str();
                    os << LiteralConstants<CharType>::m_cbracket_close;
                    
                }
                os << LiteralConstants<CharType>::m_bracket_close;
                os << LiteralConstants<CharType>::m_cbracket_close << std::endl;
            }
            os << LiteralConstants<CharType>::m_cbracket_close << std::endl;
        }
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
        if constexpr (std::is_same_v<CharType, char>)
        {
            if (m_isQuiet)
            {
                std::cout.setstate(std::ios::failbit);
            }
            else {
                std::cout.clear();
            }
            return std::cout;
        }
        else {
            if (m_isQuiet)
            {
                std::wcout.setstate(std::ios::failbit);
            }
            else {
                std::wcout.clear();
            }
            return std::wcout;
        }
    }

    OutputStream& GetErrorOutputStream()
    {
        if constexpr (std::is_same_v<CharType, char>)
        {
            return std::cerr;
        }
        else {
            return std::wcerr;
        }
    }

    void Print_(StringType& str)
    {
        bool curIsQuiet = m_isQuiet;
        //If the user requested to be quiet but to output JSON results we disable quiet mode just
        //to output the JSON tables and then reenable it afterwards.
        if ((m_outputType == TableType::JSON || m_outputType == TableType::ALL) && m_isQuiet)
        {
            m_isQuiet = false;
        }

        GetOutputStream() << str;

        if ((m_outputType == TableType::JSON || m_outputType == TableType::ALL) && curIsQuiet)
        {
            m_isQuiet = true;
        }
    }

    void OutputToFile(StringType& str)
    {
        FileStream file;
        file.open(m_filename, std::fstream::out | std::fstream::trunc);
        if (file.is_open())
        {
            file << str;
            file.close();
        }
        else {
            GetErrorOutputStream() << LITERALCONSTANTS_GET("Unable to open ") << m_filename << std::endl;
        }
    }

    template <typename T>
    void Print(TableOutput<T, CharType>& table, bool printJson = false)
    {
        StringType s = table.Print(TableType::PRETTY).str();
        GetOutputStream() << s;
        if ((m_outputType == TableType::JSON || m_outputType == TableType::ALL) && printJson)
        {
            StringType sj = table.Print(TableType::JSON).str();
            if (!m_shouldWriteToFile)
            {
                Print_(sj);
            }
            else {
                OutputToFile(sj);
            }
        }
    }

    void Print(WPerfStatJSON<CharType>& table)
    {
        if (m_outputType == TableType::JSON || m_outputType == TableType::ALL)
        {
            StringType s = table.Print().str();
            if (!m_shouldWriteToFile)
            {
                Print_(s);
            }
            else {
                OutputToFile(s);
            }
        }
    }

    void Print(WPerfListJSON<CharType>& table)
    {
        if (m_outputType == TableType::JSON || m_outputType == TableType::ALL)
        {
            StringType s = table.Print().str();
            if (!m_shouldWriteToFile)
            {
                Print_(s);
            }
            else {
                OutputToFile(s);
            }
        }
    }

    void Print(WPerfSamplingJSON<CharType>& table)
    {
        if (m_outputType == TableType::JSON || m_outputType == TableType::ALL)
        {
            StringType s = table.Print().str();
            if (!m_shouldWriteToFile)
            {
                Print_(s);
            }
            else {
                OutputToFile(s);
            }
        }
    }
};

// Handy aliases, the L at the end of each stands for local.
using GlobalCharType = wchar_t;
using GlobalStringType = std::basic_string<GlobalCharType>;
using ColumnAlignL = enum PrettyTable<GlobalCharType>::ColumnAlign;

template <bool isVerbose>
using PredefinedEventsOutputTraitsL = PredefinedEventsOutputTraits<GlobalCharType, isVerbose>;

template <bool isMultiplexing>
using PerformanceCounterOutputTraitsL = PerformanceCounterOutputTraits<GlobalCharType, isMultiplexing>;

template <bool isMultiplexing>
using SystemwidePerformanceCounterOutputTraitsL = SystemwidePerformanceCounterOutputTraits<GlobalCharType, isMultiplexing>;

using L3CacheMetricOutputTraitsL = L3CacheMetricOutputTraits<GlobalCharType>;
using TelemetrySolutionMetricOutputTraitsL = TelemetrySolutionMetricOutputTraits<GlobalCharType>;
using PMUPerformanceCounterOutputTraitsL = PMUPerformanceCounterOutputTraits<GlobalCharType>;
using DDRMetricOutputTraitsL = DDRMetricOutputTraits<GlobalCharType>;
using TestOutputTraitsL = TestOutputTraits<GlobalCharType>;

template <bool isVerbose>
using MetricOutputTraitsL = MetricOutputTraits<GlobalCharType, isVerbose>;

using VersionOutputTraitsL = VersionOutputTraits<GlobalCharType>;
using SamplingOutputTraitsL = SamplingOutputTraits<GlobalCharType>;
using SamplingAnnotateOutputTraitsL = SamplingAnnotateOutputTraits<GlobalCharType>;

using OutputControlL = OutputControl<GlobalCharType>;

// Global variables to handle output.
extern OutputControlL m_out;
extern WPerfStatJSON<GlobalCharType> m_globalJSON;
extern WPerfListJSON<GlobalCharType> m_globalListJSON;
extern WPerfSamplingJSON<GlobalCharType> m_globalSamplingJSON;

extern TableType m_outputType;
