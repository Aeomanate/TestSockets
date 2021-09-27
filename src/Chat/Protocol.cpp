// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com
//
// Created by Aeomanate on 03.05.2021.
//
#include "Protocol.hpp"

#include <utility>


void ProtocolHeaderMagicId::setDefault() {
    m_network_id = htonl(magic_id);
}

bool ProtocolHeaderMagicId::isMatch4Bytes() const {
    return magic_id == ntohl(m_network_id);
}



void ProtocolHeaderTime::setCur() {
    using clock = std::chrono::system_clock;
    time_t c_time = clock::to_time_t(clock::now());
    auto cur_time = std::localtime(&c_time);
    
    m_network_hours = htons(static_cast<u_short>(cur_time->tm_hour));
    m_network_minutes = htons(static_cast<u_short>(cur_time->tm_min));
}

u_short ProtocolHeaderTime::getHours() const {
    return ntohs(m_network_hours);
}

u_short ProtocolHeaderTime::getMinutes() const {
    return ntohs(m_network_minutes);
}



void ProtocolHeaderUsername::set(std::string const& username) {
    if(username.size() >= sizeof(ProtocolHeaderUsername)) {
        auto max_username_size = std::to_string(sizeof(ProtocolHeaderUsername));
        auto cur_username_size = std::to_string(username.size());
        auto inequality = cur_username_size + ">" + max_username_size;
        throw std::runtime_error("Username too long (" + inequality + ")");
    }
    strcpy(m_str, username.c_str());
}

char const* ProtocolHeaderUsername::get() const {
    return m_str;
}



void ProtocolHeaderMessageSize::set(size_t size) {
    m_network_size = htonl(size);
}
u_long ProtocolHeaderMessageSize::get() const {
    return ntohl(m_network_size);
}



std::string_view ProtocolHeader::getMessage() const {
    char const* message_start = reinterpret_cast<char const*>(this + 1);
    return std::string_view(message_start, m_message_size.get());
}

ProtocolHeaderPtr ProtocolHeader::deepCopy() const {
    ProtocolHeaderPtr header_ptr((ProtocolHeader*) new u_char[getPacketSize()]);
    memcpy(header_ptr.get(), this, getPacketSize());
    return header_ptr;
}

size_t ProtocolHeader::getPacketSize() const {
    return sizeof(*this) + m_message_size.get();
}



ChatSocketTcp::ChatSocketTcp(SocketTcpPtr socket_tcp_ptr, std::string nickname)
: m_socket_tcp(std::move(socket_tcp_ptr))
, nickname(std::move(nickname))
{ }

std::string ChatSocketTcp::encodeChatMessage(std::string const& username, std::string const& message) {
    ProtocolHeader header;
    header.m_id.setDefault();
    header.m_time.setCur();
    header.m_username.set(username);
    header.m_message_size.set(message.size());
    
    std::string str_header(sizeof(header), '\0');
    memcpy(str_header.data(), &header, std::size(str_header));
    return str_header + message;
}

int ChatSocketTcp::sendMessage(std::string const& message) {
    return m_socket_tcp->send(encodeChatMessage(nickname, message));
}

ProtocolHeaderPtr ChatSocketTcp::receiveMessage(int flags) {
    
    while(true) {
        auto& header = *(ProtocolHeader*)m_buffer.data();
        
        bool is_header_present = m_buffer.size() >= sizeof(header);
        if(is_header_present and !header.m_id.isMatch4Bytes()) {
            throw std::runtime_error("Magic id not match");
        }
        bool is_message_present = is_header_present and m_buffer.size() >= header.m_message_size.get();
        
        if(is_message_present) {
            ProtocolHeaderPtr message = header.deepCopy();
            int msize = static_cast<int>(message->getPacketSize());
            m_buffer.erase(m_buffer.begin(), m_buffer.begin() + msize);
            return message;
        }
    
        auto packet = m_socket_tcp->receive(flags);
        if(!packet) return { }; // Client disconnect
        m_buffer += *packet;
    }
}
