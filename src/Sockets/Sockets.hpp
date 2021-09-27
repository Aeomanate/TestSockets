//
// Created by Aeomanate on 07.04.2021.
//

#ifndef TEST_SOCKETS_SOCKETS_HPP
#define TEST_SOCKETS_SOCKETS_HPP
#include <iostream>
#include <memory>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <regex>

enum class SockLogLevel { NO, ERRORS, ALL, DEFAULT = ERRORS };
extern SockLogLevel sock_log_level;

std::string methodname(std::string&& f_info);
// Report error or log socket events
template <class... ToOut>
static int
socketReport(bool is_error = false, ToOut&&... to_out) {
    auto& out = is_error ? std::cerr : std::cout;
    bool is_out_all = sock_log_level == SockLogLevel::ALL;
    bool is_out_err = sock_log_level == SockLogLevel::ERRORS;
    static char const* error_msg = is_error ? "{ERROR} " : "";
    if(is_out_all or (is_out_err and is_error)) {
        ((out << error_msg) << ... << to_out) << "\n";
    }
    return is_error;
}
#define METHODNAME methodname(__PRETTY_FUNCTION__)
#define srep(is_error, ...) \
socketReport((is_error), METHODNAME, __VA_ARGS__)


class SocketInitializer {
  public:
    SocketInitializer();
    ~SocketInitializer();
  private:
    WSADATA m_wsa_data = {};
};

class SocketAddress;
using SocketAddressPtr = std::shared_ptr<SocketAddress>;
class SocketAddress {
    friend class Socket;
    friend class SocketUDP;
    friend class SocketTCP;
    friend class SocketTcpListener;
  public:
    static SocketAddressPtr
    ipv4FromDotNotation(std::string const& ipv4AddressPort);
    
    static SocketAddressPtr
    ipv4FromDNS(std::string const& ipv4AddressService);
    
    SocketAddress(uint32_t naddr, uint16_t nport);
    
    explicit SocketAddress(const sockaddr& inSockAddr);
    
    static int getSize();
    [[nodiscard]] std::string getStr() const;
  
  private:
    SocketAddress() = default;
    sockaddr_in& getAsSockAddrIn();
    [[nodiscard]] sockaddr_in const& getAsSockAddrIn() const;
    sockaddr m_addr = {};
};



class Socket {
  public:
    Socket() = delete;
    bool bindToAllLocalIps(uint16_t hport);
    bool bind(SocketAddress const& address);
  
  protected:
    template <class SocketT>
    std::shared_ptr<SocketT> static create(int af, int type) {
        SOCKET s = socket(af, type, 0);
        std::shared_ptr<SocketT> shared_socket;
        if(!srep(s == INVALID_SOCKET, " -> ", s)) {
            shared_socket.reset(new SocketT(s));
        }
        return shared_socket;
    }
    explicit Socket(SOCKET socket);
    ~Socket();
    SOCKET m_socket;
    
  private:
    bool bindImpl(SocketAddress const& address);
};



class SocketUDP;
using SocketUdpPtr = std::shared_ptr<SocketUDP>;
class SocketUDP: public Socket {
    friend class Socket;
  public:
    static SocketUdpPtr create();
    int sendto(
        std::string const& data,
        SocketAddress const& to,
        int flags = 0
    );
    std::string receive(int flags = 0);
    [[nodiscard]] SocketAddress const& getLastSender() const;
    
  private:
    explicit SocketUDP(SOCKET socket);
  
  private:
    SocketAddress m_last_receiver = {};
};



class SocketTCP;
class SocketTcpListener;
using SocketTcpPtr = std::shared_ptr<SocketTCP>;
using SocketTcpListenerPtr = std::shared_ptr<SocketTcpListener>;

class SocketTCP: public Socket {
    friend class Socket;
    friend class SocketTcpListener;
  public:
    static SocketTcpPtr create(SocketAddress const& remote_host);
    int send(std::string const& buffer, int flags = 0);
    std::optional<std::string> receive(int flags = 0);
    [[nodiscard]] SocketAddress const& getConnectionAddr() const;
    
  private:
    SocketTCP(SOCKET socket, SocketAddress const& connection_addr);
    explicit SocketTCP(SOCKET socket);
    SocketAddress m_connection_addr;
};



class SocketTcpListener: public Socket {
    friend class Socket;
  public:
    static SocketTcpListenerPtr create(SocketAddress const& address);
    static SocketTcpListenerPtr create(uint16_t hport);
    SocketTcpPtr accept();
  
  private:
    static SocketTcpListenerPtr create();
    explicit SocketTcpListener(SOCKET socket);
    bool listen();
};

#endif // TEST_SOCKETS_SOCKETS_HPP
