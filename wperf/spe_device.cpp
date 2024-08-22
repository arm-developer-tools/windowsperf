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

#include "spe_device.h"

namespace SPEParser
{
    constexpr UINT8 MASK_MIDDLE = 0xCF;
    constexpr UINT8 MASK_LAST2 = 0xFC;
    constexpr UINT8 MASK_LAST3 = 0xF8;

    enum class PacketType : UINT8
    {
        PADDING = 0x00,
        END = 0x01,
        LONG_HEADER = 0x20,
        EVENTS = 0x42,
        DATA_SOURCE = 0x43,
        OPERATION_TYPE = 0x48,
        CONTEXT = 0x64,
        TIMESTAMP = 0x71,
        COUNTER = 0x98,
        ADDRESS = 0xB0,
        ADDRESS_LONG,
        COUNTER_LONG,
        UNKNOWN
    };

    enum class ParsingState : UINT8
    {
        READING_HEADER,
        READING_DATA,
        FINISHED,
    };

    enum class AdressType : UINT8
    {
        PC = 0,
        BT,         // Branch target
        VA,         // Virtual address
        PA,         // Physical address
        PBT         // Previous branch address
    };
    
    enum class OperationTypeClass : UINT8
    {
        OTHER = 0,
        LOAD_STORE_ATOMIC,
        BRANCH_OR_EXCEPTION
    };

    static PacketType get_packet_type(UINT8 hdr)
    {
        if (hdr == static_cast<std::underlying_type_t<PacketType>>(PacketType::PADDING))                       return PacketType::PADDING;
        if (hdr == static_cast<std::underlying_type_t<PacketType>>(PacketType::END))                           return PacketType::END;
        if (hdr == static_cast<std::underlying_type_t<PacketType>>(PacketType::TIMESTAMP))                     return PacketType::TIMESTAMP;
        if ((hdr & MASK_MIDDLE) == static_cast<std::underlying_type_t<PacketType>>(PacketType::EVENTS))        return PacketType::EVENTS;
        if ((hdr & MASK_MIDDLE) == static_cast<std::underlying_type_t<PacketType>>(PacketType::DATA_SOURCE))   return PacketType::DATA_SOURCE;
        if ((hdr & MASK_LAST2) == static_cast<std::underlying_type_t<PacketType>>(PacketType::CONTEXT))        return PacketType::CONTEXT;
        if ((hdr & MASK_LAST2) == static_cast<std::underlying_type_t<PacketType>>(PacketType::OPERATION_TYPE)) return PacketType::OPERATION_TYPE;
        if ((hdr & MASK_LAST2) == static_cast<std::underlying_type_t<PacketType>>(PacketType::LONG_HEADER))    return PacketType::LONG_HEADER;
        if ((hdr & MASK_LAST3) == static_cast<std::underlying_type_t<PacketType>>(PacketType::ADDRESS))        return PacketType::ADDRESS;
        if ((hdr & MASK_LAST3) == static_cast<std::underlying_type_t<PacketType>>(PacketType::COUNTER))        return PacketType::COUNTER;

        return PacketType::UNKNOWN;
    }

    class Packet
    {
    public:
        UINT8 m_header0, m_header1;
        size_t m_bytes_read;
        size_t m_bytes_to_read;
        size_t m_payload_index;
        UINT64 m_payload;
        PacketType m_type;
        ParsingState m_state;

        Packet() : m_header0(0), m_header1(0), m_type(PacketType::UNKNOWN), m_state(ParsingState::READING_HEADER), m_bytes_read(0), m_bytes_to_read(0), m_payload(0), m_payload_index(0) {}

