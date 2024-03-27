#pragma once
// BSD 3-Clause License
//
// Copyright (c) 2024, Arm Limited
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

#include <iostream>
#include <sstream>
#include <map>
#include <vector>
#include <typeindex>
#include <algorithm>
#include "outpututil.h"

/* 
The JSON implementation bases itself on the JSON definition from JavaScript where it is defined as a dictionary with keys and values.
The values themselves can be either arrays, raw values or recursively contain JSON Objects. Each key/value pair is defined as a 
property. 

This class defines a generic JSONObject, it contains a map with an arbitray number of StringType/ValueType where StringType is inferred from CharType
and ValueType is a template parameter which can point to any object that implements the operator<< including another JSONObject. The isContainer template argument tells the class
if ValueType is a container, we could use metaprogramming but strings are containers as well and the code got clumsy.

To use a JSONObject, something as simple as JSONObject<int> property where you can use its m_map to define include StringType/int members and then just std::cout << property to 
print results. If you define JSONObject<int, true> each dictionary value will be an array of ints.

To use it recusively simply do JSONObject<JSONObject<int>> in this particular case each dictionary value is itself a JSONObject containing int values. 

This abstract approach allows essentially any combination of key/value pairs to be used.
*/
template <typename ValueType, bool isContainer=false, bool isComplete=true, typename CharType=char>
class JSONObject
{
    typedef typename std::conditional_t<std::is_same_v<CharType, char>, std::string, std::wstring> StringType;
    typedef typename std::conditional_t<std::is_same_v<CharType, char>, std::ostream, std::wostream> OutputStream;
    typedef typename std::conditional_t<std::is_same_v<CharType, char>, std::stringstream, std::wstringstream> StringStream;

public:
    std::map<StringType, ValueType> m_map;

    JSONObject() {}

    JSONObject(std::map<StringType, ValueType> map) : m_map(map) {}

    friend OutputStream& operator<<(OutputStream& os, const JSONObject& json)
    {
        if constexpr(isComplete)
        {
            os << LiteralConstants<CharType>::m_cbracket_open;
        }

        bool isFirst = true;
        for(const auto& [key, val] : json.m_map)
        {
            if(!isFirst)
            {
                os << LiteralConstants<CharType>::m_comma;
            } else {
                isFirst = false;
            }
            StringType newKey(key);
            std::replace(newKey.begin(), newKey.end(), ' ', '_');
            os << LiteralConstants<CharType>::m_quotes << newKey << LiteralConstants<CharType>::m_quotes << LiteralConstants<CharType>::m_colon;
            if constexpr(isContainer)
            {
                os << LiteralConstants<CharType>::m_bracket_open;
                bool isFirstNested = true;
                for(auto cv : val)
                {
                    if(!isFirstNested)
                    {
                        os << LiteralConstants<CharType>::m_comma;
                    } else {
                        isFirstNested = false;
                    }

                    if constexpr(std::is_same_v<ValueType, StringType> || std::is_same_v<ValueType, char> || std::is_same_v<ValueType, wchar_t>)
                    {
                        os << LiteralConstants<CharType>::m_quotes;
                    }

                    os << cv;

                    if constexpr(std::is_same_v<ValueType, StringType> || std::is_same_v<ValueType, char> || std::is_same_v<ValueType, wchar_t>)
                    {
                        os << LiteralConstants<CharType>::m_quotes;
                    }
                }
                os << LiteralConstants<CharType>::m_bracket_close;
            } else {
                    if constexpr(std::is_same_v<ValueType, StringType> || std::is_same_v<ValueType, char> || std::is_same_v<ValueType, wchar_t>)
                    {
                        os << LiteralConstants<CharType>::m_quotes;
                    }

                    os << val;

                    if constexpr(std::is_same_v<ValueType, StringType> || std::is_same_v<ValueType, char> || std::is_same_v<ValueType, wchar_t>)
                    {
                        os << LiteralConstants<CharType>::m_quotes;
                    }
            }
        }

        if constexpr(isComplete)
        {
            os << LiteralConstants<CharType>::m_cbracket_close;
        }
        return os;
    }
};

template <typename CharType, typename Traits, int N = Traits::size - 1>
struct JSONObjectTraits
{
    typedef typename std::conditional_t<std::is_same_v<CharType, char>, std::string, std::wstring> StringType;
    typedef typename std::conditional_t<std::is_same_v<CharType, char>, std::ostream, std::wostream> OutputStream;
    typedef std::decay_t<decltype(std::get<N>(Traits::columns))> Type;

    JSONObject<Type, false, false, CharType> m_property;
    Type m_value{};
    JSONObjectTraits<CharType, Traits, N - 1> previous;

    Type get_t()
    {
        return m_value;
    }

    void* get(int i)
    {
        if (i == N)
        {
            return (void*)&m_value;
        }
        else {
            return previous.get(i);
        }
    }

    void set(int i, StringType& key, void* val)
    {
        if (i == N)
        {
            m_value = *((Type*)val);
            m_property.m_map[key] = *((Type*)val);
        }
        else {
            previous.set(i, key, val);
        }
    }

