// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com
//
// Created by Aeomanate on 07.04.2021.
#include "Sockets.hpp"

std::string methodname(std::string&& f_info) {
    // Class::Name [Template params]
    std::regex expr(R"---((\S+)(?=\(.*?\))(\(.*\))(\s*\[.*?\])*)---");
    std::smatch results;
    std::regex_search(f_info, results, expr);
    return results[1].str() + results[3].str();
}

SockLogLevel sock_log_level = SockLogLevel::DEFAULT;

SocketInitializer::SocketInitializer() {
    int err_code = WSAStartup(MAKEWORD(2, 2), &m_wsa_data);
    if(err_code) {
        throw std::runtime_error(
            "WinSick2 initialize error: " +
            std::to_string(err_code)
        );
    }
}

SocketInitializer::~SocketInitializer() { WSACleanup(); }



std::pair<std::string, std::string> hostAndService(std::string const& ipv4AddressPort) {
    std::string host = ipv4AddressPort, service = "0";
    size_t pos = ipv4AddressPort.find_last_of(':');
    if(pos != std::string::npos) {
        host.erase(host.begin() + (int)pos, host.end());
        service = ipv4AddressPort.substr(pos + 1);
    }
    return { std::move(host), std::move(service) };
}

SocketAddressPtr SocketAddress::ipv4FromDotNotation(std::string const& ipv4AddressPort) {
    auto&& [host, service] = hostAndService(ipv4AddressPort);
    
    uint32_t addr = inet_addr(host.c_str());
    auto port = (uint16_t)std::stoi(service);
    
    SocketAddressPtr ret;
    bool is_error = addr == INADDR_NONE;
    char const* parsed_addr = inet_ntoa(*(in_addr*) &addr);
    if(!srep(is_error, "(", host, ") -> ", parsed_addr)) {
        ret.reset(new SocketAddress(addr, ntohs(port)));
    }
    return ret;
}

SocketAddressPtr SocketAddress::ipv4FromDNS(std::string const& ipv4AddressService) {
    auto&& [host, service] = hostAndService(ipv4AddressService);
    
    addrinfo hint = { }, *result = nullptr;
    hint.ai_family = AF_INET;
    int error = getaddrinfo(host.c_str(), service.c_str(), &hint, &result);
    std::unique_ptr<addrinfo, void(*)(addrinfo*)> guard(
        result, [] (addrinfo* ai) { freeaddrinfo(ai); }
    );
    
    bool is_error = error != 0 or result == nullptr;
    if(srep(is_error, "::getaddrinfo(", host, ":", service, ") -> ",
            "Err: ", error, ", ptr: ", result)) {
        return nullptr;
    }
    while(!result->ai_addr && result->ai_next) {
        result = result->ai_next;
    }
    if(srep(!result->ai_addr, ", find exists ipv4 for ", host, ":", service)) {
        return nullptr;
    }
    
    return std::make_shared<SocketAddress>(*result->ai_addr);
}

SocketAddress::SocketAddress(uint32_t naddr, uint16_t nport) {
    getAsSockAddrIn().sin_family = AF_INET;
    getAsSockAddrIn().sin_addr.S_un.S_addr = naddr;
    getAsSockAddrIn().sin_port = nport;
}

SocketAddress::SocketAddress(sockaddr const& inSockAddr) {
    memcpy(&m_addr, &inSockAddr, sizeof(sockaddr));
}

int SocketAddress::getSize() {
    return (int)sizeof(sockaddr);
}

std::string SocketAddress::getStr() const {
    return inet_ntoa(getAsSockAddrIn().sin_addr) +
           (":" + std::to_string(ntohs(getAsSockAddrIn().sin_port)));
}

sockaddr_in& SocketAddress::getAsSockAddrIn() {
    return reinterpret_cast<sockaddr_in&>(m_addr);
}

sockaddr_in const& SocketAddress::getAsSockAddrIn() const {
    return reinterpret_cast<sockaddr_in const&>(m_addr);
}



bool Socket::bindToAllLocalIps(uint16_t hport) {
    return bindImpl(SocketAddress(htons(INADDR_ANY), htons(hport)));
}

bool Socket::bind(SocketAddress const& address) {
    return bindImpl(address);
}

bool Socket::bindImpl(SocketAddress const& address) {
    int error = ::bind(m_socket, &address.m_addr, SocketAddress::getSize());
    return !srep(error == SOCKET_ERROR, "(", address.getStr(), ") -> ", error);
}

Socket::Socket(SOCKET socket)
: m_socket(socket)
{ }

Socket::~Socket() {
    closesocket(m_socket);
}



