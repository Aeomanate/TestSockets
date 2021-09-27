#include <iostream>
#include "src/Server/Server.hpp"
#include "src/Client/Client.hpp"
#include "src/Sockets/Sockets.hpp"

int main() {
    SocketInitializer sockets_init;
    
    int ans = -1;
    while(ans != 0 and ans != 1) {
        std::cout << "Server(0) or client(1)? Ans: ";
        std::cin >> ans;
    }
    
    if(ans == 0) {
        std::cout << "Run server...\n";
        Server().run();
    } else if(ans == 1) {
        std::cout << "Run client...\n";
        Client().run();
    }
    system("pause");
    
    return 0;
}
