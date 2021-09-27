// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com
//
// Created by Aeomanate on 07.04.2021.
//

#include <iostream>
#include "Server.hpp"
#include "../Sockets/Sockets.hpp"
#include "../Chat/Protocol.hpp"
#include <fstream>

[[maybe_unused]] static void test() {
    auto listen_socket = SocketUDP::create();
    if(!listen_socket->bindToAllLocalIps(60000)) return;
    std::cout << "Received \"" + listen_socket->receive() + "\"\n\n";
    
    std::cout << "Run TCP socket...\n";
    auto tcp_listener = SocketTcpListener::create(60001);
    auto client = tcp_listener->accept();
    std::cout << *client->receive() << "\n";
}

void Server::run() {
    auto listener = SocketTcpListener::create(60001);
    volatile bool is_run = true;
    while(is_run) {
        auto client = ChatSocketTcp(listener->accept(), "");
        while(true) {
            auto from_client = client.receiveMessage();
            if(from_client) {
                std::cout << "Message: >>\""
                          << from_client->getMessage()
                          << "\"<<\n";
            } else {
                break;
            }
        }
    }
}