SocketUdpPtr SocketUDP::create() {
    return Socket::create<SocketUDP>(AF_INET, SOCK_DGRAM);
}
int SocketUDP::sendto(
    std::string const& data,
    SocketAddress const& to,
    int flags
) {
    int bytes = ::sendto(
        m_socket, data.c_str(), (int)data.size(), flags,
        &to.m_addr, SocketAddress::getSize()
    );
    bool is_error = bytes == SOCKET_ERROR or bytes != (int)data.size();
    srep(is_error, "(", data.size(), "B) -> ", to.getStr());
    return bytes;
}

SocketUDP::SocketUDP(SOCKET socket)
: Socket(socket)
{}

std::string SocketUDP::receive(int flags) {
    char buffer[65536] = {};
    int addr_length = SocketAddress::getSize();
    srep(false, ", receive...");
    int bytes = recvfrom(
        m_socket, buffer, sizeof(buffer),
        flags, &m_last_receiver.m_addr, &addr_length
    );
    std::string received;
    if(!srep(bytes == SOCKET_ERROR, " -> ", bytes, "B")) received = buffer;
    return received;
}

SocketAddress const& SocketUDP::getLastSender() const {
    return m_last_receiver;
}



SocketTcpListenerPtr SocketTcpListener::create(SocketAddress const& address) {
    SocketTcpListenerPtr socket = create();
    socket->bind(address);
    socket->listen();
    return socket;
}

SocketTcpListenerPtr SocketTcpListener::create(uint16_t hport) {
    SocketTcpListenerPtr socket = create();
    socket->bindToAllLocalIps(hport);
    socket->listen();
    return socket;
}

SocketTcpListenerPtr SocketTcpListener::create() {
    return Socket::create<SocketTcpListener>(AF_INET, SOCK_STREAM);
}

SocketTcpPtr SocketTcpListener::accept() {
    SocketAddress connection_addr;
    int addr_len = SocketAddress::getSize();
    SOCKET socket = ::accept(m_socket, &connection_addr.m_addr, &addr_len);
    
    SocketTcpPtr socket_ptr;
    if(!srep(socket == INVALID_SOCKET, " -> ", socket)) {
        socket_ptr.reset(new SocketTCP(socket, connection_addr));
    }
    return socket_ptr;
}

bool SocketTcpListener::listen() {
    int error = ::listen(m_socket, SOMAXCONN);
    return !srep(
        error == SOCKET_ERROR,
        "(SOCKET(", m_socket, "), ", SOMAXCONN, ") -> ", error
    );
}

SocketTcpListener::SocketTcpListener(SOCKET socket)
: Socket(socket)
{ }



SocketTcpPtr SocketTCP::create(SocketAddress const& remote_host) {
    SocketTcpPtr socket = Socket::create<SocketTCP>(AF_INET, SOCK_STREAM);
    if(socket) {
        srep(false, METHODNAME, ", connect to ", remote_host.getStr());
        int err = ::connect(socket->m_socket, &remote_host.m_addr, SocketAddress::getSize());
        if(!srep(err == SOCKET_ERROR, " -> ", socket)) {
            socket->m_connection_addr = remote_host;
        }
    }
    return socket;
}

int SocketTCP::send(std::string const& buffer, int flags) {
    int bytes = ::send(m_socket, buffer.c_str(), (int)buffer.size(), flags);
    srep(
        bytes == SOCKET_ERROR,
        "(SOCKET(", m_socket, "), ", buffer.size(), "B, flags: ", flags, ") -> ",
        bytes, "B"
    );
    return bytes;
}

std::optional<std::string> SocketTCP::receive(int flags) {
    char buffer[65536] = {};
    srep(false, METHODNAME, " from ", m_connection_addr.getStr());
    int bytes = ::recv(m_socket, buffer, sizeof(buffer), flags);
    
    bool ie_err = srep(
        bytes == 0 or bytes == SOCKET_ERROR,
        "(SOCKET(", m_socket, "), ..., flags: ", flags, ") -> ", bytes, "B"
    );
    if(ie_err) {
        return {};
    }
    
    std::string packet;
    if(bytes > 0) {
        packet.resize(size_t(bytes));
        memcpy(packet.data(), buffer, size_t(bytes));
    }
    return packet;
}

SocketAddress const& SocketTCP::getConnectionAddr() const {
    return m_connection_addr;
}

SocketTCP::SocketTCP(SOCKET socket, SocketAddress const& connection_addr)
: Socket(socket)
, m_connection_addr(connection_addr)
{ }

SocketTCP::SocketTCP(SOCKET socket): Socket(socket) { }