        void add(UINT8 byte)
        {
            if (m_state == ParsingState::READING_HEADER)
            {
                PacketType type = get_packet_type(byte);
                switch (type)
                {
                case PacketType::ADDRESS:
                case PacketType::COUNTER:
                {
                    if (m_bytes_read >= 1)
                    {
                        m_type = type == PacketType::ADDRESS ? PacketType::ADDRESS_LONG : PacketType::COUNTER_LONG;
                        m_header1 = byte;
                    }
                    else {
                        m_type = type;
                        m_header0 = byte;
                        m_bytes_to_read = 8;
                    }
                    m_bytes_to_read = type == PacketType::ADDRESS ? 8 : 2;
                    m_state = ParsingState::READING_DATA;
                    break;
                }
                case PacketType::END:
                {
                    m_type = type;
                    m_header0 = byte;
                    m_state = ParsingState::FINISHED;
                    m_bytes_to_read = 0;
                    break;
                }
                case PacketType::EVENTS:
                case PacketType::DATA_SOURCE:
                {
                    m_type = type;
                    m_header0 = byte;
                    m_state = ParsingState::READING_DATA;
                    m_bytes_to_read = ((m_header0 & (~MASK_MIDDLE)) >> 4) + 1;
                    break;
                }
                case PacketType::OPERATION_TYPE:
                {
                    m_type = type;
                    m_header0 = byte;
                    m_state = ParsingState::READING_DATA;
                    m_bytes_to_read = 1;
                    break;
                }
                case PacketType::CONTEXT:
                {
                    m_type = type;
                    m_header0 = byte;
                    m_state = ParsingState::READING_DATA;
                    m_bytes_to_read = 4;
                    break;
                }
                case PacketType::TIMESTAMP:
                {
                    m_type = type;
                    m_header0 = byte;
                    m_state = ParsingState::READING_DATA;
                    m_bytes_to_read = 8;
                    break;
                }
                case PacketType::LONG_HEADER: m_header0 = byte; break;
                }
            }
            else if (m_state == ParsingState::FINISHED) {
            }
            else {
                m_bytes_to_read--;
                m_payload |= (static_cast<uint64_t>(byte) << m_payload_index * 8);
                m_payload_index++;
                if (m_bytes_to_read == 0)
                {
                    m_state = ParsingState::FINISHED;
                }
            }
            m_bytes_read++;
        }
    };
    class AddressPacket : public Packet
    {
    public:
        UINT64 m_address;
        UINT8 m_index;
        AdressType m_address_type;


        AddressPacket(const Packet& p)
        {
            m_payload = p.m_payload;
            m_type = p.m_type;
            m_header0 = p.m_header0;
            m_header1 = p.m_header1;
            m_address = m_payload & 0x00FFFFFFFFFFFF;

            if (m_type == PacketType::ADDRESS_LONG)
            {
                m_index = ((m_header0 & (~MASK_LAST2)) << 3) || (m_header1 & (~MASK_LAST3));
            }
            else {
                m_index = m_header0 & (~MASK_LAST3);
            }

            m_address_type = static_cast<AdressType>(m_index);
        }
    };

    class EventPacket : public Packet
    {
        union EventBit
        {
            struct BitFields
            {
                unsigned generated_exception : 1;
                unsigned architecturally_executed : 1;
                unsigned level1_data_cache_acess : 1;
                unsigned level1_data_cache_refill : 1;
                unsigned tlb_access : 1;
                unsigned tlb_walk : 1;
                unsigned not_taken : 1;
                unsigned mispredicted : 1;
                unsigned last_level_cache_access : 1;
                unsigned last_level_cache_miss : 1;
                unsigned remote_access : 1;
                unsigned alignment : 1;
                unsigned res0 : 4;
                unsigned zero : 1;
                unsigned partial_predicate : 1;
                unsigned empty_predicate : 1;
                unsigned res1 : 19;
            } bitFields;
            UINT64 val;
        };

        EventBit eb;
    public:
        EventPacket(const Packet& p)
        {
            m_payload = p.m_payload;
            m_type = p.m_type;
            m_header0 = p.m_header0;
            m_header1 = p.m_header1;
            eb.val = m_payload;
        }