    friend OutputStream& operator<<(OutputStream& os, const JSONObjectTraits& json)
    {
        os << json.previous;

        if constexpr (N > 0)
        {
            os << LiteralConstants<CharType>::m_comma;
        }

        os << json.m_property;

        if constexpr (N == Traits::size - 1)
        {
            os << LiteralConstants<CharType>::m_cbracket_close;
        }

        return os;
    }
};

template <typename CharType, typename Traits>
struct JSONObjectTraits<CharType, Traits, -1>
{
    typedef typename std::conditional_t<std::is_same_v<CharType, char>, std::string, std::wstring> StringType;
    typedef typename std::conditional_t<std::is_same_v<CharType, char>, std::ostream, std::wostream> OutputStream;

    void get_t() {}
    void* get(int) {}
    void set(int, StringType&, void*) {}
    friend OutputStream& operator<<(OutputStream& os, const JSONObjectTraits&)
    {
        os << LiteralConstants<CharType>::m_cbracket_open;
        return os;
    }
};

/*
TableJSON is an implementation of a table using JSONObjects for compatibility with PrettyTable.
*/
template <typename Traits, typename CharType = char>
class TableJSON
{
    typedef typename std::conditional_t<std::is_same_v<CharType, char>, std::ostream, std::wostream> OutputStream;
    typedef typename std::conditional_t<std::is_same_v<CharType, char>, std::string, std::wstring> StringType;
    typedef JSONObjectTraits<CharType, Traits> ElemT;

    JSONObject<
        std::vector<ElemT>, true, false, CharType> m_properties;
    JSONObject<StringType, false, false, CharType> m_additional_properties;
    JSONObject<uint32_t, false, false, CharType> m_additional_uint_properties;
    JSONObject<uint64_t, false, false, CharType> m_additional_uint64_properties;

    StringType m_key;
    std::vector<StringType> m_column_headers;

    bool m_hasAdditional = false;
    bool m_hasAdditionalUint = false;
    bool m_hasAdditionalUint64 = false;
public:
    bool m_isEmbedded = false;

    TableJSON() {}

    void AddColumn(const StringType& header)
    {
        m_column_headers.push_back(header);
    }

    void SetKey(const StringType& key)
    {
        m_key = key;
        m_properties.m_map[key] = std::vector<ElemT>();
    }

    template <int I = 0>
    void Insert_(ElemT& element)
    {
        m_properties.m_map[m_key].push_back(element);
    }

    template <int I = 0, typename T, typename... Ts>
    void Insert_(ElemT& element, T arg1, Ts... args)
    {
        if (m_column_headers.size() > I)
        {
            element.set(I, m_column_headers[I], (void*)&arg1);
            Insert_<I + 1>(element, args...);
        }
    }

    template <typename... Ts>
    void InsertItem(Ts... args)
    {
        ElemT element;
        Insert_(element, args...);
    }

    template <int I = 0>
    void InsertVector_(size_t) { }

    template <int I = 0, typename T, typename... Ts>
    void InsertVector_(const size_t cur, T arg1, Ts... args)
    {
        if (m_column_headers.size() > I)
        {
            if constexpr (I == 0)
            {
                for (auto& elem : arg1)
                {
                    ElemT value;
                    value.set(I, m_column_headers[I], (void*)&elem);
                    m_properties.m_map[m_key].push_back(value);
                }
            }
            else {
                for (auto i = 0; i < arg1.size(); i++)
                {
                    m_properties.m_map[m_key][cur + i].set(I, m_column_headers[I], (void*)&arg1[i]);
                }
            }
            InsertVector_<I + 1>(cur, args...);
        }
    }

    template <typename... Ts>
    void Insert(Ts... args)
    {
        size_t cur = m_properties.m_map[m_key].size();
        InsertVector_(cur, args...);
    }

    template <typename T>
    void InsertAdditional(const StringType& key, const T& additional)
    {
        if constexpr (std::is_same_v<T, StringType>)
        {
            m_hasAdditional = true;
            m_additional_properties.m_map[key] = additional;
        } 
        else if constexpr(std::is_same_v<T,uint32_t>) {
            m_hasAdditionalUint = true;
            m_additional_uint_properties.m_map[key] = additional;
        }
        else if constexpr (std::is_same_v<T, uint64_t>) {
            m_hasAdditionalUint64 = true;
            m_additional_uint64_properties.m_map[key] = additional;
        }
    }

    friend OutputStream& operator<<(OutputStream& os, const TableJSON& json)
    {
        if (!json.m_isEmbedded)
            os << LiteralConstants<CharType>::m_cbracket_open;
        os << json.m_properties;
        if (json.m_hasAdditional)
        {
            os << LiteralConstants<CharType>::m_comma;
            os << json.m_additional_properties;
        }
        if (json.m_hasAdditionalUint)
        {
            os << LiteralConstants<CharType>::m_comma;
            os << json.m_additional_uint_properties;
        }
        if (json.m_hasAdditionalUint64)
        {
            os << LiteralConstants<CharType>::m_comma;
            os << json.m_additional_uint64_properties;
        }
        if (!json.m_isEmbedded)
            os << LiteralConstants<CharType>::m_cbracket_close;
        return os;
    }
};
