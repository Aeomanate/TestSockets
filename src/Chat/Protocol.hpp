//
// Created by Aeomanate on 18.04.2021.
//

#ifndef PROTOCOL_CHAT_HPP
#define PROTOCOL_CHAT_HPP

#include <string>
#include <chrono>
#include <stdexcept>
#include <string_view>
#include <winsock2.h>
#include "../Sockets/Sockets.hpp"

struct ProtocolHeaderMagicId {
  public:
    void setDefault();
    [[nodiscard]] bool isMatch4Bytes() const;
    
  private:
    u_long m_network_id = { };
    static inline const u_long magic_id = 0x881088;
};

class ProtocolHeaderTime {
  public:
    void setCur();
    [[nodiscard]] u_short getHours() const;
    [[nodiscard]] u_short getMinutes() const;
    
  private:
    u_short m_network_hours = { };
    u_short m_network_minutes = { };
};

class ProtocolHeaderUsername {
  public:
    void set(std::string const& username);
    [[nodiscard]] char const* get() const;
    
  private:
    char m_str[16] = { };
};

class ProtocolHeaderMessageSize {
  public:
    void set(size_t size);
    [[nodiscard]] u_long get() const;
    
  private:
    u_long m_network_size = { };
};



struct ProtocolHeader;
using ProtocolHeaderPtr = std::unique_ptr<ProtocolHeader>;
struct ProtocolHeader {
    friend class ProtocolHeaderDeleter;
    ProtocolHeaderMagicId m_id;
    ProtocolHeaderTime m_time;
    ProtocolHeaderUsername m_username;
    ProtocolHeaderMessageSize m_message_size;
    [[nodiscard]] size_t getPacketSize() const;
    [[nodiscard]] std::string_view getMessage() const;
    [[nodiscard]] ProtocolHeaderPtr deepCopy() const;
};

using SocketTcpPtr = std::shared_ptr<SocketTCP>;
class ChatSocketTcp {
  public:
    ChatSocketTcp(SocketTcpPtr socket_tcp_ptr, std::string nickname);
    int sendMessage(std::string const& message);
    ProtocolHeaderPtr receiveMessage(int flags = 0);

  private:
    std::string encodeChatMessage(std::string const& username, std::string const& message);
    
  private:
    SocketTcpPtr m_socket_tcp;
    std::string nickname;
    std::string m_buffer;
};

#endif //PROTOCOL_CHAT_HPP