        std::wstring get_event_desc() const
        {
            std::wstring desc = L"";
            if (eb.bitFields.alignment) desc += L"alignment+";
            if (eb.bitFields.architecturally_executed) desc += L"retired+";
            if (eb.bitFields.empty_predicate) desc += L"sve-empty-predicate+";
            if (eb.bitFields.generated_exception) desc += L"generated-exception+";
            if (eb.bitFields.last_level_cache_access) desc += L"last-level-cache-access+";
            if (eb.bitFields.last_level_cache_miss) desc += L"last-level-cache-miss+";
            if (eb.bitFields.level1_data_cache_acess) desc += L"level1-data-cache-access+";
            if (eb.bitFields.level1_data_cache_refill) desc += L"level1-data-cache-refill+";
            if (eb.bitFields.mispredicted) desc += L"mispredicted+";
            if (eb.bitFields.not_taken) desc += L"not-taken+";
            if (eb.bitFields.partial_predicate) desc += L"sve-partial-predicate+";
            if (eb.bitFields.remote_access) desc += L"remote-acess+";
            if (eb.bitFields.tlb_access) desc += L"tlb_access+";
            if (eb.bitFields.tlb_walk) desc += L"tlb-walk+";
            return std::wstring(desc.begin(), desc.end() - 1);
        }
    };

    class OperationTypePacket : public Packet
    {
    public:
        OperationTypeClass m_class;
        bool m_is_store;

        OperationTypePacket(const Packet& p)
        {
            m_payload = p.m_payload;
            m_type = p.m_type;
            m_header0 = p.m_header0;
            m_header1 = p.m_header1;
            m_class = static_cast<OperationTypeClass>(m_header0 & (~MASK_LAST2));
            m_is_store = m_payload & 1;
        }

        std::wstring get_subclass() const
        {
            switch (m_class)
            {
            case OperationTypeClass::LOAD_STORE_ATOMIC:
            {
                uint64_t v = m_payload >> 1;
                std::wstring ldst = (m_payload & 1) ? L"STORE-" : L"LOAD-";
                if (v == 0)
                {
                    return ldst + L"GP";
                }
                else if (v == 2)
                {
                    return ldst + L"SIMD-FP";
                }
                else {
                    return ldst + L"OTHER";
                }
                break;
            }
            case OperationTypeClass::BRANCH_OR_EXCEPTION:
                return std::wstring(((m_payload & 1) ? L"CONDITIONAL-" : L"UNCONDITIONAL-")) + std::wstring(((m_payload & 2) ? L"INDIRECT" : L"DIRECT"));
            }
            return L"";
        }

        std::wstring get_class()
        {
            switch (m_class)
            {
            case OperationTypeClass::OTHER:                return L"OTHER";
            case OperationTypeClass::LOAD_STORE_ATOMIC:    return L"LOAD_STORE_ATOMIC";
            case OperationTypeClass::BRANCH_OR_EXCEPTION:  return L"BRANCH_OR_EXCEPTION";
            }
            return L"";
        }

        std::wstring get_desc()
        {
            return get_class() + ((m_class != OperationTypeClass::OTHER) ? L"-" : L"") + get_subclass();
        }
    };

    class Record
    {
    public:
        std::vector<Packet> m_packets;
        std::wstring m_event_name;
        std::wstring m_optype_name;
        std::wstring m_full_desc;
        UINT64 m_pc;

        Record() : m_pc(0) {}

        void parse_record_data()
        {
            for (const auto& packet : m_packets)
            {
                switch (packet.m_type)
                {
                case PacketType::ADDRESS:
                case PacketType::ADDRESS_LONG:
                {
                    AddressPacket ap = packet;
                    if (ap.m_address_type == AdressType::PC)
                    {
                        m_pc = ap.m_address;
                    }
                    break;
                }
                case PacketType::OPERATION_TYPE:
                {
                    OperationTypePacket op = packet;
                    m_optype_name = op.get_desc();
                    break;
                }
                case PacketType::EVENTS:
                {
                    EventPacket ep = packet;
                    m_event_name = ep.get_event_desc();
                    break;
                }
                }
            }
            m_full_desc = m_optype_name + L"/" + m_event_name;
        }
    };

    static std::vector<std::pair<std::wstring, UINT64>> read_spe_buffer(const std::vector<UINT8>& buffer)
    {
        std::vector<std::pair<std::wstring, UINT64>> records;
        std::vector<Packet> packets;
        packets.resize(packets.size() + 1);
        for (const UINT8 byte : buffer)
        {
            packets.back().add(byte);
            if (packets.back().m_state == ParsingState::FINISHED)
            {
                if (packets.back().m_type == PacketType::END || packets.back().m_type == PacketType::TIMESTAMP)
                {
                    Record rec;
                    rec.m_packets = std::vector<Packet>(packets.begin(), packets.end());
                    rec.parse_record_data();
                    records.push_back(std::make_pair(rec.m_full_desc, rec.m_pc));
                    packets.clear();
                }
                packets.resize(packets.size() + 1);
            }
        }
        return records;
    }
}

const std::vector<std::wstring> spe_device::m_filter_names = {
        L"load_filter", L"store_filter", L"branch_filter" };

// Filters also have aliases, this structure helps to translate alias to filter name
const std::map<std::wstring, std::wstring> spe_device::m_filter_names_aliases = {
    { L"ld", L"load_filter", },
    { L"st", L"store_filter", },
    { L"b" , L"branch_filter" }
};

// Filters also have aliases, this structure helps to translate alias to filter name
const std::map<std::wstring, std::wstring> spe_device::m_filter_names_description = {
    { L"load_filter",   L"Enables collection of load sampled operations, including atomic operations that return a value to a register." },
    { L"store_filter",  L"Enables collection of store sampled operations, including all atomic operations." },
    { L"branch_filter", L"Enables collection of branch sampled operations, including direct and indirect branches and exception returns." }
};

spe_device::spe_device() {}

spe_device::~spe_device() {}

void spe_device::init()
{

}

void spe_device::get_samples(const std::vector<UINT8>& spe_buffer, std::vector<FrameChain>& raw_samples, std::map<UINT64, std::wstring>& spe_events)
{
    std::vector<std::pair<std::wstring, UINT64>> records = SPEParser::read_spe_buffer(spe_buffer);
    std::map<std::wstring, unsigned int> event_map;
    UINT32 events_idx = 0;
    for (const auto& rec : records)
    {
        FrameChain fc{ 0 };
        fc.pc = rec.second;
        if (event_map.find(rec.first) != event_map.end())
        {
            fc.spe_event_idx = event_map[rec.first];
        } else {
            fc.spe_event_idx = events_idx;
            spe_events[events_idx] = rec.first;
            event_map[rec.first] = events_idx;
            events_idx++;
        }
        raw_samples.push_back(fc);
    }
}

std::wstring spe_device::get_spe_version_name(UINT64 id_aa64dfr0_el1_value)
{
    UINT8 aa64_pms_ver = ID_AA64DFR0_EL1_PMSVer(id_aa64dfr0_el1_value);

    // Print SPE feature version
    std::wstring spe_str = L"unknown SPE configuration!";
    switch (aa64_pms_ver)
    {
    case 0b000: spe_str = L"not implemented"; break;
    case 0b001: spe_str = L"FEAT_SPE"; break;
    case 0b010: spe_str = L"FEAT_SPEv1p1"; break;
    case 0b011: spe_str = L"FEAT_SPEv1p2"; break;
    case 0b100: spe_str = L"FEAT_SPEv1p3"; break;
    }
    return spe_str;
}

bool spe_device::is_spe_supported(UINT64 id_aa64dfr0_el1_value)
{
#ifdef ENABLE_SPE
    UINT8 aa64_pms_ver = ID_AA64DFR0_EL1_PMSVer(id_aa64dfr0_el1_value);
    return aa64_pms_ver >= 0b001;
#else
    UNREFERENCED_PARAMETER(id_aa64dfr0_el1_value);
    return false;
#endif
}
